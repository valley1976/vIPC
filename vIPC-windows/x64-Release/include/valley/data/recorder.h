#pragma once

#include <cstddef>
#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "trigger.h"

#include "valley/base/lang/static_string.h"
#include "valley/data/export.h"

namespace valley {
namespace data {
namespace internal {
     class Recorder;

     using New_file = std::function<void(const std::string& /*recorder*/, const std::string& /*bundle*/, size_t /*sequence*/, base::SStr<256>&/*new file name*/)>;
     using Close_file  = std::function<void(const base::SStr<256>&/*saved file name*/)>;
}

class LIBVALLEY_DATA_EXPORT Recorder
{
public:
    Recorder();
    ~Recorder();

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    Recorder(Recorder&& orig) noexcept;
    Recorder& operator=(Recorder&& orig) noexcept;

    static Recorder create(const std::string& name, const internal::New_file& new_fn = {}, const internal::Close_file& close_fn = {});

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    void record(Trigger::Ptr&& trigger);

    void start();
    void stop();

private:
    std::unique_ptr<internal::Recorder> impl_;
};

}
}