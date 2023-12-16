#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/fence.hpp>
#include <seng/rendering/swapchain.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace seng::rendering;

Swapchain::Swapchain(const Device &dev,
                     const vk::raii::SurfaceKHR &surface,
                     const GlfwWindow &window) :
    vulkanDev(std::addressof(dev)),
    _format(dev.swapchainSupportDetails().chooseFormat()),
    _extent(dev.swapchainSupportDetails().chooseSwapchainExtent(window)),
    // === Create swapchain
    _swapchain(std::invoke([&]() {
      QueueFamilyIndices indices(dev.queueFamilyIndices());
      vk::SurfaceCapabilitiesKHR capabilities(
          dev.swapchainSupportDetails().capabilities());

      vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
      uint32_t imageCount = capabilities.minImageCount + 1;
      if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

      vk::SwapchainCreateInfoKHR sci{};
      sci.surface = *surface;
      sci.minImageCount = imageCount;
      sci.imageFormat = _format.format;
      sci.imageColorSpace = _format.colorSpace;
      sci.imageExtent = _extent;
      sci.imageArrayLayers = 1;
      sci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
      sci.preTransform = capabilities.currentTransform;
      sci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
      sci.presentMode = presentMode;
      sci.clipped = true;
      sci.oldSwapchain = VK_NULL_HANDLE;

      if (indices.graphicsFamily() != indices.presentFamily()) {
        array<uint32_t, 2> queueFamilyIndices{*indices.graphicsFamily(),
                                              *indices.presentFamily()};
        sci.imageSharingMode = vk::SharingMode::eConcurrent;
        sci.setQueueFamilyIndices(queueFamilyIndices);
      } else {
        sci.imageSharingMode = vk::SharingMode::eExclusive;
        sci.setQueueFamilyIndices({});
      }
      return vk::raii::SwapchainKHR(dev.logical(), sci);
    })),
    _images(_swapchain.getImages()),
    // === Create ImageViews
    _imageViews(std::invoke([&]() {
      vector<vk::raii::ImageView> ret;
      ret.reserve(_images.size());
      for (auto image : _images) {
        vk::ImageViewCreateInfo ci;
        ci.image = image;
        ci.viewType = vk::ImageViewType::e2D;
        ci.format = _format.format;
        ci.components.r = vk::ComponentSwizzle::eIdentity;
        ci.components.g = vk::ComponentSwizzle::eIdentity;
        ci.components.b = vk::ComponentSwizzle::eIdentity;
        ci.components.a = vk::ComponentSwizzle::eIdentity;
        ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        ci.subresourceRange.baseMipLevel = 0;
        ci.subresourceRange.levelCount = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount = 1;

        ret.emplace_back(dev.logical(), ci);
      }
      return ret;
    })),
    // === Create depth buffer
    _depthBufferImage(std::invoke([&]() {
      Image::CreateInfo ci{vk::ImageType::e2D,
                           _extent.width,
                           _extent.height,
                           dev.depthFormat().format,
                           vk::ImageTiling::eOptimal,
                           vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           vk::MemoryPropertyFlagBits::eDeviceLocal,
                           vk::ImageAspectFlagBits::eDepth,
                           true};
      return Image(dev, ci);
    }))
{
  log::dbg("Swapchain created with extent {}x{}", _extent.width, _extent.height);
}

uint32_t Swapchain::nextImageIndex(const vk::raii::Semaphore &imgAvailable,
                                   const Fence *fence,
                                   uint64_t timeout) const
{
  optional<pair<vk::Result, uint32_t>> res;
  if (fence != nullptr) {
    res = _swapchain.acquireNextImage(timeout, *imgAvailable, *fence->handle());
  } else {
    res = _swapchain.acquireNextImage(timeout, *imgAvailable);
  }
  switch (res->first) {
    case vk::Result::eSuccess:
    case vk::Result::eSuboptimalKHR:
      return res->second;
    case vk::Result::eErrorOutOfDateKHR:
      throw InadequateSwapchainException("Out of date swapchain", res->first);
    default:
      string s = string("Failed to present swapchain image! Error: ") +
                 vk::to_string(res->first);
      throw runtime_error(s);
  }
}

void Swapchain::present(const vk::raii::Queue &presentQueue,
                        const vk::raii::Queue &,
                        const vk::raii::Semaphore &renderComplete,
                        uint32_t imageIndex) const
{
  vk::PresentInfoKHR info{};
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &(*renderComplete);
  info.swapchainCount = 1;
  info.pSwapchains = &(*_swapchain);
  info.pImageIndices = &imageIndex;
  info.pResults = nullptr;

  auto res = presentQueue.presentKHR(info);
  switch (res) {
    case vk::Result::eSuccess:
      break;
    case vk::Result::eErrorOutOfDateKHR:
    case vk::Result::eSuboptimalKHR:
      throw InadequateSwapchainException("Out of date swapchain", res);
    default:
      string s =
          string("Failed to present swapchain image! Error: ") + vk::to_string(res);
      throw runtime_error(s);
  }
}

Swapchain::~Swapchain()
{
  if (*_swapchain != vk::SwapchainKHR{}) {
    vulkanDev->logical().waitIdle();
    log::dbg("Destroying swapchain");
  }
}
