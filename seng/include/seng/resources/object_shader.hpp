#pragma once

#include <seng/rendering/buffer.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/resources/texture.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>

namespace seng {

namespace rendering {
class Renderer;
class RenderPass;
class Buffer;
}  // namespace rendering

class ShaderStage;

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

  ObjectShader(rendering::Renderer& dev,
               std::string name,
               std::vector<TextureType> textures,
               const std::vector<const ShaderStage*>& stages);
  ObjectShader(const ObjectShader&) = delete;
  ObjectShader(ObjectShader&&) = default;
  ~ObjectShader();

  ObjectShader& operator=(const ObjectShader&) = delete;
  ObjectShader& operator=(ObjectShader&&) = default;

  const std::vector<TextureType>& textureLayout() const { return m_texLayout; }

  /**
   * Use the shader by binding the pipeline in the given command buffer
   */
  void use(const rendering::CommandBuffer& buffer) const;

  /**
   * Bind the descriptor sets allocated for the given frame used by this object
   * shader.
   */
  void bindDescriptorSets(const rendering::FrameHandle& handle,
                          const rendering::CommandBuffer& buf) const;

  /**
   * Push the given model matrix to the shader
   */
  void updateModelState(const rendering::CommandBuffer& buf, glm::mat4 model) const;

 private:
  const rendering::Renderer* m_renderer;
  std::string m_name;

  std::vector<TextureType> m_texLayout;
  vk::DescriptorSetLayout m_texSetLayout;

  rendering::Pipeline m_pipeline;
};

}  // namespace seng
