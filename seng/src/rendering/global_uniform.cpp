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

GlobalUniform::GlobalUniform(std::nullptr_t) :
    m_renderer(nullptr),
    m_layout(nullptr),
    m_projection{},
    m_projectionBuffer(nullptr),
    m_projectionInfos{}
{
}

GlobalUniform::GlobalUniform(Renderer &renderer) :
    m_renderer(std::addressof(renderer)),
    m_layout(nullptr),
    m_projection{glm::mat4(0.0f), glm::mat4(0.0f)},
    m_projectionBuffer(renderer.device(),
                       UNIFORM_USAGE_FLAGS,
                       renderer.framesInFlight() * sizeof(ProjectionUniform),
                       UNIFORM_MEM_FLAGS,
                       true),
    m_projectionInfos{}
{
  std::array<vk::DescriptorSetLayoutBinding, BINDINGS> a = {ProjectionUniform::binding()};
  for (size_t i = 0; i < a.size(); i++) a[i].binding = i;

  vk::DescriptorSetLayoutCreateInfo info{};
  info.setBindings(a);
  m_layout = vk::raii::DescriptorSetLayout(renderer.device().logical(), info);

  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(renderer.framesInFlight());

  m_projectionInfos.reserve(renderer.framesInFlight());
  // TODO: lighting buffer

  for (size_t i = 0; i < renderer.framesInFlight(); i++) {
    vk::DescriptorBufferInfo info{*m_projectionBuffer.buffer(),
                                  i * sizeof(ProjectionUniform),
                                  sizeof(ProjectionUniform)};
    m_projectionInfos.push_back(info);
    // TODO: lighting buffer

    renderer.requestDescriptorSet(i, *m_layout, {info}, {});

    vk::WriteDescriptorSet write{};
    write.dstSet = *renderer.getDescriptorSet(i, *m_layout, {info}, {});
    write.descriptorType = vk::DescriptorType::eUniformBuffer;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.setBufferInfo(m_projectionInfos[i]);
    writes.push_back(write);
  }

  renderer.device().logical().updateDescriptorSets(writes, {});
}

void GlobalUniform::update(const FrameHandle &handle) const
{
  // Update buffers
  m_projectionBuffer.load(&m_projection, handle.asIndex() * sizeof(ProjectionUniform),
                          sizeof(ProjectionUniform), {});
  // TODO: lighting
}

const std::vector<vk::DescriptorBufferInfo> GlobalUniform::bufferInfos(
    FrameHandle frame) const
{
  std::vector<vk::DescriptorBufferInfo> infos(BINDINGS);
  infos[0] = m_projectionInfos[frame.asIndex()];
  return infos;
}
