#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/utils.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace seng::rendering;
using namespace std;

static constexpr char VERT_SUFFIX[] = ".vert.spv";
static constexpr char FRAG_SUFFIX[] = ".frag.spv";

ShaderStage::ShaderStage(const Device &dev,
                         const string &shaderLoadPath,
                         string name,
                         ShaderStage::Type type) :
    vulkanDev(std::addressof(dev)),
    typ(type),
    name(std::move(name)),
    code(std::invoke([&]() {
      namespace fs = std::filesystem;

      fs::path shaderDir(shaderLoadPath);
      const char *suffix;
      string filename;
      switch (typ) {
        case ShaderStage::Type::eVertex:
          suffix = VERT_SUFFIX;
          break;
        case ShaderStage::Type::eFragment:
          suffix = FRAG_SUFFIX;
          break;
      }
      filename = this->name + suffix;

      return seng::internal::readFile((shaderDir / filename).string());
    })),
    moduleCreateInfo{{}, code.size(), reinterpret_cast<const uint32_t *>(code.data())},
    module(dev.logical(), moduleCreateInfo),
    stageCreateInfo(std::invoke([&]() {
      vk::ShaderStageFlagBits flags{};
      switch (type) {
        case ShaderStage::Type::eVertex:
          flags = vk::ShaderStageFlagBits::eVertex;
          break;
        case ShaderStage::Type::eFragment:
          flags = vk::ShaderStageFlagBits::eFragment;
          break;
      }
      return vk::PipelineShaderStageCreateInfo{{}, flags, *module, "main"};
    }))
{
  switch (type) {
    case ShaderStage::Type::eVertex:
      log::dbg("Loaded vertex shader named {}", name);
      break;
    case ShaderStage::Type::eFragment:
      log::dbg("Loaded fragment shader named {}", name);
      break;
  }
}

ShaderStage::~ShaderStage()
{
  if (*module != vk::ShaderModule{}) log::dbg("Destroying shader module");
}
