#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/image.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/swapchain.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <string>
#include <vector>

using namespace seng::rendering;
using namespace std;

Framebuffer::Framebuffer(const Device &dev,
                         const RenderPass &pass,
                         vk::Extent2D size,
                         const vector<vk::ImageView> &attachments) :
    m_device(std::addressof(dev)),
    m_renderPass(std::addressof(pass)),
    m_attachments(attachments),
    m_handle(std::invoke([&]() {
      vk::FramebufferCreateInfo info{};
      info.renderPass = *pass.handle();
      info.setAttachments(attachments);
      info.width = size.width;
      info.height = size.height;
      info.layers = 1;

      return vk::raii::Framebuffer(dev.logical(), info);
    }))
{
  log::dbg("Framebuffer created with size {}x{}", size.width, size.height);
}

vector<Framebuffer> Framebuffer::fromSwapchain(const Device &dev,
                                               const RenderPass &pass,
                                               const Swapchain &swap,
                                               const Image &depthBuffer)
{
  vector<Framebuffer> fbs{};
  fbs.reserve(swap.images().size());

  for (auto &img : swap.images()) {
    vector<vk::ImageView> attachments(2);  // TODO: make configurable
    attachments[0] = img.imageView();
    attachments[1] = depthBuffer.imageView();
    fbs.emplace_back(dev, pass, swap.extent(), attachments);
  }
  return fbs;
}

Framebuffer::~Framebuffer()
{
  if (*m_handle != vk::Framebuffer{}) log::dbg("Destroying framebuffer");
}
