#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanCommandBuffer;
class VulkanDevice;
class VulkanRenderPass;

/**
 * Wrapper around a vulkan pipline. It implements the RAII pattern, meaning that
 * instantiation allocates a new pipline, while destruction deallocates it.
 */
class VulkanPipeline {
 public:
  /**
   * A small helper that contains information useful for pipeline createion.
   */
  struct CreateInfo {
    std::vector<vk::VertexInputAttributeDescription>& attributes;
    std::vector<vk::raii::DescriptorSetLayout>& descriptorSetLayouts;
    std::vector<vk::PipelineShaderStageCreateInfo>& stages;
    bool wireframe = false;
  };

  /**
   * Create a new pipeline that will draw to the give render pass using the
   * details specified in the given creation info
   */
  VulkanPipeline(VulkanDevice& device, VulkanRenderPass& pass, CreateInfo info);
  VulkanPipeline(const VulkanPipeline&) = delete;
  VulkanPipeline(VulkanPipeline&&) = default;
  ~VulkanPipeline();

  VulkanPipeline& operator=(const VulkanPipeline&) = delete;
  VulkanPipeline& operator=(VulkanPipeline&&) = default;

  vk::raii::Pipeline& handle() { return pipeline; }

  /**
   * Bind the pipeline at the given bind point on the given command buffer
   */
  void bind(VulkanCommandBuffer& buffer, vk::PipelineBindPoint bind);

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  std::reference_wrapper<VulkanRenderPass> vkRenderPassRef;
  vk::raii::PipelineLayout pipelineLayout;
  vk::raii::Pipeline pipeline;
};

}  // namespace seng::rendering
