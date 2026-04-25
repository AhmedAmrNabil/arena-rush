#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>

#define LOG_COLOR_RESET "\033[0m"
#define LOG_COLOR_RED "\033[1;31m"
#define LOG_COLOR_YELLOW "\033[1;33m"
#define LOG_COLOR_BLUE "\033[1;34m"
#define LOG_COLOR_CYAN "\033[0;36m"

namespace our::log {

    inline void log_impl(std::ostream& out, const char* color, std::string_view module, const char* fmt, ...) {
        out << color << '[' << module << ']' << LOG_COLOR_RESET << ": ";

        va_list args;
        va_start(args, fmt);
        // vfprintf directly to the underlying stream buffer — no heap alloc
        if (&out == &std::cout)
            std::vfprintf(stdout, fmt, args);
        else if (&out == &std::cerr)
            std::vfprintf(stderr, fmt, args);
        va_end(args);

        out << '\n';
    }

}  // namespace our::log

#define LOG_ERROR(mod, fmt, ...) our::log::log_impl(std::cerr, LOG_COLOR_RED, mod, fmt, ##__VA_ARGS__)
#define LOG_WARN(mod, fmt, ...) our::log::log_impl(std::cerr, LOG_COLOR_YELLOW, mod, fmt, ##__VA_ARGS__)
#define LOG_INFO(mod, fmt, ...) our::log::log_impl(std::cout, LOG_COLOR_BLUE, mod, fmt, ##__VA_ARGS__)

#ifndef NDEBUG
#define LOG_DEBUG(mod, fmt, ...) our::log::log_impl(std::cout, LOG_COLOR_CYAN, mod, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(...) \
    do {               \
    } while (0)
#endif
