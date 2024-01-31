#pragma once

#include <seng/resources/object_shader.hpp>
#include <seng/resources/object_shader_instance.hpp>
#include <seng/resources/shader_stage.hpp>

#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

namespace seng {

namespace rendering {
class Renderer;
}

/**
 * Simple cache for all shading objects: shader stages, object shaders and
 * instances.
 *
 * The cache can be populated by reading the shader definition file. After
 * population, it is read-only.
 */
class ShaderCache {
 public:
  using StageCache = std::unordered_map<std::string, ShaderStage>;
  using ObjectShaderCache = std::unordered_map<std::string, ObjectShader>;
  using ObjectShaderInstanceCache = std::unordered_map<std::string, ObjectShaderInstance>;

  ShaderCache() = default;
  ShaderCache(const ShaderCache&) = delete;
  ShaderCache(ShaderCache&&) = default;

  ShaderCache& operator=(const ShaderCache&) = delete;
  ShaderCache& operator=(ShaderCache&&) = default;

  /// Cached ShaderStages. The search key is the stage name.
  const StageCache& stages() const { return m_stages; }

  /// Cached ObjectShaders. The search key is the shader name.
  const ObjectShaderCache& objectShaders() const { return m_shaders; }

  /// Cached ObjectShaderInstances. The search key is the instance name.
  const ObjectShaderInstanceCache& objectShaderInstances() const { return m_instances; }

  /**
   * Parse shader data from a shader definition file with path `path`.
   * Shader resources will be read from the given `shaderPath`.
   *
   * Note: calling this method will drop all previous cached resources.
   */
  void fromSchema(rendering::Renderer& renderer,
                  const std::string& path,
                  const std::string& shaderPath);

 private:
  StageCache m_stages;
  ObjectShaderCache m_shaders;
  ObjectShaderInstanceCache m_instances;

  std::string parseStage(rendering::Renderer& renderer,
                         const std::string& shaderPath,
                         ShaderStageType type,
                         const YAML::Node& node);
  void parseShaders(rendering::Renderer& renderer,
                    const std::string& shaderPath,
                    const YAML::Node& node);
  void parseInstances(rendering::Renderer& renderer, const YAML::Node& node);
};

}  // namespace seng
