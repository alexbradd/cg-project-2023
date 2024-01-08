#pragma once

#include <seng/rendering/buffer.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace seng::rendering {
class Renderer;
class FrameHandle;

/// Data bound at the Vertex stage containing info about the projection
struct ProjectionUniform {
  alignas(16) glm::mat4 projection;
  alignas(16) glm::mat4 view;

  /// Returns the binding info for this struct. The binding index is to be
  /// set by others
  static vk::DescriptorSetLayoutBinding binding();
};

/**
 * The Global Uniform Buffer Object (GUBO).
 *
 * This uniform is passed as the first set to all shaders and contains two bindings:
 *
 * 1. Binding 0: projection data
 * 2. Binding 1: lighting data
 */
class GlobalUniform {
 public:
  static constexpr int BINDINGS = 1;  // TODO: add liighting data

  GlobalUniform(const Renderer &renderer);
  GlobalUniform(const GlobalUniform &) = delete;
  GlobalUniform(GlobalUniform &&) = default;

  GlobalUniform &operator=(const GlobalUniform &) = delete;
  GlobalUniform &operator=(GlobalUniform &&) = default;

  /// The set layout for this uniform
  const vk::raii::DescriptorSetLayout &layout() const { return m_layout; }

  const ProjectionUniform &projection() const { return m_projection; }
  ProjectionUniform &projection()
  {
    m_dirty = true;
    return m_projection;
  }

  const std::vector<vk::DescriptorBufferInfo> &bufferInfos() const
  {
    return m_cacheInfos;
  }

  bool dirty() const { return m_dirty; }

  /**
   * Update the uniform if there have been any changes to it since the last write
   */
  void updateIfDirty(const FrameHandle &handle) const;

 private:
  const Renderer *m_renderer;
  vk::raii::DescriptorSetLayout m_layout;

  mutable bool m_dirty = false;

  ProjectionUniform m_projection;
  Buffer m_projectionBuffer;
  vk::DescriptorBufferInfo m_projectionInfo;

  std::vector<vk::DescriptorBufferInfo> m_cacheInfos;
};

}  // namespace seng::rendering
