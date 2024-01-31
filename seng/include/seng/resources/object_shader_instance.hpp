#pragma once

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>

#include <string>
#include <vector>

namespace seng {

namespace rendering {
class Renderer;
class FrameHandle;
class CommandBuffer;
}  // namespace rendering

class ObjectShader;

/**
 * An instance of an object shader is a link between a set of textures and an
 * object shader. It is equivalent to a material.
 *
 * Creating an instance allocates all necessary textures (if not already loaded
 * and cached in the renderer), and descriptors.
 */
class ObjectShaderInstance {
 public:
  /**
   * Creates an instance named `name` of the given ObjectShader with the
   * textures each having paths in `textures`. To find out more how textures are
   * loaded, check the `Texture` class.
   */
  ObjectShaderInstance(rendering::Renderer& renderer,
                       const ObjectShader& shader,
                       std::string name,
                       std::vector<std::string> textures);
  ObjectShaderInstance(const ObjectShaderInstance&) = delete;
  ObjectShaderInstance(ObjectShaderInstance&&) = default;

  ObjectShaderInstance& operator=(const ObjectShaderInstance&) = delete;
  ObjectShaderInstance& operator=(ObjectShaderInstance&&) = default;

  const ObjectShader& instanceOf() const { return *m_shader; }
  const std::vector<vk::DescriptorImageInfo>& imageInfos() const { return m_imgInfos; }

  /**
   * Bind the descriptor sets allocated for the given frame used by this instance.
   */
  void bindDescriptorSets(const rendering::FrameHandle& handle,
                          const rendering::CommandBuffer& buf) const;

  /**
   * Push the given model matrix to the shader.
   */
  void updateModelState(const rendering::CommandBuffer& buf,
                        const glm::mat4& model) const;

 private:
  rendering::Renderer* m_renderer;
  const ObjectShader* m_shader;
  std::string m_name;
  std::vector<std::string> m_texturePaths;

  std::vector<vk::DescriptorImageInfo> m_imgInfos;
};

}  // namespace seng
