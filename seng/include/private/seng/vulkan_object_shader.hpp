#pragma once

#include <functional>
#include <seng/primitive_types.hpp>
#include <seng/vulkan_buffer.hpp>
#include <seng/vulkan_pipeline.hpp>
#include <vulkan/vulkan_raii.hpp>

#define ATTRIBUTE_COUNT 1

namespace seng::rendering {

class VulkanDevice;
class VulkanShaderStage;
class VulkanRenderPass;
class VulkanBuffer;

/**
 * Material needed to render a specific object. It is composed of different
 * shader stages (see also VulkanShaderStage) and is rendered by a specific
 * pipeline.
 *
 * It implements the RAII paradigm, meaning instantiation allocates resources
 * while destruction deallocates them.
 *
 * It is move-able but not copyable.
 */
class VulkanObjectShader {
 public:
  static const int STAGES = 2;

  VulkanObjectShader(VulkanDevice& dev,
                     VulkanRenderPass& pass,
                     uint32_t globalPoolSize,
                     std::string name,
                     std::vector<std::shared_ptr<VulkanShaderStage>> stages);
  VulkanObjectShader(const VulkanObjectShader&) = delete;
  VulkanObjectShader(VulkanObjectShader&&) = default;
  ~VulkanObjectShader();

  VulkanObjectShader& operator=(const VulkanObjectShader&) = delete;
  VulkanObjectShader& operator=(VulkanObjectShader&&) = default;

  GlobalUniformObject& globalUniformObject() { return guo; }

  /**
   * Use the shader by binding the pipeline in the given command buffer
   */
  void use(VulkanCommandBuffer& buffer);

  /**
   * Push the values conntained in the global uniform object to the shader
   */
  void uploadGlobalState(VulkanCommandBuffer& buf, uint32_t imageIndex);

  /**
   * Push the given model matrix to the shader
   */
  void updateModelState(VulkanCommandBuffer& buf, glm::mat4 model);

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  std::string name;
  std::vector<std::shared_ptr<VulkanShaderStage>> _stages;

  vk::raii::DescriptorPool globalDescriptorPool;
  vk::raii::DescriptorSetLayout globalDescriptorSetLayout;
  VulkanPipeline pipeline;
  vk::raii::DescriptorSets globalDescriptorSets;
  GlobalUniformObject guo;
  VulkanBuffer gubo;
};

}  // namespace seng::rendering
