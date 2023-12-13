#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace seng::rendering {

class VulkanObjectShader;
class VulkanShaderStage;
class VulkanDevice;
class VulkanRenderPass;

/**
 * Handles the storage for all shader stages and corresponding object shaders
 */
class ShaderLoader {
 public:
  ShaderLoader(const VulkanDevice &dev,
               const VulkanRenderPass &pass,
               uint32_t globalPoolSize,
               std::string shaderPath);
  ShaderLoader(const ShaderLoader &) = delete;
  ShaderLoader(ShaderLoader &&) = default;

  ShaderLoader &operator=(const ShaderLoader &) = delete;
  ShaderLoader &operator=(ShaderLoader &&) = default;

  /**
   * Load all necessary shaders
   */
  void loadShaders();

  std::shared_ptr<VulkanObjectShader> getShader(std::string name) const;

 private:
  const VulkanDevice *vulkanDev;
  const VulkanRenderPass *vulkanRenderPass;
  std::string shaderPath;
  uint32_t globalPoolSize;
  std::unordered_map<std::string, std::shared_ptr<VulkanShaderStage>> stages;
  std::unordered_map<std::string, std::shared_ptr<VulkanObjectShader>> shaders;
};

}  // namespace seng::rendering
