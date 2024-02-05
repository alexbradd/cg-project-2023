#include <seng/log.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/pipeline.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/renderer.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

using namespace seng::rendering;
using namespace std;

static vk::raii::Pipeline createPipeline(const Renderer& renderer,
                                         const RenderPass& pass,
                                         const vk::raii::PipelineLayout& layout,
                                         const Pipeline::CreateInfo& info);

Pipeline::Pipeline(std::nullptr_t) : m_layout(nullptr), m_pipeline(nullptr) {}

Pipeline::Pipeline(const Renderer& renderer, const RenderPass& pass, CreateInfo info) :
    m_layout(std::invoke([&]() {
      vk::PipelineLayoutCreateInfo layoutInfo{};

      // Push constants
      vk::PushConstantRange pushConstant{};
      pushConstant.stageFlags = vk::ShaderStageFlagBits::eVertex;
      pushConstant.offset = 0;
      pushConstant.size = sizeof(PushConstants);
      layoutInfo.setPushConstantRanges(pushConstant);

      // Layouts
      // Not using setLayouts since it breaks if passed a vector<>&
      layoutInfo.setLayoutCount = info.descriptorSetLayouts.size();
      layoutInfo.pSetLayouts = info.descriptorSetLayouts.data();
      return vk::raii::PipelineLayout(renderer.device().logical(), layoutInfo);
    })),
    m_pipeline(createPipeline(renderer, pass, m_layout, info))
{
  log::dbg("Created pipeline");
}

static vk::raii::Pipeline createPipeline(const Renderer& renderer,
                                         const RenderPass& pass,
                                         const vk::raii::PipelineLayout& layout,
                                         const Pipeline::CreateInfo& info)
{
  // Fixed part of the pipeline
  vk::PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;  // We are sing dynamic states for this
  viewportState.scissorCount = 1;   // We are using dynamic states for this

  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.depthClampEnable = false;
  rasterizer.rasterizerDiscardEnable = false;
  rasterizer.polygonMode =
      info.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace = vk::FrontFace::eClockwise;
  rasterizer.depthBiasEnable = false;

  vk::PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sampleShadingEnable = false;
  multisampling.rasterizationSamples = renderer.samples();

  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.depthTestEnable = true;
  depthStencil.depthWriteEnable = true;
  depthStencil.depthCompareOp = vk::CompareOp::eLess;
  depthStencil.depthBoundsTestEnable = false;
  depthStencil.stencilTestEnable = false;

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.blendEnable = true;
  colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
  colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
  colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
  colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  vk::PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.logicOpEnable = false;
  colorBlending.logicOp = vk::LogicOp::eCopy;
  colorBlending.setAttachments(colorBlendAttachment);

  // Dynamic states
  vk::PipelineDynamicStateCreateInfo dynamicState{};
  array<vk::DynamicState, 3> dynamicStates = {vk::DynamicState::eViewport,
                                              vk::DynamicState::eScissor,
                                              vk::DynamicState::eLineWidth};
  dynamicState.setDynamicStates(dynamicStates);

  // Vertex input
  vk::VertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = vk::VertexInputRate::eVertex;

  // Attributes
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
  vertexInputInfo.setVertexAttributeDescriptions(info.attributes);

  // Input Assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = false;

  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.setStages(info.stages);
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = *layout;
  pipelineInfo.renderPass = *pass.handle();
  pipelineInfo.subpass = 0;

  return vk::raii::Pipeline(renderer.device().logical(), nullptr, pipelineInfo);
}

void Pipeline::bind(const CommandBuffer& buffer, vk::PipelineBindPoint bind) const
{
  buffer.buffer().bindPipeline(bind, *m_pipeline);
}

Pipeline::~Pipeline()
{
  if (*m_pipeline != vk::Pipeline{}) log::dbg("Destroying pipeline");
}
