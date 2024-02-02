#pragma once

#include <fmt/core.h>

#include <string>

namespace seng::log {

/**
 * Log levels
 */
enum struct LogLevels : uint32_t {
  DBUG = 0x00000001,
  INFO = 0x00000002,
  WARN = 0x00000004,
  ERRO = 0x00000008
};

/**
 * Writes the string out to stderr formatting it with the proper previx given
 * its log leve
 */
extern void logOutput(LogLevels lvl, std::string out);

/**
 * Print a debug message to stderr. If built in release mode function is a
 * noop. Supports fmt-style format arguments.
 */
template <typename... Args>
void dbg(const std::string &fmt, Args &&...args)
{
#ifndef NDEBUG
  logOutput(LogLevels::DBUG, fmt::format(fmt, args...));
#endif
}

/**
 * Print a information message to stderr. Supports fmt-style format arguments.
 */
template <typename... Args>
void info(const std::string &fmt, Args &&...args)
{
  logOutput(LogLevels::INFO, fmt::format(fmt, args...));
}

/**
 * Print a warning message to stderr. Supports fmt-style format arguments.
 */
template <typename... Args>
void warning(const std::string &fmt, Args &&...args)
{
  logOutput(LogLevels::WARN, fmt::format(fmt, args...));
}

/**
 * Print an error message to stderr. Supports fmt-style format arguments.
 */
template <typename... Args>
void error(const std::string &fmt, Args &&...args)
{
  logOutput(LogLevels::ERRO, fmt::format(fmt, args...));
}

/// Get the current minimum logging level
extern LogLevels minimumLoggingLevel();

/// Set the minimum logging level
extern void minimumLoggingLevel(LogLevels lvl);

}  // namespace seng::log
