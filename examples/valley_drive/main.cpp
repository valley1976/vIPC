// SPDX-License-Identifier: MIT
// author: DeepSeek.AI

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <vector>
#include <functional>

#include "valley/conf/conf.h"
#include "valley/shm/channel.h"
#include "valley/exec/executor.h"
#include "valley/exec/notification.h"
#include "valley/thread/thread.h"

using namespace valley;

// ---------- 数据结构（POD，跨进程） ----------
struct SensorData {
    double value;
    uint64_t seq;
    std::chrono::steady_clock::time_point stamp;
};

struct PoseData {
    double x, y, z, yaw;
    uint64_t seq;
    std::chrono::steady_clock::time_point stamp;
};

struct LidarPointCloud {
    uint64_t seq;
    float points[64][3];
    std::chrono::steady_clock::time_point stamp;
};

struct Trajectory {
    double target_x[10];
    double target_y[10];
    uint64_t seq;
};

struct ControlCmd {
    double steer;
    double throttle;
    uint64_t seq;
};

// ---------- 全局控制 ----------
std::atomic<bool> g_stop{ false };

void collect(const SensorData* data) {};
void collect(const PoseData* data) {};
void collect(const LidarPointCloud* data) {};
void collect(const Trajectory* data) {};
void collect(const ControlCmd* data) {};

// ---------- 收集任务函数（替代原 log_task） ----------
// 接收所有需要归档的订阅者（通过引用捕获）
void collect_task(shm::Channel::Subscriber& sub_cam,
    shm::Channel::Subscriber& sub_lidar,
    shm::Channel::Subscriber& sub_traj,
    shm::Channel::Subscriber& sub_ctrl) {
    size_t total = 0;
    // 对每个订阅者调用 catch_up，处理所有积压
    sub_cam.catch_up<SensorData>([&total](const SensorData* d) {
        ++total;
        // 模拟存储（可写入文件或数据库）
        // std::cout << "Collect: camera seq=" << d->seq << std::endl;
        });
    sub_lidar.catch_up<SensorData>([&total](const SensorData* d) { ++total; });
    sub_traj.catch_up<Trajectory>([&total](const Trajectory* d) { ++total; });
    sub_ctrl.catch_up<ControlCmd>([&total](const ControlCmd* d) { ++total; });

    if (total > 0) {
        std::cout << "[Collector] 事件触发归档 " << total << " 条数据" << std::endl;
    }
}

// ---------- 主函数 ----------
int main() {
    // 1. JSON 配置
    const std::string kConfig = R"(
    {
        "application": "ValleyDrive",
        "channel": [
            {"name": "/sensor/pose",   "subscriber": ["realtime", "collector"]},
            {"name": "/sensor/camera", "subscriber": ["realtime", "collector"]},
            {"name": "/sensor/lidar",  "subscriber": ["realtime", "collector"]},
            {"name": "/trajectory",    "subscriber": ["trajectory", "collector"]},
            {"name": "/control",       "subscriber": ["control", "collector"]}
        ],
        "executor": [
            { "name": "/collector", 
              "task": ["collect_pose", 
                       "collect_cam", 
                       "collect_lidar", 
                       "collect_traj",
                       "collect_ctrl"]}
        ]
    }
    )";

    conf::initialize(kConfig, conf::Model::kBoth);

    // 2. 获取所有 Channel
    auto ch_pose  = shm::Channel("/sensor/pose");
    auto ch_cam   = shm::Channel("/sensor/camera");
    auto ch_lidar = shm::Channel("/sensor/lidar");
    auto ch_traj  = shm::Channel("/trajectory");
    auto ch_ctrl  = shm::Channel("/control");

    // 3. 创建发布者（每个 Channel 只能有一个 Publisher）
    auto pub_pose   = shm::Channel::Publisher(ch_pose);
    auto pub_cam    = shm::Channel::Publisher(ch_cam);
    auto pub_lidar  = shm::Channel::Publisher(ch_lidar);
    auto pub_traj   = shm::Channel::Publisher(ch_traj);
    auto pub_ctrl   = shm::Channel::Publisher(ch_ctrl);

    // 4. 创建订阅者（实时模块使用 "realtime" 或 "control"）
    auto sub_realtime_pose  = shm::Channel::Subscriber(ch_pose, "realtime");
    auto sub_realtime_cam   = shm::Channel::Subscriber(ch_cam, "realtime");
    auto sub_realtime_lidar = shm::Channel::Subscriber(ch_lidar, "realtime");
    auto sub_traj           = shm::Channel::Subscriber(ch_traj, "trajectory");
    auto sub_ctrl           = shm::Channel::Subscriber(ch_ctrl, "control");

    // 收集订阅者（使用 "collector" 名称，对应配置中的 subscriber 列表）
    auto sub_collect_pose  = shm::Channel::Subscriber(ch_pose, "collector");
    auto sub_collect_cam   = shm::Channel::Subscriber(ch_cam, "collector");
    auto sub_collect_lidar = shm::Channel::Subscriber(ch_lidar, "collector");
    auto sub_collect_traj  = shm::Channel::Subscriber(ch_traj, "collector");
    auto sub_collect_ctrl  = shm::Channel::Subscriber(ch_ctrl, "collector");

    // 5. 创建收集 Executor 和 Notification

    exec::Executor collector_exe("/collector",
        { 
            {"collect_pose",    [&sub_collect_pose]() { sub_collect_pose.catch_up<PoseData>([](const PoseData*){}); }},
            {"collect_cam",     [&sub_collect_cam]()  { sub_collect_cam.catch_up<SensorData>([](const SensorData*) {}); }},
            {"collect_lidar",   [&sub_collect_lidar](){ sub_collect_lidar.catch_up<LidarPointCloud>([](const LidarPointCloud*) {}); }},
            {"collect_traj",    [&sub_collect_traj]() { sub_collect_traj.catch_up<Trajectory>([](const Trajectory*) {}); }},
            {"collect_ctrl",    [&sub_collect_ctrl]() { sub_collect_ctrl.catch_up<ControlCmd>([](const ControlCmd*) {}); }},
        }
    );

    assert(collector_exe.is_valid());

    exec::Notification nty_pose("/collector", "collect_pose"); assert(nty_pose.is_valid());
    exec::Notification nty_cam("/collector", "collect_cam"); assert(nty_cam.is_valid());
    exec::Notification nty_lidar("/collector", "collect_lidar"); assert(nty_lidar.is_valid());
    exec::Notification nty_traj("/collector", "collect_traj"); assert(nty_traj.is_valid());
    exec::Notification nty_ctrl("/collector", "collect_ctrl"); assert(nty_ctrl.is_valid());

    // 6. 启动收集执行器线程（事件驱动，阻塞等待通知）
    std::thread collector_thread([&collector_exe]() {
        while (!g_stop) {
            try {
                collector_exe.run(g_stop);  // 阻塞，直到有通知或 stop_flag 置 true
            }
            catch (const std::exception& e) {
                std::cerr << "收集任务异常: " << e.what() << "，继续运行" << std::endl;
            }
        }
        });

    // 7. 启动实时处理线程（模拟感知、规划、控制）

    thread::Option option;
    option.set_priority_realtime();

    auto start_timepoint = thread::now();

    std::thread producer_pose = thread::start_cyclic(
        option,
        g_stop,
        start_timepoint,
        std::chrono::milliseconds(10), // 100Hz
        [&pub_pose, &nty_pose]()
        {
            static uint64_t seq = 0;
            static double x = 0.0;
            static double y = 0.0;
            static double yaw = 0.0;

            PoseData pose{};
            pose.seq = ++seq;
            pose.stamp = std::chrono::steady_clock::now();
            x += 0.05;
            y += 0.01;
            yaw = std::atan2(0.01, 0.05);
            pose.x = x; pose.y = y; pose.z = 0.0;
            pose.yaw = yaw;
            pub_pose.write(pose);
            nty_pose.emit();
        }
   );

    std::thread producer_cam = thread::start_cyclic(
        option,
        g_stop,
        start_timepoint,
        std::chrono::milliseconds(50), // 20Hz
        [&pub_cam, &nty_cam]()
        {
            static uint64_t seq = 0;
            static std::default_random_engine eng(42);
            static std::uniform_real_distribution<double> dist(0, 100);
            SensorData data{ dist(eng), ++seq, std::chrono::steady_clock::now() };
            pub_cam.write(data);
            nty_cam.emit();
        }
    );
    
    std::thread producer_lidar = thread::start_cyclic(
        option,
        g_stop,
        start_timepoint,
        std::chrono::milliseconds(100), // 10Hz
        [&pub_lidar, &nty_lidar]()
        {
            static uint64_t seq = 0;
            static std::default_random_engine eng(1);
            static std::uniform_real_distribution<float> dist(-10.0f, 10.0f);

            LidarPointCloud pc{};
            pc.seq = ++seq;
            pc.stamp = std::chrono::steady_clock::now();
            for (int i = 0; i < 64; ++i) {
                pc.points[i][0] = dist(eng);
                pc.points[i][1] = dist(eng);
                pc.points[i][2] = dist(eng);
            }
            pub_lidar.write(pc);
            nty_lidar.emit();
        }
    );
    

    // 实时规划线程（只读最新）

    std::thread planning = thread::start_cyclic(
        option,
        g_stop,
        start_timepoint,
        std::chrono::milliseconds(100), // 10Hz
        [&sub_realtime_pose, &sub_realtime_cam, &sub_realtime_lidar, 
         &pub_traj, &nty_traj]()
        {
            auto* pose = sub_realtime_pose.read_latest<PoseData>();
            auto* cam = sub_realtime_cam.read_latest<SensorData>();
            auto* lidar = sub_realtime_lidar.read_latest<LidarPointCloud>();

            if (pose && cam && lidar)
            {
                static uint64_t seq = 0;

                Trajectory traj{};
                traj.seq = ++seq;
                // 基于当前位置和障碍物生成轨迹（示例）
                for (int i = 0; i < 10; ++i) {
                    traj.target_x[i] = pose->x + i * 2.0;
                    traj.target_y[i] = pose->y + i * 0.5;
                }
                pub_traj.write(traj);
                nty_traj.emit();
            }
        }
        );

    std::thread control = thread::start_cyclic(
        option,
        g_stop,
        start_timepoint,
        std::chrono::milliseconds(20), // 50Hz
        [&sub_ctrl, &pub_ctrl, &nty_ctrl]()
        {
            static uint64_t seq = 0;

            const Trajectory* traj = sub_ctrl.read_latest<Trajectory>();
            if (traj) {
                ControlCmd cmd{ 0.5, 0.3, ++seq };
                pub_ctrl.write(cmd);
                nty_ctrl.emit();
            }
        }
        );

    // 8. 运行 10 秒后退出
    std::this_thread::sleep_for(std::chrono::seconds(10));
    g_stop = true;

    // 9. 等待所有线程结束
    producer_pose.join();
    producer_cam.join();
    producer_lidar.join();
    planning.join();
    control.join();
    collector_thread.join();

    std::cout << "所有线程已安全退出" << std::endl;
    return 0;
}