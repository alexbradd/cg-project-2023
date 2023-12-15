#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace seng::rendering {

struct Vertex {
  glm::vec3 pos;
};

struct GlobalUniformObject {
  glm::mat4 projection;
  glm::mat4 view;
  // Align to 256 bytes
  glm::mat4 reserved0;
  glm::mat4 reserved1;
};

}  // namespace seng::rendering
