#pragma once

#include <string>
#include <vector>

namespace seng::internal {

/**
 * Read the contents of the file with the given path in an array of bytes.
 */
extern std::vector<char> readFile(const std::string &name);

/**
 * Create a vector containing n in-place created objects
 */
template <typename T, typename... Args>
std::vector<T> many(typename std::vector<T>::size_type n, Args &&...args)
{
  std::vector<T> ret;
  ret.reserve(n);
  for (typename std::vector<T>::size_type i = 0; i < n; i++) {
    ret.emplace_back(args...);
  }
  return ret;
}

/**
 * Helper function to combine a given hash with a generated hash for the input param.
 */
template <class T>
inline void hashCombine(size_t &seed, const T &v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}  // namespace seng::internal
