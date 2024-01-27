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

vk::DescriptorSetLayoutBinding LightingUniform::binding()
{
  vk::DescriptorSetLayoutBinding b{};
  b.descriptorCount = 1;
  b.descriptorType = vk::DescriptorType::eUniformBuffer;
  b.stageFlags = vk::ShaderStageFlagBits::eFragment;
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
    m_light{},
    m_lightBuffer(nullptr)
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
    m_light{glm::vec4(0.0f), glm::vec3(0.0f), glm::vec4(0.0f), glm::vec3(0.0f)},
    m_lightBuffer(renderer.device(),
                  UNIFORM_USAGE_FLAGS,
                  renderer.framesInFlight() * sizeof(LightingUniform),
                  UNIFORM_MEM_FLAGS,
                  true)
{
  std::array<vk::DescriptorSetLayoutBinding, BINDINGS> a = {ProjectionUniform::binding(),
                                                            LightingUniform::binding()};
  for (size_t i = 0; i < a.size(); i++) a[i].binding = i;

  vk::DescriptorSetLayoutCreateInfo info{};
  info.setBindings(a);
  m_layout = vk::raii::DescriptorSetLayout(renderer.device().logical(), info);

  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(BINDINGS * renderer.framesInFlight());
  m_bufferInfos.reserve(renderer.framesInFlight());
  for (size_t i = 0; i < renderer.framesInFlight(); i++) {
    m_bufferInfos.emplace_back();
    m_bufferInfos[i].reserve(BINDINGS);

    vk::DescriptorBufferInfo projInfo{*m_projectionBuffer.buffer(),
                                      i * sizeof(ProjectionUniform),
                                      sizeof(ProjectionUniform)};
    m_bufferInfos[i].push_back(projInfo);

    vk::DescriptorBufferInfo lightInfo{
        *m_lightBuffer.buffer(), i * sizeof(LightingUniform), sizeof(LightingUniform)};
    m_bufferInfos[i].push_back(lightInfo);

    renderer.requestDescriptorSet(i, *m_layout, m_bufferInfos[i], {});
    auto set = *renderer.getDescriptorSet(i, *m_layout, m_bufferInfos[i], {});

    vk::WriteDescriptorSet projWrite{};
    projWrite.dstSet = set;
    projWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    projWrite.dstBinding = 0;
    projWrite.dstArrayElement = 0;
    projWrite.descriptorCount = 1;
    projWrite.setBufferInfo(m_bufferInfos[i][0]);
    writes.push_back(projWrite);

    vk::WriteDescriptorSet lightWrite{};
    lightWrite.dstSet = set;
    lightWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    lightWrite.dstBinding = 1;
    lightWrite.dstArrayElement = 0;
    lightWrite.descriptorCount = 1;
    lightWrite.setBufferInfo(m_bufferInfos[i][1]);
    writes.push_back(lightWrite);
  }
  renderer.device().logical().updateDescriptorSets(writes, {});
}

void GlobalUniform::update(const FrameHandle &handle) const
{
  m_projectionBuffer.load(&m_projection, handle.asIndex() * sizeof(ProjectionUniform),
                          sizeof(ProjectionUniform), {});
  m_lightBuffer.load(&m_lightBuffer, handle.asIndex() * sizeof(LightingUniform),
                     sizeof(LightingUniform), {});
}

const std::vector<vk::DescriptorBufferInfo> &GlobalUniform::bufferInfos(
    FrameHandle frame) const
{
  return m_bufferInfos[frame.asIndex()];
}
