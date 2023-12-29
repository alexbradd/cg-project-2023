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
    m_device(std::addressof(dev)),
    m_type(type),
    m_name(std::move(name)),
    m_code(std::invoke([&]() {
      namespace fs = std::filesystem;

      fs::path shaderDir(shaderLoadPath);
      const char *suffix;
      string filename;
      switch (m_type) {
        case ShaderStage::Type::eVertex:
          suffix = VERT_SUFFIX;
          break;
        case ShaderStage::Type::eFragment:
          suffix = FRAG_SUFFIX;
          break;
      }
      filename = this->m_name + suffix;

      return seng::internal::readFile((shaderDir / filename).string());
    })),
    m_moduleCreateInfo{
        {}, m_code.size(), reinterpret_cast<const uint32_t *>(m_code.data())},
    m_module(dev.logical(), m_moduleCreateInfo),
    m_stageCreateInfo(std::invoke([&]() {
      vk::ShaderStageFlagBits flags{};
      switch (type) {
        case ShaderStage::Type::eVertex:
          flags = vk::ShaderStageFlagBits::eVertex;
          break;
        case ShaderStage::Type::eFragment:
          flags = vk::ShaderStageFlagBits::eFragment;
          break;
      }
      return vk::PipelineShaderStageCreateInfo{{}, flags, *m_module, "main"};
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
  if (*m_module != vk::ShaderModule{}) log::dbg("Destroying shader module");
}
