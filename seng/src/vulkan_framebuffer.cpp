#include <seng/vulkan_device.hpp>
#include <seng/vulkan_framebuffer.hpp>
#include <seng/vulkan_render_pass.hpp>

using namespace seng::rendering;
using namespace vk::raii;
using namespace std;

static Framebuffer create(VulkanDevice &dev,
                          VulkanRenderPass &pass,
                          vk::Extent2D size,
                          vector<vk::ImageView> &attachments);

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice &dev,
                                     VulkanRenderPass &pass,
                                     vk::Extent2D size,
                                     vector<vk::ImageView> &attachments)
    : vkDevRef(dev),
      vkRenderPass(pass),
      attachments(attachments),
      _handle(create(dev, pass, size, attachments)) {}

Framebuffer create(VulkanDevice &dev,
                   VulkanRenderPass &pass,
                   vk::Extent2D size,
                   vector<vk::ImageView> &attachments) {
  vk::FramebufferCreateInfo info{};
  info.renderPass = *pass.handle();
  info.attachmentCount = attachments.size();
  info.pAttachments = attachments.data();
  info.width = size.width;
  info.height = size.height;
  info.layers = 1;

  return Framebuffer(dev.logical(), info);
}
