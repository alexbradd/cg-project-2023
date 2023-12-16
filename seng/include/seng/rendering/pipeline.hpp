#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace seng::rendering {

class CommandBuffer;
class Device;
class RenderPass;

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
    std::vector<vk::VertexInputAttributeDescription>& attributes;
    std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts;
    std::vector<vk::PipelineShaderStageCreateInfo>& stages;
    bool wireframe = false;
  };

  /**
   * Create a new pipeline that will draw to the give render pass using the
   * details specified in the given creation info
   */
  Pipeline(const Device& device, const RenderPass& pass, CreateInfo info);
  Pipeline(const Pipeline&) = delete;
  Pipeline(Pipeline&&) = default;
  ~Pipeline();

  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline& operator=(Pipeline&&) = default;

  const vk::raii::Pipeline& handle() const { return pipeline; }
  const vk::raii::PipelineLayout& layout() const { return pipelineLayout; }

  /**
   * Bind the pipeline at the given bind point on the given command buffer
   */
  void bind(const CommandBuffer& buffer, vk::PipelineBindPoint bind) const;

 private:
  const Device* vulkanDevice;
  const RenderPass* vulkanRenderPass;
  vk::raii::PipelineLayout pipelineLayout;
  vk::raii::Pipeline pipeline;
};

}  // namespace seng::rendering
