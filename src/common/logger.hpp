#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>

// ─────────────────────────────────────────────────────────────────────────────
//  ANSI color codes
// ─────────────────────────────────────────────────────────────────────────────
#define LOG_COLOR_RESET "\033[0m"
#define LOG_COLOR_RED "\033[1;31m"     // bold red    – errors
#define LOG_COLOR_YELLOW "\033[1;33m"  // bold yellow – warnings
#define LOG_COLOR_BLUE "\033[1;34m"    // bold blue   – info
#define LOG_COLOR_CYAN "\033[0;36m"    // cyan        – debug

// ─────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace our::log::detail {

    // printf-style formatter → std::string
    [[nodiscard]] inline std::string format(const char* fmt, ...) {
        va_list args;

        // First pass: measure required buffer size
        va_start(args, fmt);
        int size = std::vsnprintf(nullptr, 0, fmt, args);
        va_end(args);

        if (size <= 0) return {};

        std::string buf(static_cast<std::size_t>(size + 1), '\0');

        // Second pass: fill the buffer
        va_start(args, fmt);
        std::vsnprintf(buf.data(), buf.size(), fmt, args);
        va_end(args);

        buf.resize(static_cast<std::size_t>(size));  // drop the null terminator
        return buf;
    }

    // Core printer:  "[MODULE]: <sub: > msg\n"
    inline void print(const char* levelColor, std::string_view module, std::string_view sub, std::string_view msg,
                      std::ostream& out) {
        out << levelColor << '[' << module << ']' << LOG_COLOR_RESET << ": ";
        if (!sub.empty()) out << levelColor << sub << LOG_COLOR_RESET << ": ";
        out << msg << '\n';
    }

}  // namespace our::log::detail

// ─────────────────────────────────────────────────────────────────────────────
//  Internal dispatch macros
//
//  All LOG_* macros accept either:
//    (module, fmt, ...)              – no sub-module
//    (module, sub, fmt, ...)         – with sub-module
//
//  The trick: we always pass a sub argument;  when the caller provides only
//  (module, fmt, ...) we inject "" as sub automatically via _LOG_CALL.
// ─────────────────────────────────────────────────────────────────────────────

// Print helper that calls format() then print()
// Usage: _LOG_DO(color, stream, module, sub, fmt, ...)
#define _LOG_DO(color, stream, mod, sub, ...) \
    our::log::detail::print(color, (mod), (sub), our::log::detail::format(__VA_ARGS__), stream)

// 5-argument public form:  LOG_*(mod, sub, fmt, ...)
#define _LOG_WITH_SUB(color, stream, mod, sub, ...) _LOG_DO(color, stream, mod, sub, __VA_ARGS__)

// 4-argument public form:  LOG_*(mod, fmt, ...)   → inject empty sub
#define _LOG_NO_SUB(color, stream, mod, ...) _LOG_DO(color, stream, mod, "", __VA_ARGS__)

// ─────────────────────────────────────────────────────────────────────────────
//  Public macros
//
//  LOG_ERROR(module,       fmt, ...)
//  LOG_ERROR(module, sub,  fmt, ...)
//  LOG_WARN (module,       fmt, ...)
//  LOG_WARN (module, sub,  fmt, ...)
//  LOG_INFO (module,       fmt, ...)
//  LOG_INFO (module, sub,  fmt, ...)
//  LOG_DEBUG(module,       fmt, ...)   ← compiled out in Release (NDEBUG)
//  LOG_DEBUG(module, sub,  fmt, ...)   ← compiled out in Release (NDEBUG)
// ─────────────────────────────────────────────────────────────────────────────

// ── ERROR ────────────────────────────────────────────────────────────────────
#define LOG_ERROR(mod, ...) _LOG_NO_SUB(LOG_COLOR_RED, std::cerr, mod, __VA_ARGS__)
#define LOG_ERRORS(mod, sub, ...) _LOG_WITH_SUB(LOG_COLOR_RED, std::cerr, mod, sub, __VA_ARGS__)

// ── WARN ─────────────────────────────────────────────────────────────────────
#define LOG_WARN(mod, ...) _LOG_NO_SUB(LOG_COLOR_YELLOW, std::cerr, mod, __VA_ARGS__)
#define LOG_WARNS(mod, sub, ...) _LOG_WITH_SUB(LOG_COLOR_YELLOW, std::cerr, mod, sub, __VA_ARGS__)

// ── INFO ─────────────────────────────────────────────────────────────────────
#define LOG_INFO(mod, ...) _LOG_NO_SUB(LOG_COLOR_BLUE, std::cout, mod, __VA_ARGS__)
#define LOG_INFOS(mod, sub, ...) _LOG_WITH_SUB(LOG_COLOR_BLUE, std::cout, mod, sub, __VA_ARGS__)

// ── DEBUG (disabled in Release via NDEBUG) ───────────────────────────────────
#ifndef NDEBUG
#define LOG_DEBUG(mod, ...) _LOG_NO_SUB(LOG_COLOR_CYAN, std::cout, mod, __VA_ARGS__)
#define LOG_DEBUGS(mod, sub, ...) _LOG_WITH_SUB(LOG_COLOR_CYAN, std::cout, mod, sub, __VA_ARGS__)
#else
#define LOG_DEBUG(...) \
    do {               \
    } while (0)
#define LOG_DEBUGS(...) \
    do {                \
    } while (0)
#endif
