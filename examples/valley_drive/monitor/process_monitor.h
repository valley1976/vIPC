#pragma once

#include <string>
#include <memory>
#include <chrono>

namespace sensor_sim {

struct ProcessStats {
    uint64_t pid = 0;
    std::string name;
    double cpu_usage = 0.0;
    uint64_t mem_usage_bytes = 0;
    uint64_t io_read_bytes = 0;
    uint64_t io_write_bytes = 0;
    bool running = false;
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    ProcessMonitor(ProcessMonitor&&) noexcept = default;
    ProcessMonitor& operator=(ProcessMonitor&&) noexcept = default;
    
    ProcessMonitor(const ProcessMonitor&) = delete;
    ProcessMonitor& operator=(const ProcessMonitor&) = delete;
    
    bool attach(uint64_t pid);
    bool attach(const std::string& process_name);
    void detach();
    bool is_attached() const;
    bool update();
    const ProcessStats& stats() const;
    uint64_t pid() const;
    const std::string& process_name() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
