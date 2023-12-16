#include <seng/log.hpp>
#include <seng/primitive_types.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/shader_stage.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <stddef.h>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace seng::rendering;
using namespace std;
using namespace vk::raii;

static const vk::BufferUsageFlags UNIFORM_USAGE_FLAGS =
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;
static const vk::MemoryPropertyFlags UNIFORM_MEM_FLAGS =
    vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent;

ObjectShader::ObjectShader(const Device& dev,
                           const RenderPass& pass,
                           uint32_t globalPoolSize,
                           string name,
                           vector<const ShaderStage*> stages) :
    vulkanDev(std::addressof(dev)),
    name(std::move(name)),
    _stages(std::move(stages)),
    globalDescriptorPool(std::invoke([&]() {
      vk::DescriptorPoolSize poolSize{vk::DescriptorType::eUniformBuffer, globalPoolSize};
      vk::DescriptorPoolCreateInfo info{};
      info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
      info.maxSets = globalPoolSize;
      info.setPoolSizes(poolSize);
      return DescriptorPool(dev.logical(), info);
    })),
    globalDescriptorSetLayout(std::invoke([&]() {
      vk::DescriptorSetLayoutBinding guboBinding{};
      guboBinding.binding = 0;
      guboBinding.descriptorCount = 1;
      guboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
      guboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

      vk::DescriptorSetLayoutCreateInfo info{};
      info.bindingCount = 1;
      info.pBindings = &guboBinding;
      return DescriptorSetLayout(dev.logical(), info);
    })),
    pipeline(std::invoke([&]() {
      // Attributes:
      // 0. Position at location 0
      uint32_t offset = 0;
      vector<vk::VertexInputAttributeDescription> attributeDescriptions{ATTRIBUTE_COUNT};
      array<vk::Format, ATTRIBUTE_COUNT> formats{vk::Format::eR32G32B32Sfloat};
      array<uint64_t, ATTRIBUTE_COUNT> sizes{sizeof(Vertex)};

      for (uint32_t i = 0; i < ATTRIBUTE_COUNT; i++) {
        attributeDescriptions[i].binding = 0;
        attributeDescriptions[i].location = i;
        attributeDescriptions[i].format = formats[i];
        attributeDescriptions[i].offset = offset;
        offset += sizes[i];
      }

      // Descriptor layouts
      vector<vk::DescriptorSetLayout> descriptors;
      descriptors.emplace_back(*globalDescriptorSetLayout);
      // TODO: Local descriptor layouts

      // Stages
      vector<vk::PipelineShaderStageCreateInfo> stageCreateInfo;
      for (size_t i = 0; i < ObjectShader::STAGES; i++) {
        stageCreateInfo.emplace_back(stages[i]->createInfo());
      }

      Pipeline::CreateInfo pipeInfo{attributeDescriptions, descriptors, stageCreateInfo,
                                    false};
      return Pipeline(dev, pass, pipeInfo);
    })),
    globalDescriptorSets(std::invoke([&]() {
      vector<vk::DescriptorSetLayout> descs(globalPoolSize, *globalDescriptorSetLayout);

      vk::DescriptorSetAllocateInfo info{};
      info.descriptorPool = *globalDescriptorPool;
      info.setSetLayouts(descs);
      return DescriptorSets(dev.logical(), info);
    })),
    gubo(dev, UNIFORM_USAGE_FLAGS, sizeof(guo), UNIFORM_MEM_FLAGS, true)
{
  log::dbg("Created object shader {}", name);
}

void ObjectShader::use(const CommandBuffer& buffer) const
{
  pipeline.bind(buffer, vk::PipelineBindPoint::eGraphics);
}

void ObjectShader::uploadGlobalState(const CommandBuffer& buf, uint32_t imageIndex) const
{
  auto& descriptor = globalDescriptorSets[imageIndex];

  // Copy to buffer
  vk::DeviceSize range = sizeof(guo);
  vk::DeviceSize offset = 0;

  gubo.load(&guo, offset, range, {});

  // Update descriptor set
  vk::DescriptorBufferInfo bufferInfo{*gubo.buffer(), offset, range};
  vk::WriteDescriptorSet descWrite{};
  descWrite.dstSet = *descriptor;
  descWrite.dstBinding = 0;
  descWrite.dstArrayElement = 0;
  descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  descWrite.descriptorCount = 1;
  descWrite.pBufferInfo = &bufferInfo;
  vulkanDev->logical().updateDescriptorSets(descWrite, {});

  // Bind descriptor
  buf.buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout(), 0,
                                  *descriptor, {});
}

void ObjectShader::updateModelState(const CommandBuffer& buf, glm::mat4 model) const
{
  buf.buffer().pushConstants<glm::mat4>(*pipeline.layout(),
                                        vk::ShaderStageFlagBits::eVertex, 0, model);
}

ObjectShader::~ObjectShader()
{
  // Since all handles are either all valid or all invalid, we simply check one
  // of them to see if we have been moved from
  if (*pipeline.handle() != vk::Pipeline(nullptr)) log::dbg("Destroying object shader");
}
