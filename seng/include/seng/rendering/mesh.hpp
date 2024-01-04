#pragma once

#include <seng/rendering/buffer.hpp>
#include <seng/rendering/primitive_types.hpp>

#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <optional>
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
  /// Create an empty mesh
  Mesh(const Renderer &renderer);
  Mesh(const Mesh &) = delete;
  Mesh(Mesh &&) = default;

  Mesh &operator=(const Mesh &) = delete;
  Mesh &operator=(Mesh &&) = default;

  // Accessors
  const std::vector<Vertex> &vertices() const { return m_vertices; }
  const std::vector<uint32_t> &indices() const { return m_indices; }

  const std::optional<Buffer> &vertexBuffer() const { return m_vbo; }
  const std::optional<Buffer> &indexBuffer() const { return m_ibo; }

  /// Return true if device buffers are up to date with the ones in host memory
  bool synced() const { return m_vbo.has_value() && m_ibo.has_value(); }

  /**
   * Send the in-host-memory mesh data to the device
   */
  void sync();

  /**
   * Clear the vertex and index buffers from the device
   */
  void free();

  /**
   * Factory method that creates a mesh by loading the model with the given name.
   *
   * Models are searched inside the asset path (defined in ApplicationConfig) and
   * filename construction is done like this: `${assetPath}/${name}.obj`
   */
  static Mesh loadFromDisk(const Renderer &renderer,
                           const std::string &assetPath,
                           std::string name);

 private:
  const Renderer *m_renderer;
  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;

  std::optional<Buffer> m_vbo;
  std::optional<Buffer> m_ibo;

  Mesh(const Renderer &renderer,
       std::vector<Vertex> &&vertices,
       std::vector<uint32_t> &&indices);
};

};  // namespace seng::rendering
