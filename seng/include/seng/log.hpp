#ifndef __SENG_LOG_HPP__
#define __SENG_LOG_HPP__

#include <iostream>
#include <string>
#include <fmt/core.h>

namespace seng::log {

#define INFO_LVL "[INFO] "
#define WARN_LVL "[WARN] "
#define ERRO_LVL "[ERRO] "

template <typename... Args>
void print_to_err(const std::string &lvl, const std::string &fmt,
                  Args&&... args) {
  std::cerr << lvl << fmt::format(fmt, args...) << std::endl;
}

template <typename... Args> void info(const std::string &fmt, Args&&... args) {
  print_to_err(INFO_LVL, fmt, args...);
}

template <typename... Args> void warning(const std::string &fmt, Args&&... args) {
  print_to_err(WARN_LVL, fmt, args...);
}

template <typename... Args> void error(const std::string &fmt, Args&&... args) {
  print_to_err(ERRO_LVL, fmt, args...);
}

} // seng::log

#endif // !__SENG_LOG_HPP__

