#include <cstdint>
#include <filesystem>
#include <seng/log.hpp>
#include <seng/utils.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_shader_stage.hpp>

using namespace seng::rendering;
using namespace std;
using namespace vk::raii;

static constexpr char VERT_SUFFIX[] = ".vert.spv";
static constexpr char FRAG_SUFFIX[] = ".frag.spv";

static vector<char> readCode(string &path,
                             string &name,
                             VulkanShaderStage::Type typ);

VulkanShaderStage::VulkanShaderStage(VulkanDevice &dev,
                                     string &shaderLoadPath,
                                     string name,
                                     VulkanShaderStage::Type type,
                                     vk::ShaderStageFlagBits flags)
    : vkDevRef(dev),
      typ(type),
      name(name),
      code(readCode(shaderLoadPath, name, typ)),
      moduleCreateInfo{
          {}, code.size(), reinterpret_cast<const uint32_t *>(code.data())},
      module(vkDevRef.get().logical(), moduleCreateInfo),
      stageCreateInfo{{}, flags, *module, "main"} {
  switch (type) {
    case VulkanShaderStage::Type::eVertex:
      log::dbg("Loaded vertex shader named {}", name);
      break;
    case VulkanShaderStage::Type::eFragment:
      log::dbg("Loaded fragment shader named {}", name);
      break;
  }
}

vector<char> readCode(string &path, string &name, VulkanShaderStage::Type typ) {
  namespace fs = std::filesystem;

  fs::path shaderDir(path);
  const char *suffix;
  string filename;

  switch (typ) {
    case VulkanShaderStage::Type::eVertex:
      suffix = VERT_SUFFIX;
      break;
    case VulkanShaderStage::Type::eFragment:
      suffix = FRAG_SUFFIX;
      break;
  }
  filename = name + suffix;

  return seng::internal::readFile((shaderDir / filename).string());
}

VulkanShaderStage::~VulkanShaderStage() {
  if (*module != vk::ShaderModule{}) log::dbg("Destroying shader module");
}
