#pragma once

#include <cassert>
#include <string>
#include <system_error>
#include <functional>

#include "valley/base/errors/expected.h"
#include "valley/base/lang/optional.h"

#include "error.h"

namespace valley {
namespace serve {
// ---------------------------------------------------------------------------
// Error type
// ---------------------------------------------------------------------------

struct Error {
    Status code = Status::kOk;
    std::string message = {};

    Error() = default;
    Error(Status c, const char* m) : code(c), message(m) {}
    Error(Status c, const std::string& m) : code(c), message(m) {}

    static Error ok() { return Error{ Status::kOk, "" }; }

    static Error timeout(const std::string& msg = "timeout") {
        return Error{ Status::kDeadlineExceeded, msg };
    }
    static Error io(const std::string& msg = "io error") {
        return Error{ Status::kIO, msg };
    }
    static Error connection_closed(const std::string& msg = "connection closed") {
        return Error{ Status::kConnectionClosed, msg };
    }
    static Error invalid_argument(const std::string& msg = "invalid argument") {
        return Error{ Status::kInvalidArgument, msg };
    }
    static Error not_found(const std::string& msg = "not found") {
        return Error{ Status::kNotFound, msg };
    }
    static Error cancelled(const std::string& msg = "cancelled") {
        return Error{ Status::kCancelled, msg };
    }
    static Error max_concurrent(const std::string& msg = "max concurrent requests") {
        return Error{ Status::kMaxConcurrent, msg };
    }
    static Error protocol(const std::string& msg = "protocol error") {
        return { Status::kProtocol, msg };
    }
    static Error handler_error(const std::string& msg = "handler error") {
        return Error{ Status::kHandlerError, msg };
    }
    static Error not_implemented(const std::string& msg = "not implemented") {
        return Error{ Status::kUnimplemented, msg };
    }
    static Error reconnect_failed(const std::string& msg = "reconnect failed") {
        return Error{ Status::kReconnectFailed, msg };
    }

    bool is_ok() const { return code == Status::kOk; }
    bool is_err() const { return code != Status::kOk; }
    explicit operator bool() const { return is_ok(); }
};

// ---------------------------------------------------------------------------
// Result<T> — lightweight alternative to std::expected (C++23)
// ---------------------------------------------------------------------------

template <typename T>
class Result {
    base::Expected<T, Error> data_;
public:
    Result(T v) : data_(std::move(v)) {}          // NOLINT
    Result(Error e) : data_(base::make_unexpected(e)) {}      // NOLINT

    bool is_ok()  const { return data_.has_value(); }
    bool is_err() const { return !data_.has_value(); }

    T& value() { assert(is_ok());  return data_.value(); }
    const T& value() const { assert(is_ok());  return data_.value(); }

    T&& move_value() { assert(is_ok()); return std::move(data_.value()); }

    Error& error() { assert(is_err()); return data_.error(); }
    const Error& error() const { assert(is_err()); return data_.error(); }
};

template <>
class Result<void> {
    base::Optional<Error> error_;
public:
    Result() = default;                                    // NOLINT
    Result(Error e) : error_(std::move(e)) {}              // NOLINT

    bool is_ok()  const { return !error_.has_value(); }
    bool is_err() const { return  error_.has_value(); }

    Error& error() { assert(is_err()); return *error_; }
    const Error& error() const { assert(is_err()); return *error_; }
};

}
}