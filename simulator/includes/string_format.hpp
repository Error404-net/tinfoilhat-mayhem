#pragma once
#include <cstdint>
#include <string>

// Mayhem string formatting helpers — thin wrappers around std::to_string.
inline std::string to_string_dec_int(int32_t v, int width = 0, char fill = ' ') {
    std::string s = std::to_string(v);
    while ((int)s.size() < width) s = fill + s;
    return s;
}

inline std::string to_string_dec_uint(uint32_t v, int width = 0, char fill = ' ') {
    std::string s = std::to_string(v);
    while ((int)s.size() < width) s = fill + s;
    return s;
}

// rtc::RTC is declared in rtc_time.hpp; forward the type here for to_string_datetime.
namespace rtc { struct RTC; }
std::string to_string_datetime(const rtc::RTC& t);
