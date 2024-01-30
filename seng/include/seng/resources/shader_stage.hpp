#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <string>

namespace seng {

namespace rendering {
class Device;
}

/// Supported types of loadable shader stages
enum struct ShaderStageType { eVertex, eFragment };

/**
 * A ShaderStage is a wrapper for vk::ShaderModules. A ShaderStage can either
 * be a vertex stage or a fragment stage. Stage type determines at which Pipeline
 * point the shader will be executed.
 */
class ShaderStage {
 public:
  /**
   * Load a new ShaderStage from on-disk SPIR-V code.
   *
   * Stages are searched in the given `shaderPath`. Filename construction is
   * done as follows: `${shaderPath}/${name}.${STAGE}.spv` where
   * `STAGE := vert|frag` depending on the ShaderStageType.
   */
  ShaderStage(const rendering::Device& device,
              const std::string& shaderPath,
              std::string name,
              ShaderStageType type);
  ShaderStage(const ShaderStage&) = delete;
  ShaderStage(ShaderStage&&) = default;
  ~ShaderStage();

  ShaderStage& operator=(const ShaderStage&) = delete;
  ShaderStage& operator=(ShaderStage&&) = default;

  // Accessors
  vk::ShaderModule handle() const { return *m_module; }
  const std::string& name() const { return m_name; }
  ShaderStageType type() const { return m_type; }

  /// Generate a PipelineShaderStageCreateInfo for this ShaderStage
  const vk::PipelineShaderStageCreateInfo stageCreateInfo() const;

 private:
  ShaderStageType m_type;
  std::string m_name;
  vk::raii::ShaderModule m_module;
};

}  // namespace seng
