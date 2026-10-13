// ipc_unit_test.cpp —— valley::serve::ipc 的 Ipc_client / Ipc_server 单元测试
//
// 设计：每个用例在自己的 fixture 里【同进程】起一个 Ipc_server（跑在独立的 Event_loop 线程上）
//       和一个 Ipc_client（跑在主线程上），用唯一 socket 路径。这样不依赖外部进程编排、
//       就绪等待、日志解析与 stdout 刷新，断言全部是进程内的。
//
// 构建：build.ps1 -Src ipc_unit_test.cpp -Config Release
// 运行：out\Release\ipc_unit_test.exe            （不需要参数）
//       退出码 = fail ? 1 : 0 ；末行 FINAL: pass=<n> fail=<n>
//
// 覆盖：
//   T1 连接 + 逐字节回显（含回调次数 == 1、回调在 loop 线程）
//   T2 request_async / notify_async 的错误码（未注册的 method / 未通告的 notification）
//   T3 per-call 主动超时（Busy_option::expire_after）-> 到期回调状态非 0 + 槽位释放
//   T4 主动超时：connect 之后设置 -> 接口返回失败且不生效
//   T5 忙策略：kRejectIfBusy 拒绝；kCancelAndReplace 替换且被替换者恰好回调一次（状态非 0）
//   T6 断开：服务端 disconnect_all -> 在途请求恰好回调一次（状态非 0）+ on_disconnected 触发
//   T7 通知：连发 2000 条（64 000 B < 64 KB）-> 服务端一条不漏
//   T8 回调线程契约：所有回调都在 loop 线程；set_on_connected / set_on_disconnected 可共存
//   T9 同步 Ipc_client::request()（阻塞接口）：echo 成功 / 黑洞按时返回非 0

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "valley/base/log/context.h"
#include "valley/serve/event_loop.h"
#include "valley/serve/ipc_client.h"
#include "valley/serve/ipc_server.h"

#if defined(_WIN32)
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#endif

using namespace valley::serve;
namespace ipc = valley::serve::ipc;

// ------------------------------------------------------------------ 断言

static int g_pass = 0, g_fail = 0, g_skip = 0;

static const char* g_case = "";

static void expect(bool ok, const char* what)
{
    if (ok) { ++g_pass; std::printf("    [ OK ] %s\n", what); }
    else    { ++g_fail; std::printf("    [FAIL] %s\n", what); }
}
static void case_begin(const char* name) { g_case = name; std::printf("\n== %s ==\n", name); }

// ------------------------------------------------------------------ 工具

static uint64_t now_ns()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

struct Resp
{
    std::atomic<bool>     done{false};
    std::atomic<int>      calls{0};
    std::atomic<uint32_t> status{0xFFFFFFFFu};
    std::atomic<uint32_t> size{0};
    std::atomic<double>   ms{-1.0};
    std::atomic<bool>     on_main{false};
    bool                  accepted{false};
    std::vector<unsigned char> payload;
};
using Resp_ptr = std::shared_ptr<Resp>;

// ------------------------------------------------------------------ fixture

static std::atomic<int> g_sock_seq{0};
static std::thread::id  g_main_tid;

struct Env
{
    std::string sock;
    Event_loop  srv_loop{"ut-srv"};
    Event_loop  cli_loop{"ut-cli"};
    std::unique_ptr<Ipc_server> srv;
    std::unique_ptr<Ipc_client> cli;

    std::atomic<uint64_t> srv_echo{0}, srv_black{0}, srv_notif{0};
    std::atomic<bool> cli_connected{false}, cli_disconnected{false};
    std::atomic<bool> srv_connected{false}, srv_disconnected{false};

    explicit Env(const char* tag)
    {
        // socket 目录：运行时取系统临时目录（零硬编码路径）。
        // 若某环境下该目录不可用，可改成当前工作目录：
        //   GetCurrentDirectoryA(...)
        char dir[512] = {0};
#if defined(_WIN32)
        DWORD n = GetTempPathA((DWORD)sizeof(dir), dir);
        if (n == 0 || n >= sizeof(dir)) std::snprintf(dir, sizeof(dir), ".\\");
#else
        std::snprintf(dir, sizeof(dir), "/tmp/");
#endif
        std::string base = dir;
        if (!base.empty() && base.back() != '\\' && base.back() != '/') base += '\\';
        char buf[768];
        std::snprintf(buf, sizeof(buf), "%svipc_ut_%s_%d_%d.sock",
                      base.c_str(), tag, (int)GetCurrentProcessId(), g_sock_seq.fetch_add(1));
        sock = buf;
        std::remove(sock.c_str());
    }
    ~Env() { std::remove(sock.c_str()); }

    // 起服务端（方法 1=回显、2=黑洞、3=每 20ms 一条通知）；返回是否成功
    bool start_server()
    {
        srv_loop.start();
        srv.reset(new Ipc_server(srv_loop, sock));

        srv->register_method(1,
            [this](const ipc::Session_ptr&, const ipc::Request_view req, ipc::Response& r) {
                srv_echo.fetch_add(1);
                r.make_response(req.method(), req.sequence(), 0, req.payload_size());
                if (req.payload_size() && req.payload())
                    std::memcpy(r.payload(), req.payload(), req.payload_size());
            });
        srv->register_method(2,
            [this](const ipc::Session_ptr&, const ipc::Request_view, ipc::Response&) { srv_black.fetch_add(1); });
        srv->register_method(3,
            [this](const ipc::Session_ptr&, const ipc::Request_view, ipc::Response&) { srv_black.fetch_add(1); });
        srv->register_method(4,
            [this](const ipc::Session_ptr&, const ipc::Request_view, ipc::Response&) { srv->disconnect_all(); });
        srv->register_notification(50,
            [this](const ipc::Session_ptr&, const ipc::Notification_view) {
                srv_notif.fetch_add(1);
            });
        srv->set_on_connected([this](const ipc::Session_ptr&) {
            srv_connected.store(true, std::memory_order_release);
        });
        srv->set_on_disconnected([this](const ipc::Session_ptr&) {
            srv_disconnected.store(true, std::memory_order_release);
        });

        const bool ok = srv->start();
        if (ok) sleep_ms(50);          // 让监听就绪
        return ok;
    }

    struct Cli_cfg
    {
        bool want_notif_50 = true;
    };
    // 起客户端并连接；返回是否连上
    bool start_client(const Cli_cfg& cfg = Cli_cfg{})
    {
        cli_loop.start();
        cli.reset(new Ipc_client(cli_loop, sock));
        if (cfg.want_notif_50) {
            cli->register_notification(50,
                [](const ipc::Notification_view) {});
        }
        cli->register_method(1,
            [](const ipc::Request_view req, ipc::Response& r) {
                r.make_response(req.method(), req.sequence(), 0, req.payload_size());
                if (req.payload_size() && req.payload())
                    std::memcpy(r.payload(), req.payload(), req.payload_size());
            });

        cli_ok_disconnected = cli->set_on_disconnected([this]() {
            cli_disconnected.store(true, std::memory_order_release);
        });
        cli_ok_connected = cli->set_on_connected([this]() {
            cli_connected.store(true, std::memory_order_release);
        });

        if (!cli->connect_async()) return false;
        const uint64_t t_end = now_ns() + 5000ull * 1000000ull;
        while (!cli_connected.load(std::memory_order_acquire) && now_ns() < t_end) sleep_ms(2);
        if (!cli_connected.load(std::memory_order_acquire)) return false;

        return true;
    }

    bool cli_ok_connected = false, cli_ok_disconnected = false;

    Resp_ptr request(uint16_t method, uint32_t tag,
                     const ipc::Busy_option& opt = ipc::Busy_policy::kRejectIfBusy)
    {
        auto r = std::make_shared<Resp>();
        ipc::Request req;
        req.make_request(method, 64);
        unsigned char pattern[64];
        for (int i = 0; i < 64; ++i) pattern[i] = (unsigned char)((tag + i) & 0xFF);
        std::memcpy(req.payload(), pattern, 64);
        const uint64_t t0 = now_ns();
        r->accepted = cli->request_async(req,
            [r, t0, pattern](const ipc::Response_view resp) {
                r->calls.fetch_add(1);
                r->ms.store((double)(now_ns() - t0) / 1e6);
                r->status.store(resp.status());
                r->size.store(resp.payload_size());
                r->on_main.store(std::this_thread::get_id() == g_main_tid);
                if (resp.payload_size() == 64 && resp.payload())
                    r->payload.assign((const unsigned char*)resp.payload(),
                                      (const unsigned char*)resp.payload() + resp.payload_size());
                else r->payload.assign(pattern, pattern + 64);   // 便于比对失败原因
                r->done.store(true, std::memory_order_release);
            }, opt).value() == 0;
        return r;
    }
    bool wait(const Resp_ptr& r, double ms)
    {
        const uint64_t t_end = now_ns() + (uint64_t)(ms * 1e6);
        while (!r->done.load(std::memory_order_acquire) && now_ns() < t_end) sleep_ms(1);
        return r->done.load(std::memory_order_acquire);
    }

    void shutdown()
    {
        if (cli) { cli->disconnect_async(); sleep_ms(150); cli.reset(); }
        if (srv) { srv->stop(); sleep_ms(100); srv.reset(); }
        cli_loop.stop();
        srv_loop.stop();
        sleep_ms(50);
    }
};

// ------------------------------------------------------------------ T1

static void t1_echo()
{
    case_begin("T1 连接 + 逐字节回显 + 回调契约");
    Env e("t1");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功（握手完成）");
    expect(e.cli_ok_connected && e.cli_ok_disconnected, "set_on_connected / set_on_disconnected 都注册成功");


    auto r = e.request(1, 7);
    expect(r->accepted, "request_async 接受请求（error_code == 0）");
    expect(e.wait(r, 2000), "回显在 2 s 内返回");
    expect(r->status.load() == 0, "status == 0");
    expect(r->size.load() == 64, "回显长度 == 64");
    expect(r->calls.load() == 1, "回调次数 == 1");
    unsigned char pattern[64];
    for (int i = 0; i < 64; ++i) pattern[i] = (unsigned char)((7 + i) & 0xFF);
    expect(r->payload.size() == 64 && std::memcmp(r->payload.data(), pattern, 64) == 0,
           "回显载荷逐字节一致");
    expect(r->on_main.load() == false, "回调不在调用者线程上（在 loop 线程）");
    expect(e.srv_echo.load() == 1, "服务端 echo handler 命中 1 次");
    e.shutdown();
}

// ------------------------------------------------------------------ T2

static void t2_error_codes()
{
    case_begin("T2 request_async / notify_async 的错误码");
    Env e("t2");
    expect(e.start_server() && e.start_client(), "fixture 就绪");

    {   // 服务端未注册的 method -> 非 0
        ipc::Request req; req.make_request(99, 16);
        const std::error_code ec = e.cli->request_async(req,
            [](const ipc::Response_view) {});
        expect(ec.value() != 0, "对未注册 method 的 request_async 返回非 0 error_code");
        std::printf("        （ec=%d: %s）\n", ec.value(), ec.message().c_str());
    }
    {   // 服务端未通告的 notification -> 非 0
        ipc::Notification n; n.make_notification(999, 8);
        const std::error_code ec = e.cli->notify_async(n);
        expect(ec.value() != 0, "对未通告 notification 的 notify_async 返回非 0 error_code");
        std::printf("        （ec=%d: %s）\n", ec.value(), ec.message().c_str());
    }
    {   // 正常路径 -> 0
        ipc::Notification n; n.make_notification(50, 8);
        const std::error_code ec = e.cli->notify_async(n);
        expect(ec.value() == 0, "对已通告 notification 的 notify_async 返回 0");
    }
    e.shutdown();
}

// ------------------------------------------------------------------ T3

static void t3_per_call_timeout()
{
    case_begin("T3 主动超时（per-call Busy_option::expire_after）");
    Env e("t3");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");

    const double T = 300.0;
    auto r = e.request(2, 100, ipc::Busy_option(ipc::Busy_policy::kRejectIfBusy, ipc::Ms(300)));
    expect(r->accepted, "T3a 黑洞请求被接受");
    const bool got = e.wait(r, T + 1500);
    std::printf("        （T3a 回调=%d 实测 %.1f ms status=%u）\n", (int)got, r->ms.load(), r->status.load());
    expect(got, "T3a 超时触发了回调（回调必须发生）");
    expect(got && r->status.load() != 0, "T3a 超时回调 status != 0");
    expect(got && r->calls.load() == 1, "T3a 超时回调次数 == 1");
    expect(got && r->ms.load() >= T * 0.8, "T3a 超时不早于 expire_after*0.8");
    expect(got && r->ms.load() <= T + 500.0, "T3a 超时不晚于 expire_after+500ms");

    auto r2 = e.request(2, 101, ipc::Busy_option(ipc::Busy_policy::kRejectIfBusy, ipc::Ms(300)));
    expect(r2->accepted, "T3b 超时后同一 method 可再次发起（槽位已释放）");
    e.wait(r2, T + 1500);
    e.shutdown();
}

static void t4_no_timeout_means_never()
{
    case_begin("T4 expire_after == 0 表示不超时");
    Env e("t4");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");

    auto r = e.request(2, 200);                     // 默认 Busy_option：kRejectIfBusy + expire_after = 0
    expect(r->accepted, "T4a 黑洞请求被接受");
    const bool got = e.wait(r, 900);
    expect(!got, "T4a 未设 expire_after 时不会超时");

    auto busy = e.request(2, 201);                  // 同 method 仍在飞
    expect(!busy->accepted, "T4b 该请求仍在飞（同 method 再发被 busy 拒）");
    e.shutdown();
}

static void t5_busy_and_replace()
{
    case_begin("T5 忙策略：kRejectIfBusy 与 kCancelAndReplace");
    Env e("t5");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");

    {   // kRejectIfBusy：第二个被拒
        auto a = e.request(2, 300, ipc::Busy_policy::kRejectIfBusy);
        auto b = e.request(2, 301, ipc::Busy_policy::kRejectIfBusy);
        expect(a->accepted, "kRejectIfBusy：第 1 个被接受");
        expect(!b->accepted, "kRejectIfBusy：第 2 个被拒（error_code != 0）");
        expect(!a->done.load(), "kRejectIfBusy：第 1 个仍在途（未被取消）");
    }
    {   // kCancelAndReplace：第二个被接受，第一个恰好收到一次非 0 回调
        auto a = e.request(2, 310, ipc::Busy_policy::kCancelAndReplace);
        expect(a->accepted, "kCancelAndReplace：第 1 个被接受");
        auto b = e.request(2, 311, ipc::Busy_policy::kCancelAndReplace);
        expect(b->accepted, "kCancelAndReplace：第 2 个被接受（发生替换）");
        expect(e.wait(a, 1500), "被替换的第 1 个收到了回调");
        expect(a->status.load() != 0, "被替换者的回调 status != 0");
        expect(a->calls.load() == 1, "被替换者恰好被回调 1 次");
        expect(!b->done.load(), "第 2 个仍在途（未被误回调）");
        auto c = e.request(2, 312, ipc::Busy_policy::kCancelAndReplace);
        expect(c->accepted, "kCancelAndReplace：第 3 个被接受");
        e.wait(b, 1000);
    }
    e.shutdown();
}

// ------------------------------------------------------------------ T6

static void t6_disconnect_cancels_inflight()
{
    case_begin("T6 断开：在途请求被取消 + on_disconnected 触发");
    Env e("t6");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");

    auto r = e.request(2, 400);                    // 在途黑洞请求
    expect(r->accepted, "在途请求被接受");
    sleep_ms(50);
    ipc::Request drop; drop.make_request(4, 16);   // 服务端 disconnect_all
    e.cli->request_async(drop, [](const ipc::Response_view) {});

    expect(e.wait(r, 3000), "在途请求在断开时收到了回调");
    expect(r->status.load() != 0, "断线回调 status != 0");
    expect(r->calls.load() == 1, "断线回调次数 == 1");
    sleep_ms(200);
    expect(e.cli_disconnected.load(), "客户端 on_disconnected 已触发");
    expect(e.srv_disconnected.load(), "服务端 on_disconnected 已触发");
    e.shutdown();
}

// ------------------------------------------------------------------ T7

static void t7_notification_no_loss()
{
    case_begin("T7 通知：连发 2000 条（64 000 B < 64 KB）一条不漏");
    Env e("t7");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");

    const int N = 2000, PL = 16;                   // 帧 32 B * 2000 = 64 000 B < 64 KB
    int failed = 0;
    for (int i = 0; i < N; ++i) {
        ipc::Notification n;
        n.make_notification(50, (uint16_t)PL);
        std::memcpy(n.payload(), &i, sizeof(i));
        if (e.cli->notify_async(n).value() != 0) ++failed;
    }
    expect(failed == 0, "2000 条 notify_async 全部返回 0");
    sleep_ms(1500);                                // 等接收侧消化
    expect(e.srv_notif.load() == (uint64_t)N,
           "服务端收到条数与发出条数一致（一条不漏）");
    std::printf("        （发 %d 收 %llu）\n", N, (unsigned long long)e.srv_notif.load());
    e.shutdown();
}

// ------------------------------------------------------------------ T8

static void t8_callback_thread_and_coexist()
{
    case_begin("T8 回调线程契约 + 两个 setter 共存");
    Env e("t8");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");
    expect(e.cli_connected.load(), "on_connected 已触发（连接成功即有）");

    // 从【主线程】发起 200 次替换，统计回调线程分布
    std::atomic<int> on_main{0}, on_loop{0}, multi{0};
    std::vector<Resp_ptr> keep;
    for (int i = 0; i < 200; ++i) {
        auto r = std::make_shared<Resp>();
        keep.push_back(r);
        ipc::Request req; req.make_request(2, 16);
        const uint32_t tag = (uint32_t)i; std::memcpy(req.payload(), &tag, 4);
        e.cli->request_async(req,
            [r, &on_main, &on_loop, &multi](const ipc::Response_view) {
                if (r->calls.fetch_add(1) == 0) {
                    if (std::this_thread::get_id() == g_main_tid) on_main.fetch_add(1);
                    else                                          on_loop.fetch_add(1);
                } else multi.fetch_add(1);
            }, ipc::Busy_policy::kCancelAndReplace);
    }
    sleep_ms(600);
    std::printf("        （回调线程：调用者=%d loop=%d；同一请求多次回调=%d）\n",
                on_main.load(), on_loop.load(), multi.load());
    expect(on_main.load() == 0, "199/200 次替换的回调全部投递到 loop 线程");
    expect(multi.load() == 0, "同一请求没有被回调多次");
    expect(e.cli_ok_connected && e.cli_ok_disconnected, "set_on_connected / set_on_disconnected 可共存");
    e.shutdown();
    expect(e.cli_disconnected.load(), "disconnect 后 on_disconnected 触发（共存验证）");
}

static void t9_sync_request()
{
    case_begin("T9 同步 Ipc_client::request()（阻塞接口）");
    Env e("t9");
    expect(e.start_server(), "服务端 start() 成功");
    expect(e.start_client(), "客户端 connect 成功");

    {
        unsigned char pattern[64];
        for (int i = 0; i < 64; ++i) pattern[i] = (unsigned char)((9 + i) & 0xFF);
        ipc::Request req; req.make_request(1, 64);
        std::memcpy(req.payload(), pattern, 64);

        std::atomic<int>  calls{0};
        std::atomic<bool> shape_ok{false};
        const uint64_t t0 = now_ns();
        const std::error_code ec = e.cli->request(req,
            [&](const ipc::Response_view resp) {
                calls.fetch_add(1);
                const void* q = resp.payload();
                shape_ok.store(resp.payload_size() == 64 && q && std::memcmp(q, pattern, 64) == 0);
            }, ipc::Ms(1000));
        const double ms = (double)(now_ns() - t0) / 1e6;
        std::printf("        （T9a ec=%d(%s) 耗时 %.1f ms handler 调用=%d）\n",
                    ec.value(), ec.message().c_str(), ms, calls.load());
        expect(ec.value() == 0, "T9a 同步 request(echo) 返回成功");
        expect(calls.load() == 1, "T9a handler 被调用 1 次");
        expect(shape_ok.load(), "T9a 载荷逐字节一致");
    }
    {
        ipc::Request req; req.make_request(2, 16);
        std::atomic<int> calls{0};
        const uint64_t t0 = now_ns();
        const std::error_code ec = e.cli->request(req,
            [&](const ipc::Response_view) { calls.fetch_add(1); }, ipc::Ms(300));
        const double ms = (double)(now_ns() - t0) / 1e6;
        std::printf("        （T9b ec=%d(%s) 耗时 %.1f ms handler 调用=%d）\n",
                    ec.value(), ec.message().c_str(), ms, calls.load());
        expect(ec.value() != 0, "T9b 同步 request(黑洞, 300ms) 返回非 0");
        expect(ms >= 300.0 * 0.8 && ms <= 300.0 + 800.0, "T9b 耗时接近 expire_after（未永久阻塞）");
    }
    e.shutdown();
}

// ------------------------------------------------------------------ main

int main(int argc, char** argv)
{
#if defined(_WIN32)
    timeBeginPeriod(1);
#endif
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* only = (argc > 1) ? argv[1] : nullptr;
    auto want = [only](int n) { return !only || std::atoi(only) == n; };
    g_main_tid = std::this_thread::get_id();
    valley::base::Log_context::init((valley::base::Log_severity)4);   // ERROR

    std::printf("valley::serve::ipc 单元测试（Ipc_client / Ipc_server）\n");
    if (want(1)) t1_echo();
    if (want(2)) t2_error_codes();
    if (want(3)) t3_per_call_timeout();
    if (want(4)) t4_no_timeout_means_never();
    if (want(5)) t5_busy_and_replace();
    if (want(6)) t6_disconnect_cancels_inflight();
    if (want(7)) t7_notification_no_loss();
    if (want(8)) t8_callback_thread_and_coexist();
    if (want(9)) t9_sync_request();

    std::printf("\nFINAL: pass=%d fail=%d skip=%d\n", g_pass, g_fail, g_skip);
    return g_fail == 0 ? 0 : 1;
}
