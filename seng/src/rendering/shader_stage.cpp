#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/utils.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using namespace seng::rendering;

static constexpr char VERT_EXT[] = ".vert.spv";
static constexpr char FRAG_EXT[] = ".frag.spv";

ShaderStage::ShaderStage(const Device &dev,
                         const std::string &shaderPath,
                         std::string name,
                         ShaderStageType type) :
    m_type(type), m_name(std::move(name)), m_module(nullptr)
{
  namespace fs = std::filesystem;

  fs::path shaderDir(shaderPath);
  std::string filename;
  switch (m_type) {
    case ShaderStageType::eVertex:
      filename = m_name + VERT_EXT;
      break;
    case ShaderStageType::eFragment:
      filename = m_name + FRAG_EXT;
      break;
  }
  fs::path stagePath(shaderDir / filename);
  if (!fs::exists(stagePath))
    throw std::runtime_error("Unable to find shader stage " + m_name);
  auto code = seng::internal::readFile(stagePath.string());
  log::dbg("Loaded {} shader stage from disk", m_name);

  vk::ShaderModuleCreateInfo ci;
  ci.codeSize = code.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
  m_module = vk::raii::ShaderModule(dev.logical(), ci);

  switch (type) {
    case ShaderStageType::eVertex:
      log::dbg("Uploaded {} vertex stage to device", m_name);
      break;
    case ShaderStageType::eFragment:
      log::dbg("Uploaded {} fragment stage to device", m_name);
      break;
  }
}

const vk::PipelineShaderStageCreateInfo ShaderStage::stageCreateInfo() const
{
  vk::ShaderStageFlagBits flags{};
  switch (m_type) {
    case ShaderStageType::eVertex:
      flags = vk::ShaderStageFlagBits::eVertex;
      break;
    case ShaderStageType::eFragment:
      flags = vk::ShaderStageFlagBits::eFragment;
      break;
  }
  return vk::PipelineShaderStageCreateInfo{{}, flags, *m_module, "main"};
}

ShaderStage::~ShaderStage()
{
  if (*m_module != vk::ShaderModule{}) log::dbg("Destroying shader stage");
}
