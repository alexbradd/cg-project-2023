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

VulkanShaderStage::VulkanShaderStage(const VulkanDevice &dev,
                                     const string &shaderLoadPath,
                                     string name,
                                     VulkanShaderStage::Type type) :
    vulkanDev(std::addressof(dev)),
    typ(type),
    name(std::move(name)),
    code(std::invoke([&]() {
      namespace fs = std::filesystem;

      fs::path shaderDir(shaderLoadPath);
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
    })),
    moduleCreateInfo{{}, code.size(), reinterpret_cast<const uint32_t *>(code.data())},
    module(dev.logical(), moduleCreateInfo),
    stageCreateInfo(std::invoke([&]() {
      vk::ShaderStageFlagBits flags{};
      switch (type) {
        case VulkanShaderStage::Type::eVertex:
          flags = vk::ShaderStageFlagBits::eVertex;
          break;
        case VulkanShaderStage::Type::eFragment:
          flags = vk::ShaderStageFlagBits::eFragment;
          break;
      }
      return vk::PipelineShaderStageCreateInfo{{}, flags, *module, "main"};
    }))
{
  switch (type) {
    case VulkanShaderStage::Type::eVertex:
      log::dbg("Loaded vertex shader named {}", name);
      break;
    case VulkanShaderStage::Type::eFragment:
      log::dbg("Loaded fragment shader named {}", name);
      break;
  }
}

VulkanShaderStage::~VulkanShaderStage()
{
  if (*module != vk::ShaderModule{}) log::dbg("Destroying shader module");
}
