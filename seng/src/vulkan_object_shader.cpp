#include <functional>
#include <seng/log.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_object_shader.hpp>
#include <seng/vulkan_shader_stage.hpp>

using namespace seng::rendering;
using namespace std;
// using namespace vk::raii;

VulkanObjectShader::VulkanObjectShader(
    VulkanDevice& dev,
    VulkanRenderPass& pass,
    string name,
    vector<shared_ptr<VulkanShaderStage>> stages)
    : vkDevRef(dev), name(name), _stages(stages) {
  log::dbg("Created object shader {}", name);
}

VulkanObjectShader::VulkanObjectShader(VulkanObjectShader&& rhs)
    : isMoved(std::exchange(rhs.isMoved, true)),
      vkDevRef(std::move(rhs.vkDevRef)),
      name(std::move(rhs.name)),
      _stages(std::move(rhs._stages)) {}

VulkanObjectShader& VulkanObjectShader::operator=(VulkanObjectShader&& rhs) {
  if (this != &rhs) {
    std::swap(isMoved, rhs.isMoved);
    std::swap(vkDevRef, rhs.vkDevRef);
    std::swap(name, rhs.name);
    std::swap(_stages, rhs._stages);
  }
  return *this;
}

VulkanObjectShader::~VulkanObjectShader() {
  if (!isMoved) log::dbg("Destroying object shader");
}
