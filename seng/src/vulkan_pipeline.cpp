#include <seng/log.hpp>
#include <seng/primitive_types.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_pipeline.hpp>
#include <seng/vulkan_render_pass.hpp>

using namespace seng::rendering;
using namespace vk::raii;
using namespace std;

static PipelineLayout createPipelineLayout(VulkanDevice& dev,
                                           VulkanPipeline::CreateInfo& info);
static Pipeline createPipeline(VulkanDevice& dev,
                               VulkanRenderPass& pass,
                               PipelineLayout& layout,
                               VulkanPipeline::CreateInfo& info);

VulkanPipeline::VulkanPipeline(VulkanDevice& device,
                               VulkanRenderPass& pass,
                               CreateInfo info)
    : vkDevRef(device),
      vkRenderPassRef(pass),
      pipelineLayout(createPipelineLayout(vkDevRef, info)),
      pipeline(
          createPipeline(vkDevRef, vkRenderPassRef, pipelineLayout, info)) {
  log::dbg("Created pipeline");
}

static PipelineLayout createPipelineLayout(VulkanDevice& dev,
                                           VulkanPipeline::CreateInfo& info) {
  vector<vk::DescriptorSetLayout> layouts{};
  for (const auto& l : info.descriptorSetLayouts) layouts.emplace_back(*l);

  vk::PipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.setSetLayouts(layouts);
  return PipelineLayout(dev.logical(), layoutInfo);
}

static Pipeline createPipeline(VulkanDevice& dev,
                               VulkanRenderPass& pass,
                               PipelineLayout& layout,
                               VulkanPipeline::CreateInfo& info) {
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
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable = false;

  vk::PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sampleShadingEnable = false;
  multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

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
  array<vk::DynamicState, 3> dynamicStates{vk::DynamicState::eViewport,
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

  return Pipeline(dev.logical(), nullptr, pipelineInfo);
}

void VulkanPipeline::bind(VulkanCommandBuffer& buffer,
                          vk::PipelineBindPoint bind) {
  buffer.buffer().bindPipeline(bind, *pipeline);
}

VulkanPipeline::~VulkanPipeline() {
  if (*pipeline != vk::Pipeline{}) log::dbg("Destroying pipeline");
}
