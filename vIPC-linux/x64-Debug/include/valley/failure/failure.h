#pragma once

#include <memory>
#include <string>

#include "valley/failure/export.h"

namespace valley {
namespace failure {

class LIBVALLEY_FAILURE_EXPORT Failure
{
public:
    ~Failure();

    Failure(const Failure&) = delete;
    Failure& operator=(const Failure&) = delete;

    static Failure& instance();

    void setup_process(const std::string& process_name, const std::string& dump_path = "coredump");
    void dump();

private:
    Failure();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}