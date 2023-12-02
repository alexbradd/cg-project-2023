#include <functional>
#include <memory>
#include <seng/shader_loader.hpp>
#include <seng/vulkan_object_shader.hpp>
#include <seng/vulkan_shader_stage.hpp>

using namespace seng::rendering;
using namespace std;

#define VERT_NAME "shader"
#define FRAG_NAME "shader"
#define SHADER_NAME "default"

ShaderLoader::ShaderLoader(VulkanDevice &dev,
                           VulkanRenderPass &pass,
                           std::string shaderPath) :
    vkDevRef(dev), vkRenderPassRef(pass), shaderPath(shaderPath) {}

// FIXME: add dynamic shader loading
void ShaderLoader::loadShaders() {
  stages.clear();
  shaders.clear();

  stages[VERT_NAME ".vert"] = make_shared<VulkanShaderStage>(
      vkDevRef, shaderPath, VERT_NAME, VulkanShaderStage::Type::eVertex,
      vk::ShaderStageFlagBits::eVertex);
  stages[FRAG_NAME ".frag"] = make_shared<VulkanShaderStage>(
      vkDevRef, shaderPath, FRAG_NAME, VulkanShaderStage::Type::eFragment,
      vk::ShaderStageFlagBits::eFragment);

  shaders[SHADER_NAME] = make_shared<VulkanObjectShader>(
      vkDevRef.get(), vkRenderPassRef.get(), SHADER_NAME,
      vector<shared_ptr<VulkanShaderStage>>{stages[VERT_NAME ".vert"],
                                            stages[FRAG_NAME ".frag"]});
}

shared_ptr<VulkanObjectShader> ShaderLoader::getShader(string name) {
  return shaders.at(name);
}
