#pragma once

#include <string>
#include <vector>

namespace seng::internal {

extern std::vector<char> readFile(const std::string& name);

/**
 * Create a vector containing n in-place created objects
 */
template <typename T, typename... Args>
std::vector<T> many(typename std::vector<T>::size_type n, Args&&... args) {
  std::vector<T> ret;
  ret.reserve(n);
  for (typename std::vector<T>::size_type i = 0; i < n; i++) {
    ret.emplace_back(args...);
  }
  return ret;
}

}  // namespace seng::internal
