#pragma once

#include <seng/primitive_types.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/pipeline.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <string>
#include <vector>

#define ATTRIBUTE_COUNT 1

namespace seng::rendering {

class Device;
class ShaderStage;
class RenderPass;
class Buffer;

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
class ObjectShader {
 public:
  static constexpr int STAGES = 2;

  ObjectShader(const Device& dev,
               const RenderPass& pass,
               uint32_t globalPoolSize,
               std::string name,
               std::vector<const ShaderStage*> stages);
  ObjectShader(const ObjectShader&) = delete;
  ObjectShader(ObjectShader&&) = default;
  ~ObjectShader();

  ObjectShader& operator=(const ObjectShader&) = delete;
  ObjectShader& operator=(ObjectShader&&) = default;

  const GlobalUniformObject& globalUniformObject() const { return guo; }
  GlobalUniformObject& globalUniformObject() { return guo; }

  /**
   * Use the shader by binding the pipeline in the given command buffer
   */
  void use(const CommandBuffer& buffer) const;

  /**
   * Push the values conntained in the global uniform object to the shader
   */
  void uploadGlobalState(const CommandBuffer& buf, uint32_t imageIndex) const;

  /**
   * Push the given model matrix to the shader
   */
  void updateModelState(const CommandBuffer& buf, glm::mat4 model) const;

 private:
  const Device* vulkanDev;
  std::string name;
  std::vector<const ShaderStage*> _stages;

  vk::raii::DescriptorPool globalDescriptorPool;
  vk::raii::DescriptorSetLayout globalDescriptorSetLayout;
  Pipeline pipeline;
  vk::raii::DescriptorSets globalDescriptorSets;
  GlobalUniformObject guo;
  Buffer gubo;
};

}  // namespace seng::rendering
