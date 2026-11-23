// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#include "data/recorder.h"
#include "data/recorder_config.h"
#include "data/trigger.h"
#include "exec/broker/broker.h"
#include "serve/address.h"

#include "common/sensor_types.h"
#include "sensor/camera_device.h"
#include "sensor/imu_device.h"
#include "sensor/lidar_device.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using sensor_sim::CameraImage;
using sensor_sim::ImuData;
using sensor_sim::LaserScan;
using sensor_sim::SensorMessageHeader;
using sensor_sim::SensorType;

constexpr std::size_t CAMERA_WIDTH = 640;
constexpr std::size_t CAMERA_HEIGHT = 480;

const std::vector<std::string> TOPICS = {
    "/sensor/camera_front", "/sensor/camera_near",
    "/sensor/camera_left",  "/sensor/camera_right",
    "/sensor/imu",          "/sensor/lidar",
};

std::uint64_t now_ns()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::int64_t now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string default_application()
{
    return "valley-drive";
}

void usage(const char* exe)
{
    std::printf(
        "usage: %s <publisher|subscriber|recorder> [options]\n"
        "  --application NAME   supervisor application (default valley-drive)\n"
        "  --socket-dir DIR     POSIX supervisor socket directory (default /tmp)\n"
        "  --duration N         run seconds (default 30)\n"
        "  --trigger-after N    recorder trigger delay (default 5)\n"
        "  --storage DIR        recorder output directory\n",
        exe);
}

struct Options
{
    std::string role;
    std::string application = default_application();
    std::string socket_dir = "/tmp";
    std::string storage = ".";
    int duration_sec = 30;
    int trigger_after_sec = 5;
};

Options parse_args(int argc, char** argv)
{
    if (argc < 2) {
        usage(argv[0]);
        std::exit(2);
    }
    Options options;
    options.role = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto value = [&](const char* name) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing %s\n", name);
                std::exit(2);
            }
            return std::string{argv[++i]};
        };
        if (arg == "--application")
            options.application = value("--application");
        else if (arg == "--socket-dir")
            options.socket_dir = value("--socket-dir");
        else if (arg == "--duration")
            options.duration_sec = std::stoi(value("--duration"));
        else if (arg == "--trigger-after")
            options.trigger_after_sec = std::stoi(value("--trigger-after"));
        else if (arg == "--storage")
            options.storage = value("--storage");
        else if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            std::exit(0);
        } else {
            std::fprintf(stderr, "unknown arg: %s\n", arg.c_str());
            std::exit(2);
        }
    }
    return options;
}

SensorMessageHeader make_header(SensorType type, std::uint32_t sequence,
                                const char* device, std::uint32_t payload_size)
{
    SensorMessageHeader header;
    header.type = type;
    header.sequence = sequence;
    header.payload_size = payload_size;
    header.timestamp = sensor_sim::Timestamp::now();
    std::snprintf(header.device_name, sizeof header.device_name, "%s", device);
    return header;
}

bool publish_payload(exec::broker::Publisher& publisher, SensorType type,
                     std::uint32_t sequence, const char* device,
                     const void* body, std::size_t body_size)
{
    auto chunk = publisher.acquire_chunk();
    if (!chunk)
        return false;
    const std::size_t total = sizeof(SensorMessageHeader) + body_size;
    if (total > chunk->capacity())
        return false;

    const auto header = make_header(
        type, sequence, device, static_cast<std::uint32_t>(body_size));
    auto* destination = static_cast<std::uint8_t*>(chunk->data());
    std::memcpy(destination, &header, sizeof header);
    std::memcpy(destination + sizeof header, body, body_size);
    chunk->set_size(total);
    chunk->set_publish_time_ns(now_ns());
    return publisher.publish(std::move(*chunk));
}

void run_publisher(exec::broker::Broker& broker, const Options& options)
{
    auto camera_front = broker.acquire_publisher("/sensor/camera_front");
    auto camera_near = broker.acquire_publisher("/sensor/camera_near");
    auto camera_left = broker.acquire_publisher("/sensor/camera_left");
    auto camera_right = broker.acquire_publisher("/sensor/camera_right");
    auto imu = broker.acquire_publisher("/sensor/imu");
    auto lidar = broker.acquire_publisher("/sensor/lidar");
    if (!camera_front || !camera_near || !camera_left || !camera_right ||
        !imu || !lidar) {
        std::fprintf(stderr, "publisher attach failed\n");
        return;
    }

    std::atomic<std::uint32_t> camera_sequence{0};
    std::atomic<std::uint32_t> imu_sequence{0};
    std::atomic<std::uint32_t> lidar_sequence{0};

    auto camera = [&](exec::broker::Publisher& publisher, const char* name) {
        std::vector<std::uint8_t> pixels(
            CAMERA_WIDTH * CAMERA_HEIGHT * 3, 0);
        for (std::size_t i = 0; i < pixels.size(); ++i)
            pixels[i] = static_cast<std::uint8_t>(i & 0xFF);
        CameraImage image;
        image.timestamp = sensor_sim::Timestamp::now();
        std::snprintf(image.frame_id, sizeof image.frame_id, "%s", name);
        image.width = static_cast<std::uint32_t>(CAMERA_WIDTH);
        image.height = static_cast<std::uint32_t>(CAMERA_HEIGHT);
        std::snprintf(image.encoding, sizeof image.encoding, "rgb8");
        image.step = static_cast<std::uint32_t>(CAMERA_WIDTH * 3);

        std::vector<std::uint8_t> body(sizeof image + pixels.size());
        std::memcpy(body.data(), &image, sizeof image);
        std::memcpy(body.data() + sizeof image, pixels.data(), pixels.size());
        (void)publish_payload(publisher, SensorType::kCamera,
                              camera_sequence.fetch_add(1),
                              name, body.data(), body.size());
    };

    sensor_sim::CameraDevice camera_front_device("camera front",
                                                 "/sensor/camera_front");
    sensor_sim::CameraDevice camera_near_device("camera near",
                                                "/sensor/camera_near");
    sensor_sim::CameraDevice camera_left_device("camera left",
                                                "/sensor/camera_left");
    sensor_sim::CameraDevice camera_right_device("camera right",
                                                 "/sensor/camera_right");
    camera_front_device.set_data_callback(
        [&](const CameraImage&, const std::vector<std::uint8_t>&, std::uint32_t) {
            camera(*camera_front, "camera_front");
        });
    camera_near_device.set_data_callback(
        [&](const CameraImage&, const std::vector<std::uint8_t>&, std::uint32_t) {
            camera(*camera_near, "camera_near");
        });
    camera_left_device.set_data_callback(
        [&](const CameraImage&, const std::vector<std::uint8_t>&, std::uint32_t) {
            camera(*camera_left, "camera_left");
        });
    camera_right_device.set_data_callback(
        [&](const CameraImage&, const std::vector<std::uint8_t>&, std::uint32_t) {
            camera(*camera_right, "camera_right");
        });

    sensor_sim::ImuDevice imu_device("imu", "/sensor/imu");
    imu_device.set_data_callback(
        [&](const ImuData& data, std::uint32_t) {
            (void)publish_payload(*imu, SensorType::kImu,
                                  imu_sequence.fetch_add(1), "imu",
                                  &data, sizeof data);
        });

    sensor_sim::LidarDevice lidar_device("lidar", "/sensor/lidar");
    lidar_device.set_data_callback(
        [&](const LaserScan& scan, const std::vector<double>& ranges,
            const std::vector<double>& intensities, std::uint32_t) {
            std::vector<double> values;
            values.reserve(ranges.size() + intensities.size());
            values.insert(values.end(), ranges.begin(), ranges.end());
            values.insert(values.end(), intensities.begin(), intensities.end());

            std::vector<std::uint8_t> body(
                sizeof scan + values.size() * sizeof(double));
            std::memcpy(body.data(), &scan, sizeof scan);
            std::memcpy(body.data() + sizeof scan, values.data(),
                        values.size() * sizeof(double));
            (void)publish_payload(*lidar, SensorType::kLidar,
                                  lidar_sequence.fetch_add(1), "lidar",
                                  body.data(), body.size());
        });

    camera_front_device.start(10);
    camera_near_device.start(10);
    camera_left_device.start(10);
    camera_right_device.start(10);
    imu_device.start(100);
    lidar_device.start(10);
    std::this_thread::sleep_for(std::chrono::seconds(options.duration_sec));
    camera_front_device.stop();
    camera_near_device.stop();
    camera_left_device.stop();
    camera_right_device.stop();
    imu_device.stop();
    lidar_device.stop();
}

void run_subscriber(exec::broker::Broker& broker, const Options& options)
{
    std::vector<std::shared_ptr<exec::broker::Subscriber>> subscribers;
    std::vector<std::atomic<std::uint64_t>> counts(TOPICS.size());
    for (std::size_t i = 0; i < TOPICS.size(); ++i) {
        auto subscriber = broker.acquire_subscriber(TOPICS[i]);
        if (!subscriber) {
            std::fprintf(stderr, "subscriber attach failed: %s\n",
                         TOPICS[i].c_str());
            return;
        }
        subscriber->set_handler(
            [&counts, i](const exec::broker::Message_view& message) {
                if (message.size < sizeof(SensorMessageHeader))
                    return;
                counts[i].fetch_add(1, std::memory_order_relaxed);
            });
        subscriber->start();
        subscribers.push_back(std::move(subscriber));
    }

    std::this_thread::sleep_for(std::chrono::seconds(options.duration_sec));
    for (auto& subscriber : subscribers)
        subscriber->stop();
    for (std::size_t i = 0; i < TOPICS.size(); ++i)
        std::printf("subscriber %s received=%llu\n", TOPICS[i].c_str(),
                    static_cast<unsigned long long>(
                        counts[i].load(std::memory_order_relaxed)));
}

void run_recorder(exec::broker::Broker& broker, const Options& options)
{
    data::Recorder_config config;
    config.name = "valley-drive";

    data::Bundle_config bundle;
    bundle.name = "sensor";
    bundle.topics = TOPICS;
    bundle.max_file_size = 512 * 1024 * 1024;
    bundle.write_mode = data::Write_mode::ASYNC;
    bundle.max_queue_bytes = 256 * 1024 * 1024;
    config.bundles = {bundle};

    data::Recorder_options recorder_options;
    recorder_options.storage_directory = options.storage;
    recorder_options.poll_interval = std::chrono::milliseconds{20};

    auto recorder = data::Recorder::create(config, recorder_options);
    if (!recorder) {
        std::fprintf(stderr, "recorder create failed\n");
        return;
    }

    for (const auto& topic : TOPICS) {
        auto subscriber = broker.acquire_subscriber_for_pull(topic);
        if (!subscriber) {
            std::fprintf(stderr, "recorder pull attach failed: %s\n",
                         topic.c_str());
            return;
        }
        data::Subscriber_topic subscriber_topic;
        subscriber_topic.name = topic;
        subscriber_topic.subscriber = std::move(subscriber);
        if (!recorder.add_topic(std::move(subscriber_topic))) {
            std::fprintf(stderr, "recorder add_topic failed: %s\n",
                         topic.c_str());
            return;
        }
    }

    recorder.start();
    std::this_thread::sleep_for(
        std::chrono::seconds(options.trigger_after_sec));

    auto trigger = data::Trigger::make_unique(
        "drive-event", "valley-drive",
        std::chrono::milliseconds(now_ms()), 3, 5);
    trigger->topic = TOPICS;
    if (!recorder.record(std::move(trigger)))
        std::fprintf(stderr, "trigger rejected\n");

    std::this_thread::sleep_for(std::chrono::seconds(
        options.duration_sec > options.trigger_after_sec
            ? options.duration_sec - options.trigger_after_sec
            : 0));
    recorder.stop();

    for (const auto& metrics : recorder.metrics()) {
        std::printf(
            "recorder %s/%s mode=%s written=%llu dropped=%llu "
            "segments=%llu pre_roll_truncated=%llu\n",
            config.name.c_str(), metrics.name.c_str(),
            metrics.mode == data::Write_mode::ASYNC ? "async" : "sync",
            static_cast<unsigned long long>(metrics.written),
            static_cast<unsigned long long>(metrics.dropped),
            static_cast<unsigned long long>(metrics.segment_count),
            static_cast<unsigned long long>(metrics.pre_roll_truncated));
    }
}

}  // namespace

int main(int argc, char** argv)
{
    const Options options = parse_args(argc, argv);
    if (options.role != "publisher" && options.role != "subscriber" &&
        options.role != "recorder") {
        std::fprintf(stderr, "unknown role: %s\n", options.role.c_str());
        return 2;
    }
    if (options.application.empty()) {
        std::fprintf(stderr, "application must not be empty\n");
        return 2;
    }

    exec::broker::Broker_options broker_options;
    broker_options.client_name = options.role == "subscriber"
                                     ? "realtime"
                                     : (options.role == "recorder" ? "recorder"
                                                                   : "publisher");
    // 和 supervisor 一样：地址由 application 决定。
    const std::string address = serve::Address::make_ipc_address(
        options.application, options.socket_dir);
    broker_options.addresses = {address};
    auto& broker = exec::broker::Broker::instance();
    if (!broker.start(broker_options)) {
        std::fprintf(stderr,
                     "broker start failed: application=%s socket_dir=%s "
                     "address=%s\n",
                     options.application.c_str(), options.socket_dir.c_str(),
                     address.c_str());
        return 1;
    }

    if (options.role == "publisher")
        run_publisher(broker, options);
    else if (options.role == "subscriber")
        run_subscriber(broker, options);
    else
        run_recorder(broker, options);

    broker.stop();
    return 0;
}
