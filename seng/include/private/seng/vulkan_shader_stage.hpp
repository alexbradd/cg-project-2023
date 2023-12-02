#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

/**
 * Shader stages are basically wrappers for ShaderModules. They are part of
 * an object shader (see VulkanObjectShader) and will be attached to a pipeline.
 *
 * It implements the RAII paradigm, meaning instantiation allocates resources
 * while destruction deallocates them.
 *
 * It is move-able but not copyable.
 */
class VulkanShaderStage {
 public:
  enum struct Type { eVertex, eFragment };

  VulkanShaderStage(VulkanDevice& device,
                    std::string& shaderLoadPath,
                    std::string name,
                    Type type,
                    vk::ShaderStageFlagBits flags);
  VulkanShaderStage(const VulkanShaderStage&) = delete;
  VulkanShaderStage(VulkanShaderStage&&) = default;
  ~VulkanShaderStage();

  VulkanShaderStage& operator=(const VulkanShaderStage&) = delete;
  VulkanShaderStage& operator=(VulkanShaderStage&&) = default;

  // Accessors
  vk::PipelineShaderStageCreateInfo createInfo() { return stageCreateInfo; }

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  Type typ;
  std::string name;
  std::vector<char> code;
  vk::ShaderModuleCreateInfo moduleCreateInfo;
  vk::raii::ShaderModule module;
  vk::PipelineShaderStageCreateInfo stageCreateInfo;
};

}  // namespace seng::rendering
