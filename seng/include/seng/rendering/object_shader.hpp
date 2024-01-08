#pragma once

#include <seng/rendering/buffer.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/primitive_types.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>

namespace seng::rendering {

class Renderer;
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

  ObjectShader(const Renderer& dev,
               const GlobalUniform& globalDescriptorSetLayout,
               std::string name,
               std::vector<const ShaderStage*> stages);
  ObjectShader(const ObjectShader&) = delete;
  ObjectShader(ObjectShader&&) = default;
  ~ObjectShader();

  ObjectShader& operator=(const ObjectShader&) = delete;
  ObjectShader& operator=(ObjectShader&&) = default;

  /**
   * Use the shader by binding the pipeline in the given command buffer
   */
  void use(const CommandBuffer& buffer) const;

  /**
   * Bind the descriptor sets allocated for the given frame used by this object
   * shader.
   */
  void bindDescriptorSets(const FrameHandle& handle, const CommandBuffer& buf) const;

  /**
   * Push the given model matrix to the shader
   */
  void updateModelState(const CommandBuffer& buf, glm::mat4 model) const;

 private:
  const Renderer* m_renderer;
  std::string m_name;
  std::vector<const ShaderStage*> m_stages;

  const GlobalUniform* m_gubo;
  Pipeline m_pipeline;
};

}  // namespace seng::rendering
