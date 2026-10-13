// ipc_latency.cpp —— valley::serve::ipc 请求-响应延迟基准（同进程 fixture，适配 view 接口）
//
// 与 ipc_bench 的区别：不做丢包/流水线，只测「单发单收」的往返延迟，用于对齐
// REPORT-ipc.md §〇.2 的基线（64 B / 1 KB loop p50 ≈ 24 µs、p99 ≤ 100 µs）。
// 设计：服务端跑在独立 Event_loop 线程、客户端跑在另一个 Event_loop 上，
//       主线程只负责编排与统计 —— 无外部进程、无 socket 文件竞态、无日志解析。
//
// 构建：build.ps1 -Src ipc_latency.cpp -Config Release
// 运行：out\Release\ipc_latency.exe [--iters N] [--warmup W] [--payloads 64,1024] [--sections main,loop]
//
// 通道：
//   main —— 主线程发请求、等回调（一次一个在途）
//   loop —— 上一个请求的回调里（loop 线程上）立刻发下一个（一次一个在途，省掉唤醒开销）

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "valley/base/log/context.h"
#include "valley/serve/event_loop.h"
#include "valley/serve/ipc_client.h"
#include "valley/serve/ipc_server.h"

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>   // timeBeginPeriod
#pragma comment(lib, "winmm.lib")
#endif

using namespace valley::serve;
namespace ipc = valley::serve::ipc;

static uint64_t now_ns()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

static double pct(std::vector<double>& v, double p)
{
    if (v.empty()) return 0.0;
    const size_t k = (size_t)((v.size() - 1) * p / 100.0 + 0.5);
    return v[(std::min)(k, v.size() - 1)];
}
static void report(const char* tag, int payload, std::vector<double>& v)
{
    if (v.empty()) { std::printf("%-14s %6d   （无样本）\n", tag, payload); return; }
    std::sort(v.begin(), v.end());
    double sum = 0;
    for (double x : v) sum += x;
    std::printf("%-14s %6d %9.1f %9.1f %9.1f %9.1f %9.1f %9.1f\n",
                tag, (int)v.size(), v.front(), pct(v, 50), pct(v, 90), pct(v, 99), v.back(), sum / v.size());
}

int main(int argc, char** argv)
{
#if defined(_WIN32)
    timeBeginPeriod(1);
#endif
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    int iters = 2000, warmup = 300;
    std::vector<int> payloads{64, 1024};
    bool do_main = true, do_loop = true;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--iters" && i + 1 < argc)        iters = std::atoi(argv[++i]);
        else if (a == "--warmup" && i + 1 < argc)  warmup = std::atoi(argv[++i]);
        else if (a == "--payloads" && i + 1 < argc) {
            payloads.clear();
            std::string s = argv[++i], cur;
            for (char c : s + ",") { if (c == ',') { if (!cur.empty()) payloads.push_back(std::atoi(cur.c_str())); cur.clear(); } else cur += c; }
        }
        else if (a == "--sections" && i + 1 < argc) {
            std::string s = argv[++i];
            do_main = s.find("main") != std::string::npos;
            do_loop = s.find("loop") != std::string::npos;
        }
    }

    valley::base::Log_context::init((valley::base::Log_severity)4);   // ERROR：日志会主导耗时

    char dir[512] = {0};
#if defined(_WIN32)
    DWORD n = GetTempPathA((DWORD)sizeof(dir), dir);
    if (n == 0 || n >= sizeof(dir)) std::snprintf(dir, sizeof(dir), ".\\");
#else
    std::snprintf(dir, sizeof(dir), "/tmp/");
#endif
    std::string base = dir;
    if (!base.empty() && base.back() != '\\' && base.back() != '/') base += '\\';
    char sockbuf[768];
    std::snprintf(sockbuf, sizeof(sockbuf), "%svipc_lat_%d.sock", base.c_str(), (int)GetCurrentProcessId());
    const std::string sock = sockbuf;
    std::remove(sock.c_str());

    // ---- 服务端（自己的 loop 线程）
    Event_loop srv_loop("lat-srv");
    srv_loop.start();
    Ipc_server srv(srv_loop, sock);
    std::atomic<uint64_t> srv_hits{0};
    const bool reg = srv.register_method(1,
        [&srv_hits](const ipc::Session_ptr&, const ipc::Request_view req, ipc::Response& r) {
            srv_hits.fetch_add(1, std::memory_order_relaxed);
            r.make_response(req.method(), req.sequence(), 0, req.payload_size());
            if (req.payload_size() && req.payload())
                std::memcpy(r.payload(), req.payload(), req.payload_size());
        });
    const bool started = srv.start();
    std::printf("server: register=%d start=%d sock=%s\n", (int)reg, (int)started, sock.c_str());
    if (!started) return 3;
    sleep_ms(50);

    // ---- 客户端
    Event_loop cli_loop("lat-cli");
    cli_loop.start();
    Ipc_client cli(cli_loop, sock);
    std::atomic<bool> connected{false};
    cli.set_on_connected([&connected]() { connected.store(true, std::memory_order_release); });
    if (!cli.connect_async()) { std::printf("connect_async 失败\n"); return 3; }
    for (int i = 0; i < 500 && !connected.load(std::memory_order_acquire); ++i) sleep_ms(2);
    if (!connected.load(std::memory_order_acquire)) { std::printf("连接超时\n"); return 3; }
    sleep_ms(100);

    std::printf("iters=%d warmup=%d  单位: 微秒(us)\n\n", iters, warmup);
    std::printf("%-14s %6s %9s %9s %9s %9s %9s %9s\n",
                "payload", "n", "min", "p50", "p90", "p99", "max", "mean");

    for (int pl : payloads)
    {
        // ---- 通道 main：主线程发、等回调
        if (do_main)
        {
            std::vector<double> v;
            v.reserve((size_t)iters);
            bool failed = false;
            for (int i = 0; i < warmup + iters; ++i)
            {
                std::atomic<bool> done{false};
                ipc::Request req;
                req.make_request(1, (uint16_t)pl);
                std::memset(req.payload(), 0x5A, (size_t)pl);
                const uint64_t t0 = now_ns();
                double us = -1.0;
                const std::error_code ec = cli.request_async(req,
                    [&done, &us, t0](const ipc::Response_view resp) {
                        if (resp.payload_size() == 0 && resp.status() != 0) { done.store(true, std::memory_order_release); return; }
                        us = (double)(now_ns() - t0) / 1e3;
                        done.store(true, std::memory_order_release);
                    });
                if (ec.value() != 0) { failed = true; break; }
                while (!done.load(std::memory_order_acquire)) { /* 忙等，精度优先 */ }
                if (i >= warmup && us > 0) v.push_back(us);
            }
            char tag[64];
            std::snprintf(tag, sizeof(tag), "%d B main", pl);
            if (failed) std::printf("%-14s  request_async 返回错误，跳过\n", tag);
            else        report(tag, (int)v.size(), v);
        }

        // ---- 通道 loop：回调里（loop 线程）立刻发下一个
        if (do_loop)
        {
            std::vector<double> v;
            v.reserve((size_t)iters);
            const int total = warmup + iters;
            std::atomic<int> submitted{0};
            std::atomic<bool> all_done{false};
            bool failed = false;

            std::function<void(int)> issue;
            issue = [&](int i) {
                ipc::Request req;
                req.make_request(1, (uint16_t)pl);
                std::memset(req.payload(), 0x5A, (size_t)pl);
                const uint64_t t0 = now_ns();
                const std::error_code ec = cli.request_async(req,
                    [&, t0, i](const ipc::Response_view) {
                        const double us = (double)(now_ns() - t0) / 1e3;
                        if (i >= warmup) v.push_back(us);
                        if (i + 1 < total) issue(i + 1);
                        else all_done.store(true, std::memory_order_release);
                    });
                if (ec.value() != 0) failed = true;
            };
            issue(0);
            const uint64_t t_end = now_ns() + 120ull * 1000000000ull;
            while (!all_done.load(std::memory_order_acquire) && now_ns() < t_end) sleep_ms(1);

            char tag[64];
            std::snprintf(tag, sizeof(tag), "%d B loop", pl);
            if (failed || !all_done.load()) std::printf("%-14s  %s，样本 %d\n", tag,
                                                        failed ? "request_async 返回错误" : "超时未跑完", (int)v.size());
            else                            report(tag, (int)v.size(), v);
        }
    }

    std::printf("\n服务端 echo handler 命中 = %llu\n", (unsigned long long)srv_hits.load());

    cli.disconnect_async();
    sleep_ms(150);
    srv.stop();
    sleep_ms(100);
    cli_loop.stop();
    srv_loop.stop();
    std::remove(sock.c_str());
    return 0;
}
