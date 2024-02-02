#pragma once

#include <cstddef>
#include <seng/hashes.hpp>
#include <seng/utils.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace seng::rendering {

/**
 * Basic vertex format
 */
struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 texCoord;
  glm::vec3 tangent;

  static constexpr size_t ATTRIBUTE_COUNT = 5;

  /**
   * Attribute description (in order):
   *
   * 0. vec3<float> position
   * 1. vec3<float> normal
   * 2. vec3<float> vertex color
   * 3. vec2<float> UV coordinates
   * 4. vec3<float> tangent vector
   */
  static std::array<vk::VertexInputAttributeDescription, ATTRIBUTE_COUNT>
  attributeDescriptions()
  {
    std::array<vk::VertexInputAttributeDescription, ATTRIBUTE_COUNT> descs;

    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].format = vk::Format::eR32G32B32Sfloat;
    descs[0].offset = offsetof(Vertex, pos);

    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].format = vk::Format::eR32G32B32Sfloat;
    descs[1].offset = offsetof(Vertex, normal);

    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].format = vk::Format::eR32G32B32Sfloat;
    descs[2].offset = offsetof(Vertex, color);

    descs[3].binding = 0;
    descs[3].location = 3;
    descs[3].format = vk::Format::eR32G32Sfloat;
    descs[3].offset = offsetof(Vertex, texCoord);

    descs[4].binding = 0;
    descs[4].location = 4;
    descs[4].format = vk::Format::eR32G32B32Sfloat;
    descs[4].offset = offsetof(Vertex, tangent);

    return descs;
  }

  bool operator==(const Vertex& other) const
  {
    return pos == other.pos && normal == other.normal && color == other.color &&
           texCoord == other.texCoord && tangent == other.tangent;
  }
};

// Less verbose and useful in some occasions
using AttributeDescriptions =
    std::array<vk::VertexInputAttributeDescription, Vertex::ATTRIBUTE_COUNT>;

}  // namespace seng::rendering

MAKE_HASHABLE(seng::rendering::Vertex, t.pos, t.normal, t.color, t.texCoord, t.tangent);
