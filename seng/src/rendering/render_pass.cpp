#include <seng/log.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/swapchain.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

using namespace std;
using namespace seng::rendering;

static vk::raii::RenderPass createRenderPass(const Device &device,
                                             const vector<Attachment> &attachments);

RenderPass::RenderPass(std::nullptr_t) : m_attachments(), m_renderPass(nullptr) {}

RenderPass::RenderPass(const Device &device, std::vector<Attachment> attachments) :
    m_attachments(std::move(attachments)),
    m_renderPass(createRenderPass(device, this->m_attachments))
{
}

vk::raii::RenderPass createRenderPass(const Device &device,
                                      const vector<Attachment> &attachments)
{
  array<vk::SubpassDescription, 1> subpasses;

  // Main subpass
  auto &subpass = subpasses[0];
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

  vector<vk::AttachmentDescription> descriptions;
  descriptions.reserve(attachments.size());

  vector<vk::AttachmentReference> colorAttachments;
  optional<vk::AttachmentReference> depthAttachment;
  // TODO: other types of attachments

  for (size_t i = 0; i < attachments.size(); i++) {
    auto &a = attachments[i];
    descriptions.emplace_back(vk::AttachmentDescriptionFlags{}, a.format, a.samples,
                              a.loadOp, a.storeOp, a.stencilLoadOp, a.stencilStoreOp,
                              a.initialLayout, a.finalLayout);
    switch (a.usage) {
      case vk::ImageLayout::eColorAttachmentOptimal:
        colorAttachments.emplace_back(i, vk::ImageLayout::eColorAttachmentOptimal);
        break;
      case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        depthAttachment =
            vk::AttachmentReference(i, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        break;
      default:
        seng::log::warning("Attachment type not supported");
    }
  }

  // Color attachment
  subpass.setColorAttachments(colorAttachments);
  if (depthAttachment.has_value())
    subpass.setPDepthStencilAttachment(&(*depthAttachment));
  // TODO: other attachment types

  array<vk::SubpassDependency, 1> dependencies;
  auto &dep = dependencies[0];
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dep.srcAccessMask = vk::AccessFlagBits::eNone;
  dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                      vk::AccessFlagBits::eColorAttachmentWrite;

  vk::RenderPassCreateInfo info{};
  info.setAttachments(descriptions).setSubpasses(subpasses).setDependencies(dependencies);
  return vk::raii::RenderPass(device.logical(), info);
}

void RenderPass::begin(const CommandBuffer &buf,
                       const Framebuffer &fb,
                       vk::Extent2D extent,
                       vk::Offset2D offset) const
{
  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = *m_renderPass;
  renderPassInfo.framebuffer = *fb.handle();
  renderPassInfo.renderArea.offset = offset;
  renderPassInfo.renderArea.extent = extent;

  std::vector<vk::ClearValue> clearValues(m_attachments.size());
  std::transform(m_attachments.begin(), m_attachments.end(), clearValues.begin(),
                 [](auto &a) { return a.clearValue; });
  renderPassInfo.setClearValues(clearValues);

  buf.buffer().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void RenderPass::end(const CommandBuffer &buf) const
{
  buf.buffer().endRenderPass();
}

RenderPass::~RenderPass()
{
  if (*m_renderPass != vk::RenderPass{}) log::dbg("Destroying render pass");
}
