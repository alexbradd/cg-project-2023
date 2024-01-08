#include <seng/rendering/device.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/renderer.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <array>

using namespace seng::rendering;

// ==== Bindings
vk::DescriptorSetLayoutBinding ProjectionUniform::binding()
{
  vk::DescriptorSetLayoutBinding b{};
  b.descriptorCount = 1;
  b.descriptorType = vk::DescriptorType::eUniformBuffer;
  b.stageFlags = vk::ShaderStageFlagBits::eVertex;
  return b;
}

// === Class definitions
static const vk::BufferUsageFlags UNIFORM_USAGE_FLAGS =
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;
static const vk::MemoryPropertyFlags UNIFORM_MEM_FLAGS =
    vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent;

vk::WriteDescriptorSet writeBinding(const vk::raii::DescriptorSet &set,
                                    const vk::DescriptorBufferInfo &buffer,
                                    int binding)
{
  vk::WriteDescriptorSet descWrite{};
  descWrite.dstSet = *set;
  descWrite.dstBinding = binding;
  descWrite.dstArrayElement = 0;
  descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  descWrite.descriptorCount = 1;
  descWrite.pBufferInfo = &buffer;
  return descWrite;
}

GlobalUniform::GlobalUniform(const Renderer &renderer) :
    m_renderer(std::addressof(renderer)),
    m_layout(nullptr),
    m_projection{glm::mat4(0.0f), glm::mat4(0.0f)},
    m_projectionBuffer(renderer.device(),
                       UNIFORM_USAGE_FLAGS,
                       sizeof(ProjectionUniform),
                       UNIFORM_MEM_FLAGS,
                       true),
    m_projectionInfo(*m_projectionBuffer.buffer(), 0, sizeof(ProjectionUniform))
{
  std::array<vk::DescriptorSetLayoutBinding, BINDINGS> a = {ProjectionUniform::binding()};
  for (size_t i = 0; i < a.size(); i++) a[i].binding = i;

  vk::DescriptorSetLayoutCreateInfo info{};
  info.setBindings(a);
  m_layout = vk::raii::DescriptorSetLayout(renderer.device().logical(), info);

  m_cacheInfos.reserve(BINDINGS);
  m_cacheInfos.push_back(m_projectionInfo);
}

void GlobalUniform::updateIfDirty(const FrameHandle &handle) const
{
  auto &set = m_renderer->getDescriptorSet(handle, *m_layout, bufferInfos(), {});
  if (m_dirty) {
    // Update buffers
    // TODO: lighting
    m_projectionBuffer.load(&m_projection, 0, sizeof(ProjectionUniform), {});

    // Write
    // TODO: lighting
    std::array<vk::WriteDescriptorSet, BINDINGS> writes;
    writes[0] = writeBinding(set, m_projectionInfo, 0);

    m_renderer->device().logical().updateDescriptorSets(writes, {});
  }
  m_dirty = false;
}
