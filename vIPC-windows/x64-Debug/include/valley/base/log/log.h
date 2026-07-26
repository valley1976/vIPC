#pragma once

#include "context.h"
#include "logger.h"

#define VALLEY_LOG(severity) valley::base::Use_logger<static_cast<int>(severity)>::type(severity, __LINE__, __FILE__, __func__)

// for operator <<
#define vTRACE() VALLEY_LOG(valley::base::Log_severity::kTrace)
#define vDEBUG() VALLEY_LOG(valley::base::Log_severity::kDebug)
#define vINFO()  VALLEY_LOG(valley::base::Log_severity::kInfo)
#define vWARN()  VALLEY_LOG(valley::base::Log_severity::kWarn)
#define vERROR() VALLEY_LOG(valley::base::Log_severity::kError)
#define vFATAL() VALLEY_LOG(valley::base::Log_severity::kFatal)

// for printf
#define vTRACE_PRT(...) VALLEY_LOG(valley::base::Log_severity::kTrace).printf(__VA_ARGS__)
#define vDEBUG_PRT(...) VALLEY_LOG(valley::base::Log_severity::kDebug).printf(__VA_ARGS__)
#define vINFO_PRT(...)  VALLEY_LOG(valley::base::Log_severity::kInfo).printf(__VA_ARGS__)
#define vWARN_PRT(...)  VALLEY_LOG(valley::base::Log_severity::kWarn).printf(__VA_ARGS__)
#define vERROR_PRT(...) VALLEY_LOG(valley::base::Log_severity::kError).printf(__VA_ARGS__)
#define vFATAL_PRT(...) VALLEY_LOG(valley::base::Log_severity::kFatal).printf(__VA_ARGS__)

// for format
#ifdef vENABLE_LOG_FMT
  #define vTRACE_FMT(...)  VALLEY_LOG(valley::base::Log_severity::kTrace).format(__VA_ARGS__)
  #define vDEBUG_FMT(...)  VALLEY_LOG(valley::base::Log_severity::kDebug).format(__VA_ARGS__)
  #define vINFO_FMT(...)   VALLEY_LOG(valley::base::Log_severity::kInfo).format(__VA_ARGS__)
  #define vWARN_FMT(...)   VALLEY_LOG(valley::base::Log_severity::kWarn).format(__VA_ARGS__)
  #define vERROR_FMT(...)  VALLEY_LOG(valley::base::Log_severity::kError).format(__VA_ARGS__)
  #define vFATAL_FMT(...)  VALLEY_LOG(valley::base::Log_severity::kFatal).format(__VA_ARGS__)
#endif // vENABLE_LOG_FMT

namespace valley {
namespace base {

template<Log_severity severity = Log_severity::kInfo>
struct Trace_scope
{
    const char* const scope;

    explicit Trace_scope(const char* scope) : scope(scope)
    {
        VALLEY_LOG(severity).printf("Enter: %s", scope);
    }

    ~Trace_scope()
    {
        VALLEY_LOG(severity).printf("Exit : %s", scope);
    }
};

}
}