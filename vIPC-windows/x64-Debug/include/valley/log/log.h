#pragma once

#include "context.h"
#include "logger.h"

#define VALLEY_LOG(level) if (valley::log::Context::instance().should_log(level)) \
                            valley::log::Use_logger<int(level)>::type(level, __LINE__, __FILE__, __func__)

// for operator <<
#define vTRACE() VALLEY_LOG(valley::log::Level::kTrace)
#define vDEBUG() VALLEY_LOG(valley::log::Level::kDebug)
#define vINFO()  VALLEY_LOG(valley::log::Level::kInfo)
#define vWARN()  VALLEY_LOG(valley::log::Level::kWarn)
#define vERROR() VALLEY_LOG(valley::log::Level::kError)
#define vFATAL() VALLEY_LOG(valley::log::Level::kFatal)

// for printf
#define vTRACE_PRT(...) VALLEY_LOG(valley::log::Level::kTrace).printf(__VA_ARGS__)
#define vDEBUG_PRT(...) VALLEY_LOG(valley::log::Level::kDebug).printf(__VA_ARGS__)
#define vINFO_PRT(...)  VALLEY_LOG(valley::log::Level::kInfo).printf(__VA_ARGS__)
#define vWARN_PRT(...)  VALLEY_LOG(valley::log::Level::kWarn).printf(__VA_ARGS__)
#define vERROR_PRT(...) VALLEY_LOG(valley::log::Level::kError).printf(__VA_ARGS__)
#define vFATAL_PRT(...) VALLEY_LOG(valley::log::Level::kFatal).printf(__VA_ARGS__)

// for format
#ifdef vENABLE_LOG_FMT
  #define vTRACE_FMT(...)  VALLEY_LOG(valley::log::Level::kTrace).format(__VA_ARGS__)
  #define vDEBUG_FMT(...)  VALLEY_LOG(valley::log::Level::kDebug).format(__VA_ARGS__)
  #define vINFO_FMT(...)   VALLEY_LOG(valley::log::Level::kInfo).format(__VA_ARGS__)
  #define vWARN_FMT(...)   VALLEY_LOG(valley::log::Level::kWarn).format(__VA_ARGS__)
  #define vERROR_FMT(...)  VALLEY_LOG(valley::log::Level::kError).format(__VA_ARGS__)
  #define vFATALT_FMT(...) VALLEY_LOG(valley::log::Level::kFatal).format(__VA_ARGS__)
#endif // vENABLE_LOG_FMT

