#pragma once
#include <ctime>
#include <string>

namespace rtc {
struct RTC {
    int year{2026}, month{1}, day{1};
    int hour{0}, minute{0}, second{0};
};
}  // namespace rtc

namespace rtc_time {
inline rtc::RTC now() {
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);
    return {1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec};
}
}  // namespace rtc_time

// Defined here (not in string_format.hpp) to avoid include order issues.
inline std::string to_string_datetime(const rtc::RTC& t) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             t.year, t.month, t.day, t.hour, t.minute, t.second);
    return buf;
}
