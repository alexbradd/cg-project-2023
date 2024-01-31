#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/resources/object_shader.hpp>
#include <seng/resources/shader_stage.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace seng;
using namespace seng::rendering;

ObjectShader::ObjectShader(Renderer& renderer,
                           std::string name,
                           std::vector<TextureType> textures,
                           const std::vector<const ShaderStage*>& stages) :
    m_renderer(std::addressof(renderer)),
    m_name(std::move(name)),
    m_texLayout(std::move(textures)),
    m_texSetLayout(nullptr),
    m_pipeline(nullptr)
{
  // Create texture descriptor set, if any present
  if (m_texLayout.size() > 0) {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(m_texLayout.size());
    for (size_t i = 0; i < m_texLayout.size(); i++) {
      vk::DescriptorSetLayoutBinding texBinding;
      texBinding.binding = i;
      texBinding.descriptorCount = 1;
      texBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
      texBinding.pImmutableSamplers = nullptr;
      texBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
      bindings.push_back(texBinding);
    }
    vk::DescriptorSetLayoutCreateInfo textureSetInfo;
    textureSetInfo.setBindings(bindings);
    m_texSetLayout = renderer.requestDescriptorSetLayout(textureSetInfo);
  }

  // === Pipeline creation
  // Attributes
  AttributeDescriptions attributes{Vertex::attributeDescriptions()};

  // Descriptor layouts
  std::vector<vk::DescriptorSetLayout> descriptors;
  descriptors.reserve(STAGES);
  descriptors.emplace_back(renderer.globalUniform().layout());
  if (m_texSetLayout != nullptr) descriptors.emplace_back(m_texSetLayout);

  // Stages
  std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfo;
  stageCreateInfo.reserve(stages.size());
  for (size_t i = 0; i < ObjectShader::STAGES; i++)
    stageCreateInfo.emplace_back(stages[i]->stageCreateInfo());

  Pipeline::CreateInfo pipeInfo{attributes, descriptors, stageCreateInfo, false};
  m_pipeline = Pipeline(renderer.device(), renderer.renderPass(), pipeInfo);
  log::dbg("Created object shader {}", name);
}

void ObjectShader::use(const CommandBuffer& buffer) const
{
  m_pipeline.bind(buffer, vk::PipelineBindPoint::eGraphics);
}

void ObjectShader::bindDescriptorSets(const rendering::CommandBuffer& buf,
                                      const std::vector<vk::DescriptorSet>& sets) const
{
  buf.buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline.layout(),
                                  0, sets, {});
}

void ObjectShader::updateModelState(const CommandBuffer& buf,
                                    const glm::mat4& model) const
{
  buf.buffer().pushConstants<glm::mat4>(*m_pipeline.layout(),
                                        vk::ShaderStageFlagBits::eVertex,
                                        offsetof(PushConstants, modelMatrix), model);
}

ObjectShader::~ObjectShader()
{
  // Since all handles are either all valid or all invalid, we simply check one
  // of them to see if we have been moved from
  if (*m_pipeline.handle() != vk::Pipeline(nullptr)) log::dbg("Destroying object shader");
}
