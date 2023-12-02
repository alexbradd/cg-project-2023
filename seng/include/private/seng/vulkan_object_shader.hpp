#pragma once

#include <functional>
#include <seng/vulkan_shader_stage.hpp>

namespace seng::rendering {

class VulkanDevice;
class VulkanShaderStage;
/* TODO: class VulkanPipeline; */
class VulkanRenderPass;

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
                     std::string name,
                     std::vector<std::shared_ptr<VulkanShaderStage>> stages);
  VulkanObjectShader(const VulkanObjectShader&) = delete;
  VulkanObjectShader(VulkanObjectShader&&);
  ~VulkanObjectShader();

  VulkanObjectShader& operator=(const VulkanObjectShader&) = delete;
  VulkanObjectShader& operator=(VulkanObjectShader&&);

 private:
  bool isMoved = false;
  std::reference_wrapper<VulkanDevice> vkDevRef;
  std::string name;
  std::vector<std::shared_ptr<VulkanShaderStage>> _stages;
  /* TODO: VulkanPipeline pipeline; */
};

}  // namespace seng::rendering