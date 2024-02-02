#pragma once

#include <cstddef>
#include <seng/rendering/primitive_types.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace seng::rendering {

class CommandBuffer;
class Renderer;
class RenderPass;

/// Struct that is sized exactly 128 bytes holding representing the push constant
/// range
struct PushConstants {
  glm::mat4 modelMatrix;
  glm::mat4 _reserved;
};

/**
 * Wrapper around a vulkan pipline. It implements the RAII pattern, meaning that
 * instantiation allocates a new pipline, while destruction deallocates it.
 */
class Pipeline {
 public:
  /**
   * A small helper that contains information useful for pipeline createion.
   */
  struct CreateInfo {
    AttributeDescriptions& attributes;
    std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts;
    std::vector<vk::PipelineShaderStageCreateInfo>& stages;
    bool wireframe = false;
  };

  /**
   * Create a new pipeline that will draw to the give render pass using the
   * details specified in the given creation info
   */
  Pipeline(const Renderer& renderer, const RenderPass& pass, CreateInfo info);

  /// Create a null pipeline
  Pipeline(std::nullptr_t);

  Pipeline(const Pipeline&) = delete;
  Pipeline(Pipeline&&) = default;
  ~Pipeline();

  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline& operator=(Pipeline&&) = default;

  const vk::raii::Pipeline& handle() const { return m_pipeline; }
  const vk::raii::PipelineLayout& layout() const { return m_layout; }

  /**
   * Bind the pipeline at the given bind point on the given command buffer
   */
  void bind(const CommandBuffer& buffer, vk::PipelineBindPoint bind) const;

 private:
  vk::raii::PipelineLayout m_layout;
  vk::raii::Pipeline m_pipeline;
};

}  // namespace seng::rendering
