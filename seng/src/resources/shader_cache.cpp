#include <seng/log.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/resources/object_shader.hpp>
#include <seng/resources/shader_cache.hpp>
#include <seng/resources/shader_stage.hpp>
#include <seng/resources/texture.hpp>

#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace seng;

void ShaderCache::fromSchema(rendering::Renderer &renderer,
                             const std::string &path,
                             const std::string &shaderPath)
{
  // Clear in case of multiple calls
  m_stages.clear();
  m_shaders.clear();

  YAML::Node shaderConfig;
  try {
    shaderConfig = YAML::LoadFile(path);
  } catch (std::exception &e) {
    seng::log::error("Unable to load shader definition: {}", e.what());
    return;
  }

  if (!shaderConfig.IsMap()) {
    seng::log::error("Shader definition should be a map, bailing");
    return;
  }

  parseShaders(renderer, shaderPath, shaderConfig);
  // TODO: instances
}

// Shader YAML parsing
std::string ShaderCache::parseStage(rendering::Renderer &renderer,
                                    const std::string &shaderPath,
                                    ShaderStageType type,
                                    const YAML::Node &stage)
{
  if (stage && stage.IsScalar()) {
    std::string name = stage.as<std::string>();
    auto ret = m_stages.try_emplace(name, renderer.device(), shaderPath, name, type);
    if (!ret.second) seng::log::warning("Duplicated stage name");
    return name;
  } else {
    throw std::runtime_error("Shader definition must include all stages");
  }
}

void ShaderCache::parseShaders(rendering::Renderer &renderer,
                               const std::string &shaderPath,
                               const YAML::Node &config)
{
  if (!config["Shaders"]) throw std::runtime_error("No shader definitions available");
  if (!config["Shaders"].IsSequence())
    throw std::runtime_error("Shader definitions should be in a sequence");

  YAML::Node shaders = config["Shaders"];
  for (auto it = shaders.begin(); it != shaders.end(); it++) {
    auto shader = *it;
    if (!shader.IsMap()) throw std::runtime_error("Shader definition is not a map");

    if (!shader["name"] || !shader["name"].IsScalar())
      throw std::runtime_error("Shader definition must have a valid string as name");
    std::string name = shader["name"].as<std::string>();

    // Stages
    std::vector<std::string> stageNames;
    std::vector<const ShaderStage *> stages(ObjectShader::STAGES);
    stageNames.reserve(ObjectShader::STAGES);

    stageNames.emplace_back(
        parseStage(renderer, shaderPath, ShaderStageType::eVertex, shader["vert"]));
    stageNames.emplace_back(
        parseStage(renderer, shaderPath, ShaderStageType::eFragment, shader["frag"]));
    std::transform(stageNames.begin(), stageNames.end(), stages.begin(),
                   [&](const auto &name) { return &m_stages.at(name); });

    // Texture types
    std::vector<TextureType> textures;
    if (shader["textureTypes"] && shader["textureTypes"].IsSequence()) {
      auto types = shader["textureTypes"];
      for (auto type = types.begin(); type != types.end(); type++) {
        if (!type->IsScalar())
          throw std::runtime_error("Texture type should be a valid string");
        std::string typeName = type->as<std::string>();
        if (typeName == "1d")
          textures.push_back(TextureType::e1D);
        else if (typeName == "2d")
          textures.push_back(TextureType::e2D);
        else
          throw std::runtime_error("Texture type should be either '1d' or '2d'");
      }
    }

    auto ret = m_shaders.try_emplace(name, renderer, name, std::move(textures), stages);
    if (!ret.second)
      seng::log::warning("Duplicated shader name {}", name);
    else
      seng::log::dbg("Parsed shader {}", name);
  }
}