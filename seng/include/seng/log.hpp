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
template <typename... Args>
void dbg(const std::string &fmt, Args &&...args) {
  std::cerr << DBUG_STR << __FILE__ << ":" << __LINE__ << " "
            << fmt::format(fmt, args...) << std::endl;
}
#else
template <typename... Args>
void dbg(const std::string &fmt, Args &&...args) {}
#endif

template <typename... Args>
void info(const std::string &fmt, Args &&...args) {
  std::cerr << INFO_STR << fmt::format(fmt, args...) << std::endl;
}

template <typename... Args>
void warning(const std::string &fmt, Args &&...args) {
  std::cerr << WARN_STR << fmt::format(fmt, args...) << std::endl;
}

template <typename... Args>
void error(const std::string &fmt, Args &&...args) {
  std::cerr << ERRO_STR << fmt::format(fmt, args...) << std::endl;
}

}  // namespace seng::log

#endif  // !__SENG_LOG_HPP__
