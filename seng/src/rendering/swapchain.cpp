#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/swapchain.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace seng::rendering;

Swapchain::Swapchain(const Device &dev,
                     const vk::raii::SurfaceKHR &surface,
                     const GlfwWindow &window,
                     const vk::raii::SwapchainKHR &old) :
    m_device(std::addressof(dev)),
    m_format(dev.swapchainSupportDetails().chooseFormat()),
    m_extent(dev.swapchainSupportDetails().chooseExtent(window)),
    // === Create swapchain
    m_swapchain(std::invoke([&]() {
      QueueFamilyIndices indices(dev.queueFamilyIndices());
      vk::SurfaceCapabilitiesKHR capabilities(dev.swapchainSupportDetails().capabilities);

      vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
      uint32_t imageCount = capabilities.minImageCount + 1;
      if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

      vk::SwapchainCreateInfoKHR sci{};
      sci.surface = *surface;
      sci.minImageCount = imageCount;
      sci.imageFormat = m_format.format;
      sci.imageColorSpace = m_format.colorSpace;
      sci.imageExtent = m_extent;
      sci.imageArrayLayers = 1;
      sci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
      sci.preTransform = capabilities.currentTransform;
      sci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
      sci.presentMode = presentMode;
      sci.clipped = true;
      sci.oldSwapchain = *old;

      if (indices.graphicsFamily != indices.presentFamily) {
        array<uint32_t, 2> queueFamilyIndices = {*indices.graphicsFamily,
                                                 *indices.presentFamily};
        sci.imageSharingMode = vk::SharingMode::eConcurrent;
        sci.setQueueFamilyIndices(queueFamilyIndices);
      } else {
        sci.imageSharingMode = vk::SharingMode::eExclusive;
        sci.setQueueFamilyIndices({});
      }
      return vk::raii::SwapchainKHR(dev.logical(), sci);
    })),
    m_images()
{
  std::vector<vk::Image> raw{m_swapchain.getImages()};
  m_images.reserve(raw.size());
  for (auto &i : raw) {
    Image img(dev, i);

    vk::ImageViewCreateInfo ci;
    ci.image = i;
    ci.viewType = vk::ImageViewType::e2D;
    ci.format = m_format.format;
    ci.components.r = vk::ComponentSwizzle::eIdentity;
    ci.components.g = vk::ComponentSwizzle::eIdentity;
    ci.components.b = vk::ComponentSwizzle::eIdentity;
    ci.components.a = vk::ComponentSwizzle::eIdentity;
    ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = 1;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;
    img.stealView(vk::raii::ImageView(dev.logical(), ci));

    m_images.push_back(std::move(img));
  }
  log::dbg("Swapchain created with extent {}x{}", m_extent.width, m_extent.height);
}

Swapchain::~Swapchain()
{
  if (*m_swapchain != vk::SwapchainKHR{}) {
    m_device->logical().waitIdle();
    log::dbg("Destroying swapchain");
  }
}
