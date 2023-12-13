#include <seng/log.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_framebuffer.hpp>
#include <seng/vulkan_render_pass.hpp>
#include <seng/vulkan_swapchain.hpp>

using namespace seng::rendering;
using namespace vk::raii;
using namespace std;

VulkanFramebuffer::VulkanFramebuffer(const VulkanDevice &dev,
                                     const VulkanRenderPass &pass,
                                     vk::Extent2D size,
                                     const vector<vk::ImageView> &attachments) :
    vulkanDev(std::addressof(dev)),
    vulkanRenderPass(std::addressof(pass)),
    attachments(attachments),
    _handle(std::invoke([&]() {
      vk::FramebufferCreateInfo info{};
      info.renderPass = *pass.handle();
      info.setAttachments(attachments);
      info.width = size.width;
      info.height = size.height;
      info.layers = 1;

      return Framebuffer(dev.logical(), info);
    }))
{
  log::dbg("Framebuffer created with size {}x{}", size.width, size.height);
}

vector<VulkanFramebuffer> VulkanFramebuffer::fromSwapchain(const VulkanDevice &dev,
                                                           const VulkanRenderPass &pass,
                                                           const VulkanSwapchain &swap)
{
  vector<VulkanFramebuffer> fbs{};
  fbs.reserve(swap.images().size());

  for (auto &img : swap.images()) {
    vector<vk::ImageView> attachments(2);  // TODO: make configurable
    attachments[0] = *img;
    attachments[1] = **swap.depthBuffer().imageView();
    fbs.emplace_back(dev, pass, swap.extent(), attachments);
  }
  return fbs;
}

VulkanFramebuffer::~VulkanFramebuffer()
{
  if (*_handle != vk::Framebuffer{}) log::dbg("Destroying framebuffer");
}
