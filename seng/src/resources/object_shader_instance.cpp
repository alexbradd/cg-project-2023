#include <seng/log.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/resources/object_shader.hpp>
#include <seng/resources/object_shader_instance.hpp>

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>

using namespace seng;

ObjectShaderInstance::ObjectShaderInstance(rendering::Renderer& renderer,
                                           const ObjectShader& shader,
                                           std::string name,
                                           std::vector<std::string> textures) :
    m_renderer(std::addressof(renderer)),
    m_shader(std::addressof(shader)),
    m_name(std::move(name)),
    m_texturePaths(std::move(textures))
{
  if (m_texturePaths.size() < m_shader->textureLayout().size())
    throw std::runtime_error("Not enought textures supplied");
  if (m_texturePaths.size() > m_shader->textureLayout().size())
    seng::log::warning("Too many textures supplied, ignoring excess");

  // Load textures (if needed) and create descriptor information
  seng::log::dbg("Loading necessary textures for instance {}", m_name);
  auto& texLayout = m_shader->textureLayout();
  m_imgInfos.reserve(texLayout.size());
  for (size_t i = 0; i < texLayout.size(); i++) {
    auto& tex = m_renderer->requestTexture(m_texturePaths[i], texLayout[i]);
    vk::DescriptorImageInfo info{};
    info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    info.imageView = tex.image().imageView();
    info.sampler = tex.sampler();
    m_imgInfos.push_back(info);
  }

  // Create and write descriptors if there are any textures
  if (m_imgInfos.size() > 0) {
    seng::log::dbg("Allocating descriptors for instance {}", m_name);
    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(m_imgInfos.size());
    for (size_t frame = 0; frame < m_renderer->framesInFlight(); frame++) {
      vk::DescriptorSet set = m_renderer->requestDescriptorSet(
          frame, m_shader->textureSetLayout(), {}, m_imgInfos);
      for (size_t tex = 0; tex < m_imgInfos.size(); tex++) {
        vk::WriteDescriptorSet write{};
        write.dstSet = set;
        write.dstBinding = tex;
        write.dstArrayElement = 0;
        write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        write.descriptorCount = 1;
        write.pImageInfo = &m_imgInfos[tex];
        writes.push_back(write);
      }
    }
    m_renderer->device().logical().updateDescriptorSets(writes, {});
  }
  seng::log::dbg("Instance {} of {} created", m_shader->name(), m_name);
}

void ObjectShaderInstance::bindDescriptorSets(const rendering::FrameHandle& handle,
                                              const rendering::CommandBuffer& buf) const
{
  std::vector<vk::DescriptorSet> sets;
  sets.reserve(2);

  // Get GUBO's set
  auto& gubo = m_renderer->globalUniform();
  auto guboSet =
      m_renderer->getDescriptorSet(handle, gubo.layout(), gubo.bufferInfos(handle), {});
  if (guboSet == nullptr) throw std::runtime_error("Null GUBO descriptor set");
  sets.emplace_back(guboSet);

  // Get texure set, if any textures are present
  if (m_imgInfos.size() > 0) {
    auto texSet = m_renderer->getDescriptorSet(handle, m_shader->textureSetLayout(), {},
                                               m_imgInfos);
    if (texSet == nullptr) throw std::runtime_error("Null texture descriptor set");
    sets.emplace_back(texSet);
  }

  // Bind
  m_shader->bindDescriptorSets(buf, sets);
}

void ObjectShaderInstance::updateModelState(const rendering::CommandBuffer& buf,
                                            const glm::mat4& model) const
{
  m_shader->updateModelState(buf, model);
}
