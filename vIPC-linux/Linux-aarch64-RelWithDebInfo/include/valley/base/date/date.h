#pragma once

#ifdef _WIN32
  #undef min
  #undef max
  // min max 会影响 date.h的编译
#endif // _WIN32

#include "internal/date.h"

namespace valley {
namespace base {

using date::days;
using date::weeks;
using date::years;
using date::months;

using date::sys_days;
using date::sys_seconds;

using date::local_days;
using date::local_seconds;

using date::day;
using date::month;
using date::year;

using date::weekday;
using date::weekday_indexed;
using date::weekday_last;

using date::month_day;
using date::month_day_last;
using date::month_weekday;
using date::month_weekday_last;

using date::year_month;

using date::year_month_day;
using date::year_month_day_last;
using date::year_month_weekday;
using date::year_month_weekday_last;


}
}