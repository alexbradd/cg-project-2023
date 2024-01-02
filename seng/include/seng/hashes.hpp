#pragma once

#include <seng/utils.hpp>

#include <glm/detail/qualifier.hpp>

namespace std {
template <typename T, glm::qualifier Q>
struct hash<glm::vec<2, T, Q>> {
  size_t operator()(glm::vec<2, T, Q> const& v)
  {
    using seng::internal::hashCombine;
    size_t seed = 0;
    hashCombine(seed, v.x, v.y);
    return seed;
  }
};

template <typename T, glm::qualifier Q>
struct hash<glm::vec<3, T, Q>> {
  size_t operator()(glm::vec<3, T, Q> const& v)
  {
    using seng::internal::hashCombine;
    size_t seed = 0;
    hashCombine(seed, v.x, v.y, v.z);
    return seed;
  }
};

}  // namespace std
