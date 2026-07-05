#pragma once

#include <functional>
#include <fstream>

#include "cpptrace/cpptrace.hpp"
#include "ghc/filesystem.hpp"

namespace valley {
namespace errors {
namespace internal {

namespace fs = ghc::filesystem;

class Failure
{
public:
    ~Failure();

    Failure(const Failure&) = delete;
    Failure& operator=(const Failure&) = delete;

    static Failure& instance();

    void setup_process(const std::string& process_name, const std::string& dump_path);

    void dump();

private:
    Failure() = default;

private:
    bool initialized_ = false;
    bool dump_file_ = false;
    std::string process_name_;
    fs::path dump_path_;    
    std::ofstream file_;
};

}
}
}