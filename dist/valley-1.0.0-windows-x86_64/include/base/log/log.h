// Copyright (c) 2026 shangu.
// Developed with DSH (DeepSeek Harness).

#pragma once

#include "context.h"
#include "logger.h"

#define VALLEY_LOG_TAG(severity, tag) base::Use_logger<static_cast<int>(severity)>::type(severity, __LINE__, __FILE__, __func__, tag)

// 所有流式入口必须传入 TAG。
#define vTRACE(tag) VALLEY_LOG_TAG(base::Log_severity::TRACE, #tag)
#define vDEBUG(tag) VALLEY_LOG_TAG(base::Log_severity::DEBUG, #tag)
#define vINFO(tag)  VALLEY_LOG_TAG(base::Log_severity::INFO, #tag)
#define vWARN(tag)  VALLEY_LOG_TAG(base::Log_severity::WARN, #tag)
#define vERROR(tag) VALLEY_LOG_TAG(base::Log_severity::ERROR, #tag)
#define vFATAL(tag) VALLEY_LOG_TAG(base::Log_severity::FATAL, #tag)

// for printf
#define vTRACE_PRT(tag, ...) VALLEY_LOG_TAG(base::Log_severity::TRACE, #tag).printf(__VA_ARGS__)
#define vDEBUG_PRT(tag, ...) VALLEY_LOG_TAG(base::Log_severity::DEBUG, #tag).printf(__VA_ARGS__)
#define vINFO_PRT(tag, ...)  VALLEY_LOG_TAG(base::Log_severity::INFO, #tag).printf(__VA_ARGS__)
#define vWARN_PRT(tag, ...)  VALLEY_LOG_TAG(base::Log_severity::WARN, #tag).printf(__VA_ARGS__)
#define vERROR_PRT(tag, ...) VALLEY_LOG_TAG(base::Log_severity::ERROR, #tag).printf(__VA_ARGS__)
#define vFATAL_PRT(tag, ...) VALLEY_LOG_TAG(base::Log_severity::FATAL, #tag).printf(__VA_ARGS__)

namespace base {

template<Log_severity severity = Log_severity::INFO>
struct Trace_scope
{
    const char* const tag;
    const char* const scope;

    Trace_scope(const char* tag, const char* scope)
        : tag{tag}, scope{scope}
    {
        VALLEY_LOG_TAG(severity, tag).printf("Enter: %s", scope);
    }

    ~Trace_scope()
    {
        VALLEY_LOG_TAG(severity, tag).printf("Exit : %s", scope);
    }
};

}
