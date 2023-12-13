#include <cstdint>
#include <memory>
#include <seng/shader_loader.hpp>
#include <seng/vulkan_object_shader.hpp>
#include <seng/vulkan_shader_stage.hpp>

using namespace seng::rendering;
using namespace std;

#define VERT_NAME "shader"
#define FRAG_NAME "shader"
#define SHADER_NAME "default"

ShaderLoader::ShaderLoader(const VulkanDevice &dev,
                           const VulkanRenderPass &pass,
                           uint32_t globalPoolSize,
                           std::string shaderPath) :
    vulkanDev(std::addressof(dev)),
    vulkanRenderPass(std::addressof(pass)),
    shaderPath(shaderPath),
    globalPoolSize(globalPoolSize)
{
}

// FIXME: add dynamic shader loading
void ShaderLoader::loadShaders()
{
  stages.clear();
  shaders.clear();

  stages[VERT_NAME ".vert"] = make_shared<VulkanShaderStage>(
      *vulkanDev, shaderPath, VERT_NAME, VulkanShaderStage::Type::eVertex,
      vk::ShaderStageFlagBits::eVertex);
  stages[FRAG_NAME ".frag"] = make_shared<VulkanShaderStage>(
      *vulkanDev, shaderPath, FRAG_NAME, VulkanShaderStage::Type::eFragment,
      vk::ShaderStageFlagBits::eFragment);

  shaders[SHADER_NAME] = make_shared<VulkanObjectShader>(
      *vulkanDev, *vulkanRenderPass, globalPoolSize, SHADER_NAME,
      vector<shared_ptr<VulkanShaderStage>>{stages[VERT_NAME ".vert"],
                                            stages[FRAG_NAME ".frag"]});
}

shared_ptr<VulkanObjectShader> ShaderLoader::getShader(string name) const
{
  return shaders.at(name);
}
