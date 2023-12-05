#include <cstdint>
#include <functional>
#include <seng/log.hpp>
#include <seng/primitive_types.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_object_shader.hpp>
#include <seng/vulkan_shader_stage.hpp>

using namespace seng::rendering;
using namespace std;
using namespace vk::raii;

#define ATTRIBUTE_COUNT 1

static VulkanPipeline createPipeline(
    VulkanDevice& device,
    VulkanRenderPass& pass,
    vector<shared_ptr<VulkanShaderStage>>& stages);

VulkanObjectShader::VulkanObjectShader(
    VulkanDevice& dev,
    VulkanRenderPass& pass,
    string name,
    vector<shared_ptr<VulkanShaderStage>> stages) :
    vkDevRef(dev),
    name(name),
    _stages(stages),
    pipeline(createPipeline(vkDevRef, pass, _stages)) {
  log::dbg("Created object shader {}", name);
}

VulkanPipeline createPipeline(VulkanDevice& device,
                              VulkanRenderPass& pass,
                              vector<shared_ptr<VulkanShaderStage>>& stages) {
  // Attributes:
  // 0. Position at location 0
  uint32_t offset = 0;
  vector<vk::VertexInputAttributeDescription> attributeDescriptions{
      ATTRIBUTE_COUNT};
  array<vk::Format, ATTRIBUTE_COUNT> formats{vk::Format::eR32G32B32Sfloat};
  array<uint64_t, ATTRIBUTE_COUNT> sizes{sizeof(Vertex)};

  for (uint32_t i = 0; i < ATTRIBUTE_COUNT; i++) {
    attributeDescriptions[i].binding = 0;
    attributeDescriptions[i].location = i;
    attributeDescriptions[i].format = formats[i];
    attributeDescriptions[i].offset = offset;
    offset += sizes[i];
  }

  // TODO: descriptors
  vector<DescriptorSetLayout> descriptors{};

  // Stages
  vector<vk::PipelineShaderStageCreateInfo> stageCreateInfo;
  for (size_t i = 0; i < VulkanObjectShader::STAGES; i++) {
    stageCreateInfo.emplace_back(stages[i]->createInfo());
  }

  VulkanPipeline::CreateInfo pipeInfo{attributeDescriptions, descriptors,
                                      stageCreateInfo, false};
  return VulkanPipeline{device, pass, pipeInfo};
}

void VulkanObjectShader::use(VulkanCommandBuffer& buffer) {
  pipeline.bind(buffer, vk::PipelineBindPoint::eGraphics);
}

VulkanObjectShader::~VulkanObjectShader() {
  // Since all handles are either all valid or all invalid, we simply check one
  // of them to see if we have been moved from
  if (*pipeline.handle() != vk::Pipeline(nullptr))
    log::dbg("Destroying object shader");
}
