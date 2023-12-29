#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/shader_stage.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <stddef.h>
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
                           const vk::raii::DescriptorSetLayout& globalDescriptorSetLayout,
                           string name,
                           vector<const ShaderStage*> stages) :
    m_device(std::addressof(dev)),
    m_name(std::move(name)),
    m_stages(std::move(stages)),
    m_pipeline(std::invoke([&]() {
      // Attributes
      AttributeDescriptions attributes{Vertex::attributeDescriptions()};

      // Descriptor layouts
      vector<vk::DescriptorSetLayout> descriptors;
      descriptors.emplace_back(*globalDescriptorSetLayout);
      // TODO: Local descriptor layouts

      // Stages
      vector<vk::PipelineShaderStageCreateInfo> stageCreateInfo;
      for (size_t i = 0; i < ObjectShader::STAGES; i++) {
        stageCreateInfo.emplace_back(m_stages[i]->createInfo());
      }

      Pipeline::CreateInfo pipeInfo{attributes, descriptors, stageCreateInfo, false};
      return Pipeline(dev, pass, pipeInfo);
    })),
    m_gubo(dev, UNIFORM_USAGE_FLAGS, sizeof(m_guo), UNIFORM_MEM_FLAGS, true)
{
  log::dbg("Created object shader {}", name);
}

void ObjectShader::use(const CommandBuffer& buffer) const
{
  m_pipeline.bind(buffer, vk::PipelineBindPoint::eGraphics);
}

void ObjectShader::uploadGlobalState(const CommandBuffer& buf,
                                     const vk::raii::DescriptorSet& descriptor) const
{
  // Copy to buffer
  vk::DeviceSize range = sizeof(m_guo);
  vk::DeviceSize offset = 0;

  m_gubo.load(&m_guo, offset, range, {});

  // Update descriptor set
  vk::DescriptorBufferInfo bufferInfo{*m_gubo.buffer(), offset, range};
  vk::WriteDescriptorSet descWrite{};
  descWrite.dstSet = *descriptor;
  descWrite.dstBinding = 0;
  descWrite.dstArrayElement = 0;
  descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  descWrite.descriptorCount = 1;
  descWrite.pBufferInfo = &bufferInfo;
  m_device->logical().updateDescriptorSets(descWrite, {});

  // Bind descriptor
  buf.buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline.layout(),
                                  0, *descriptor, {});
}

void ObjectShader::updateModelState(const CommandBuffer& buf, glm::mat4 model) const
{
  buf.buffer().pushConstants<glm::mat4>(*m_pipeline.layout(),
                                        vk::ShaderStageFlagBits::eVertex, 0, model);
}

ObjectShader::~ObjectShader()
{
  // Since all handles are either all valid or all invalid, we simply check one
  // of them to see if we have been moved from
  if (*m_pipeline.handle() != vk::Pipeline(nullptr)) log::dbg("Destroying object shader");
}
