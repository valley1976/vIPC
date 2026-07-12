#include "sensor_sim/process_monitor.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")
#endif

#include <sstream>
#include <algorithm>

namespace {

#ifdef _WIN32
uint64_t get_cpu_time(HANDLE process) {
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (!GetProcessTimes(process, &creation_time, &exit_time, &kernel_time, &user_time)) {
        return 0;
    }
    ULARGE_INTEGER kernel, user;
    kernel.LowPart = kernel_time.dwLowDateTime;
    kernel.HighPart = kernel_time.dwHighDateTime;
    user.LowPart = user_time.dwLowDateTime;
    user.HighPart = user_time.dwHighDateTime;
    return kernel.QuadPart + user.QuadPart;
}

uint64_t get_total_cpu_time() {
    FILETIME idle_time, kernel_time, user_time;
    if (!GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        return 0;
    }
    ULARGE_INTEGER kernel, user;
    kernel.LowPart = kernel_time.dwLowDateTime;
    kernel.HighPart = kernel_time.dwHighDateTime;
    user.LowPart = user_time.dwLowDateTime;
    user.HighPart = user_time.dwHighDateTime;
    return kernel.QuadPart + user.QuadPart;
}

uint64_t find_pid_by_name(const std::string& name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot, &pe)) {
        CloseHandle(snapshot);
        return 0;
    }

    uint64_t found_pid = 0;
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    do {
        std::string exe_name(pe.szExeFile);
        std::transform(exe_name.begin(), exe_name.end(), exe_name.begin(), ::tolower);
        if (exe_name.find(lower_name) != std::string::npos) {
            found_pid = pe.th32ProcessID;
            break;
        }
    } while (Process32Next(snapshot, &pe));

    CloseHandle(snapshot);
    return found_pid;
}

void get_io_counters(HANDLE process, uint64_t& read_bytes, uint64_t& write_bytes) {
    IO_COUNTERS counters;
    if (GetProcessIoCounters(process, &counters)) {
        read_bytes = counters.ReadTransferCount;
        write_bytes = counters.WriteTransferCount;
    } else {
        read_bytes = 0;
        write_bytes = 0;
    }
}

std::string get_process_name_by_pid(uint64_t pid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return "";
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot, &pe)) {
        CloseHandle(snapshot);
        return "";
    }

    std::string name;
    do {
        if (pe.th32ProcessID == pid) {
            name = pe.szExeFile;
            break;
        }
    } while (Process32Next(snapshot, &pe));

    CloseHandle(snapshot);
    return name;
}
#endif

}

namespace sensor_sim {

struct ProcessMonitor::Impl {
    uint64_t pid = 0;
    std::string process_name;
    ProcessStats stats{};

#ifdef _WIN32
    HANDLE process_handle = nullptr;
    uint64_t last_cpu_time = 0;
    uint64_t last_system_cpu_time = 0;
#endif

    ~Impl() {
#ifdef _WIN32
        if (process_handle != nullptr) {
            CloseHandle(process_handle);
        }
#endif
    }

    bool open_process(uint64_t target_pid) {
#ifdef _WIN32
        if (process_handle != nullptr) {
            CloseHandle(process_handle);
        }
        process_handle = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_SET_QUOTA,
            FALSE,
            static_cast<DWORD>(target_pid)
        );
        return process_handle != nullptr;
#endif
        return false;
    }

    bool update_stats() {
#ifdef _WIN32
        if (process_handle == nullptr) {
            stats.running = false;
            return false;
        }

        DWORD exit_code;
        if (!GetExitCodeProcess(process_handle, &exit_code)) {
            stats.running = false;
            return false;
        }

        if (exit_code != STILL_ACTIVE) {
            stats.running = false;
            return false;
        }
        stats.running = true;

        stats.pid = pid;
        stats.name = process_name.empty() ? get_process_name_by_pid(pid) : process_name;

        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(process_handle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
            stats.mem_usage_bytes = pmc.PrivateUsage;
        } else {
            stats.mem_usage_bytes = 0;
        }

        uint64_t current_cpu_time = get_cpu_time(process_handle);
        uint64_t current_system_cpu_time = get_total_cpu_time();

        if (last_cpu_time > 0 && last_system_cpu_time > 0) {
            uint64_t cpu_diff = current_cpu_time - last_cpu_time;
            uint64_t system_diff = current_system_cpu_time - last_system_cpu_time;

            SYSTEM_INFO sys_info;
            GetSystemInfo(&sys_info);
            int num_cpus = sys_info.dwNumberOfProcessors;

            if (system_diff > 0) {
                stats.cpu_usage = (static_cast<double>(cpu_diff) / static_cast<double>(system_diff)) * num_cpus * 100.0;
            } else {
                stats.cpu_usage = 0.0;
            }
        }

        last_cpu_time = current_cpu_time;
        last_system_cpu_time = current_system_cpu_time;

        get_io_counters(process_handle, stats.io_read_bytes, stats.io_write_bytes);

        return true;
#endif
        return false;
    }
};

ProcessMonitor::ProcessMonitor() : impl_(std::make_unique<Impl>()) {}

ProcessMonitor::~ProcessMonitor() = default;

bool ProcessMonitor::attach(uint64_t pid) {
    impl_->pid = pid;
    impl_->process_name.clear();
    if (impl_->open_process(pid)) {
        impl_->stats.pid = pid;
        impl_->stats.name = get_process_name_by_pid(pid);
        return true;
    }
    return false;
}

bool ProcessMonitor::attach(const std::string& process_name) {
    uint64_t pid = find_pid_by_name(process_name);
    if (pid == 0) {
        return false;
    }
    impl_->process_name = process_name;
    impl_->stats.name = process_name;
    return attach(pid);
}

void ProcessMonitor::detach() {
    impl_->pid = 0;
    impl_->process_name.clear();
    impl_->stats = ProcessStats{};
}

bool ProcessMonitor::is_attached() const {
    return impl_->pid != 0;
}

bool ProcessMonitor::update() {
    return impl_->update_stats();
}

const ProcessStats& ProcessMonitor::stats() const {
    return impl_->stats;
}

uint64_t ProcessMonitor::pid() const {
    return impl_->pid;
}

const std::string& ProcessMonitor::process_name() const {
    return impl_->process_name;
}

}
