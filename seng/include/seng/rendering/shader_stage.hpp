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
  const vk::PipelineShaderStageCreateInfo& createInfo() const
  {
    return m_stageCreateInfo;
  }

 private:
  const Device* m_device;
  Type m_type;
  std::string m_name;
  std::vector<char> m_code;
  vk::ShaderModuleCreateInfo m_moduleCreateInfo;
  vk::raii::ShaderModule m_module;
  vk::PipelineShaderStageCreateInfo m_stageCreateInfo;
};

}  // namespace seng::rendering
