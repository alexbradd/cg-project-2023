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
 * Combine multiple hashes into to the given seed.
 *
 * Implementation is adapted from boost's `hash_combine`.
 */
template <class T, typename... Rest>
inline void hashCombine(size_t &seed, const T &v, Rest... rest)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hashCombine(seed, rest), ...);
}

}  // namespace seng::internal

/**
 * Conveninece macro to specialize hash<> for a given type.
 *
 * This macro works for simple un-templated types. For templated types, specialize
 * hash<> manually.
 */
#define MAKE_HASHABLE(type, ...)                \
  namespace std {                               \
  template <>                                   \
  struct hash<type> {                           \
    std::size_t operator()(const type &t) const \
    {                                           \
      using seng::internal::hashCombine;        \
      std::size_t ret = 0;                      \
      hashCombine(ret, __VA_ARGS__);            \
      return ret;                               \
    }                                           \
  };                                            \
  }
