#pragma once

#include <string>
#include <system_error>

namespace valley {
namespace serve {

enum class Status : int32_t
{
    kOk = 0,
    kCancelled = 1,
    kUnknown = 2,
    kInvalidArgument = 3,
    kDeadlineExceeded = 4,
    kNotFound = 5,
    kAlreadyExists = 6,
    kPermissionDenied = 7,
    kResourceExhausted = 8,
    kFailedPrecondition = 9,
    kAborted = 10,
    kOutOfRange = 11,
    kUnimplemented = 12,
    kInternal = 13,
    kUnavailable = 14,
    kDataLoss = 15,
    kUnauthenticated = 16,
    kIO = 17,
    kConnectionClosed = 18,
    kReconnectFailed = 19,
    kMaxConcurrent = 20,
    kProtocol = 21,
    kHandlerError = 22,

    kUser = 100,
};

std::error_code make_error_code(Status);

inline const char* status_to_string(Status status)
{
    switch (status) {
    case Status::kOk:
        return "OK";
    case Status::kCancelled:
        return "CANCELLED";
    case Status::kUnknown:
        return "UNKNOWN";
    case Status::kInvalidArgument:
        return "INVALID_ARGUMENT";
    case Status::kDeadlineExceeded:
        return "DEADLINE_EXCEEDED";
    case Status::kNotFound:
        return "NOT_FOUND";
    case Status::kAlreadyExists:
        return "ALREADY_EXISTS";
    case Status::kPermissionDenied:
        return "PERMISSION_DENIED";
    case Status::kUnauthenticated:
        return "UNAUTHENTICATED";
    case Status::kResourceExhausted:
        return "RESOURCE_EXHAUSTED";
    case Status::kFailedPrecondition:
        return "FAILED_PRECONDITION";
    case Status::kAborted:
        return "ABORTED";
    case Status::kOutOfRange:
        return "OUT_OF_RANGE";
    case Status::kUnimplemented:
        return "UNIMPLEMENTED";
    case Status::kInternal:
        return "INTERNAL";
    case Status::kUnavailable:
        return "UNAVAILABLE";
    case Status::kDataLoss:
        return "DATA_LOSS";
    case Status::kIO:
        return "IO";
    case Status::kConnectionClosed:
        return "CONNECTION_CLOSED";
    case Status::kReconnectFailed:
        return "RECONNECT_FAILED";
    case Status::kMaxConcurrent:
        return "MAX_CONCURRENT";
    case Status::kProtocol:
        return "PROTOCOL";
    case Status::kHandlerError:
        return "HANDLER_ERROR";
    default:
        return "UNKNOWN_STATUS";
    }
}

}
}

namespace std {
template<>
struct is_error_code_enum<valley::serve::Status> : true_type {};

}
