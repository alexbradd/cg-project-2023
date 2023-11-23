#ifndef __SENG_LOG_HPP__
#define __SENG_LOG_HPP__

#include <fmt/core.h>

#include <iostream>
#include <string>

namespace seng::log {

#define INFO_STR "[INFO] "
#define WARN_STR "[WARN] "
#define ERRO_STR "[ERRO] "

#ifndef NDEBUG
#define DBUG_STR "[DBUG] "

/**
 * Print a debug message to stderr. If built in release mode function is a noop.
 * Supports fmt-style format arguments.
 */
template <typename... Args>
void dbg(const std::string &fmt, Args &&...args) {
  std::cerr << DBUG_STR << __FILE__ << ":" << __LINE__ << " "
            << fmt::format(fmt, args...) << std::endl;
}
#else
/**
 * Print a debug message to stderr. If built in release mode function is a noop.
 * Supports fmt-style format arguments.
 */
template <typename... Args>
void dbg(const std::string &fmt, Args &&...args) {}
#endif

/**
 * Print a information message to stderr. Supports fmt-style format arguments.
 */
template <typename... Args>
void info(const std::string &fmt, Args &&...args) {
  std::cerr << INFO_STR << fmt::format(fmt, args...) << std::endl;
}

/**
 * Print a warning message to stderr. Supports fmt-style format arguments.
 */
template <typename... Args>
void warning(const std::string &fmt, Args &&...args) {
  std::cerr << WARN_STR << fmt::format(fmt, args...) << std::endl;
}

/**
 * Print an error message to stderr. Supports fmt-style format arguments.
 */
template <typename... Args>
void error(const std::string &fmt, Args &&...args) {
  std::cerr << ERRO_STR << fmt::format(fmt, args...) << std::endl;
}

}  // namespace seng::log

#endif  // !__SENG_LOG_HPP__
