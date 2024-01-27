#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/shader_stage.hpp>

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <stddef.h>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace seng::rendering;
using namespace std;
using namespace vk::raii;

ObjectShader::ObjectShader(const Renderer& renderer,
                           const GlobalUniform& gubo,
                           // FIXME: samplers
                           string name,
                           vector<const ShaderStage*> stages) :
    m_renderer(std::addressof(renderer)),
    m_name(std::move(name)),
    m_stages(std::move(stages)),
    m_gubo(std::addressof(gubo)),
    m_pipeline(std::invoke([&]() {
      // Attributes
      AttributeDescriptions attributes{Vertex::attributeDescriptions()};

      // Descriptor layouts
      vector<vk::DescriptorSetLayout> descriptors;
      descriptors.emplace_back(gubo.layout());
      // TODO: Local descriptor layouts

      // Stages
      vector<vk::PipelineShaderStageCreateInfo> stageCreateInfo;
      for (size_t i = 0; i < ObjectShader::STAGES; i++) {
        stageCreateInfo.emplace_back(m_stages[i]->createInfo());
      }

      Pipeline::CreateInfo pipeInfo{attributes, descriptors, stageCreateInfo, false};
      return Pipeline(renderer.device(), renderer.renderPass(), pipeInfo);
    }))
{
  log::dbg("Created object shader {}", name);
}

void ObjectShader::use(const CommandBuffer& buffer) const
{
  m_pipeline.bind(buffer, vk::PipelineBindPoint::eGraphics);
}

void ObjectShader::bindDescriptorSets(const FrameHandle& handle,
                                      const CommandBuffer& buf) const
{
  std::vector<vk::DescriptorSet> sets;
  sets.reserve(1);  // FIXME: + samplers.size

  // Get GUBO's set
  auto guboSet = m_renderer->getDescriptorSet(handle, m_gubo->layout(),
                                              m_gubo->bufferInfos(handle), {});
  if (guboSet == nullptr) throw runtime_error("Null GUBO descriptor set");
  sets.emplace_back(guboSet);

  // Get the sets for each sampler
  // FIXME: samplers

  // Bind
  buf.buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline.layout(),
                                  0, sets, {});
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
