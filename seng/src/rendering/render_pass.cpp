#include <seng/log.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/swapchain.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <string>
#include <vector>

using namespace std;
using namespace seng::rendering;

static vk::raii::RenderPass createRenderPass(const Device &device,
                                             vk::Format colorFormat,
                                             vk::Format depthFormat);

RenderPass::RenderPass(const Device &dev, const Swapchain &swap) :
    RenderPass(dev,
               swap.format().format,
               dev.depthFormat().format,
               {0, 0},
               swap.extent(),
               {0.0f, 0.0f, 1.0f, 0.0f},
               {1.0f, 0})
{
}

RenderPass::RenderPass(const Device &device,
                       vk::Format colorFormat,
                       vk::Format depthFormat,
                       vk::Offset2D offset,
                       vk::Extent2D extent,
                       vk::ClearColorValue clearColor,
                       vk::ClearDepthStencilValue clearDepth) :
    vulkanDev(std::addressof(device)),
    _pass(createRenderPass(device, colorFormat, depthFormat)),
    offset(offset),
    extent(extent),
    clearColor(clearColor),
    clearDepth(clearDepth)
{
}

vk::raii::RenderPass createRenderPass(const Device &device,
                                      vk::Format colorFormat,
                                      vk::Format depthFormat)
{
  // Main subpass
  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

  vector<vk::AttachmentDescription> descriptions;

  // Color attachment
  vk::AttachmentDescription colorAttachment{};
  colorAttachment.format = colorFormat,
  colorAttachment.samples = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  descriptions.push_back(colorAttachment);

  vk::AttachmentReference colorRef{0, vk::ImageLayout::eColorAttachmentOptimal};
  subpass.setColorAttachments(colorRef);

  // Depth attachment
  vk::AttachmentDescription depthAttachment{};
  depthAttachment.format = depthFormat,
  depthAttachment.samples = vk::SampleCountFlagBits::e1;
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  descriptions.push_back(depthAttachment);

  vk::AttachmentReference depthRef{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};
  subpass.setPDepthStencilAttachment(&depthRef);

  // TODO: other attachment types

  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.pResolveAttachments = nullptr;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;

  vk::SubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dep.srcAccessMask = vk::AccessFlagBits::eNone;
  dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                      vk::AccessFlagBits::eColorAttachmentWrite;

  vk::RenderPassCreateInfo info{};
  info.attachmentCount = descriptions.size();
  info.pAttachments = descriptions.data();
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dep;
  return vk::raii::RenderPass(device.logical(), info);
}

void RenderPass::begin(const CommandBuffer &buf, const Framebuffer &fb) const
{
  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = *_pass;
  renderPassInfo.framebuffer = *fb.handle();
  renderPassInfo.renderArea.offset = offset;
  renderPassInfo.renderArea.extent = extent;

  array<vk::ClearValue, 2> clearValues{clearColor, clearDepth};
  renderPassInfo.setClearValues(clearValues);

  buf.buffer().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void RenderPass::end(const CommandBuffer &buf) const
{
  buf.buffer().endRenderPass();
}

vk::Viewport RenderPass::fullViewport() const
{
  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  return viewport;
}

vk::Rect2D RenderPass::fullScissor() const
{
  return vk::Rect2D{{0, 0}, extent};
}

void RenderPass::updateOffset(vk::Offset2D offset)
{
  this->offset = offset;
}

void RenderPass::updateExtent(vk::Extent2D extent)
{
  this->extent = extent;
}

RenderPass::~RenderPass()
{
  if (*_pass != vk::RenderPass{}) log::dbg("Destroying render pass");
}
