#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>

namespace seng::rendering {

class Device;

/**
 * Shader stages are basically wrappers for ShaderModules. They are part of
 * an object shader (see VulkanObjectShader) and will be attached to a pipeline.
 *
 * It implements the RAII paradigm, meaning instantiation allocates resources
 * while destruction deallocates them.
 *
 * It is move-able but not copyable.
 */
class ShaderStage {
 public:
  enum struct Type { eVertex, eFragment };

  ShaderStage(const Device& device,
              const std::string& shaderLoadPath,
              std::string name,
              Type type);
  ShaderStage(const ShaderStage&) = delete;
  ShaderStage(ShaderStage&&) = default;
  ~ShaderStage();

  ShaderStage& operator=(const ShaderStage&) = delete;
  ShaderStage& operator=(ShaderStage&&) = default;

  // Accessors
  const vk::PipelineShaderStageCreateInfo& createInfo() const { return stageCreateInfo; }

 private:
  const Device* vulkanDev;
  Type typ;
  std::string name;
  std::vector<char> code;
  vk::ShaderModuleCreateInfo moduleCreateInfo;
  vk::raii::ShaderModule module;
  vk::PipelineShaderStageCreateInfo stageCreateInfo;
};

}  // namespace seng::rendering
