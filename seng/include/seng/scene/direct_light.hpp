#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace seng {

/**
 * Simple class holding information about the directional light in the scene.
 */
class DirectLight {
 public:
  DirectLight() : m_color{0.0f}, m_dir{0.0f} {};
  DirectLight(glm::vec4 color, glm::vec3 direction) : m_color{color}, m_dir{direction} {};
  DirectLight(const DirectLight &) = default;
  DirectLight(DirectLight &&) = default;

  // Getters and setters
  glm::vec4 color() const { return m_color; }
  void color(glm::vec4 color) { m_color = color; }

  glm::vec3 direction() const { return m_dir; }
  void direction(glm::vec3 dir) { m_dir = dir; }

 private:
  glm::vec4 m_color;
  glm::vec3 m_dir;
};

}  // namespace seng
