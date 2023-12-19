#pragma once

#include <seng/application_config.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/primitive_types.hpp>

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>

namespace seng::rendering {

class Renderer;

/**
 * A collection of vertices and indices that defines the shape of a 3D model.
 * To be used exclusively with indexed drawing.
 *
 * A mesh can be created by reading a 3D model from disk. Onyl OBJ files are supported.
 *
 * It is non-copyable, but movable.
 */
class Mesh {
 public:
  Mesh(const Mesh &) = delete;
  Mesh(Mesh &&) = default;

  Mesh &operator=(const Mesh &) = delete;
  Mesh &operator=(Mesh &&) = default;

  // Accessors
  const Buffer &vertexBuffer() const { return vertices; }
  const Buffer &indexBuffer() const { return indices; }
  uint32_t indexCount() const { return count; }

  /**
   * Factory method that creates a mesh by loading the model with the given name.
   *
   * Models are searched inside the asset path (defined in ApplicationConfig) and
   * filename construction is done like this: `${assetPath}/${name}.obj`
   */
  static Mesh loadFromDisk(const Renderer &renderer,
                           const ApplicationConfig &config,
                           std::string name);

 private:
  Buffer vertices;
  Buffer indices;
  uint32_t count;

  /**
   * Private constructor. Users must go through the `loadFromDisk()` factory method.
   */
  Mesh(const Renderer &renderer,
       const std::vector<Vertex> &vertices,
       const std::vector<uint32_t> &indices);
};

};  // namespace seng::rendering
