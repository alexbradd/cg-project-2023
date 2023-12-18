#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace seng::rendering {

/**
 * Basic vertex format
 */
struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static constexpr size_t ATTRIBUTE_COUNT = 3;

  /**
   * Attribute description (in order):
   *
   * 0. vec3<float> position
   * 1. vec3<float> vertex color
   * 2. vec2<float> UV coordinates
   */
  static std::array<vk::VertexInputAttributeDescription, ATTRIBUTE_COUNT>
  attributeDescriptions()
  {
    std::array<vk::VertexInputAttributeDescription, 3> descs{};

    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].format = vk::Format::eR32G32B32Sfloat;
    descs[0].offset = offsetof(Vertex, pos);

    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].format = vk::Format::eR32G32B32Sfloat;
    descs[1].offset = offsetof(Vertex, color);

    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].format = vk::Format::eR32G32Sfloat;
    descs[2].offset = offsetof(Vertex, texCoord);

    return descs;
  }

  bool operator==(const Vertex& other) const
  {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
  }
};

// Less verbose and useful in some occasions
using AttributeDescriptions =
    std::array<vk::VertexInputAttributeDescription, Vertex::ATTRIBUTE_COUNT>;

struct GlobalUniformObject {
  glm::mat4 projection;
  glm::mat4 view;
  // Align to 256 bytes
  glm::mat4 reserved0;
  glm::mat4 reserved1;
};

}  // namespace seng::rendering

// Curtesy of vulkan-tutorial.com, chapter about loading models
namespace std {
template <>
struct hash<seng::rendering::Vertex> {
  size_t operator()(seng::rendering::Vertex const& vertex) const
  {
    auto hashVec3 = hash<glm::vec3>();
    auto hashVec2 = hash<glm::vec2>();
    return ((hashVec3(vertex.pos) ^ hashVec3(vertex.color) << 1) >> 1) ^
           (hashVec2(vertex.texCoord) << 1);
  }
};
}  // namespace std
