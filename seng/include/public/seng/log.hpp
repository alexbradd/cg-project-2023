#pragma once

#include <fmt/core.h>

#include <iostream>
#include <string>

namespace seng::log {

#define INFO_STR "[INFO] "
#define WARN_STR "[WARN] "
#define ERRO_STR "[ERRO] "
#define DBUG_STR "[DBUG] "

/**
 * Print a debug message to stderr. If built in release mode function is a noop.
 * Supports fmt-style format arguments.
 */
template <typename... Args>
void dbg(const std::string &fmt, Args &&...args) {
#ifndef NDEBUG
  std::cerr << DBUG_STR << fmt::format(fmt, args...) << std::endl;
#endif
}

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
