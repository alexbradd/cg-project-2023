#include <cstdint>
#include <functional>
#include <optional>
#include <seng/log.hpp>
#include <seng/vulkan_debug.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_fence.hpp>
#include <seng/vulkan_swapchain.hpp>
#include <stdexcept>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

VulkanSwapchain::VulkanSwapchain(VulkanDevice &dev,
                                 SurfaceKHR &surface,
                                 GlfwWindow &window) :
    vkDevRef(dev),
    _format(dev.swapchainSupportDetails().chooseFormat()),
    _extent(dev.swapchainSupportDetails().chooseSwapchainExtent(window)),
    // === Create swapchain
    _swapchain(std::invoke([&]() {
      QueueFamilyIndices indices(vkDevRef.get().queueFamiliyIndices());
      vk::SurfaceCapabilitiesKHR capabilities(
          vkDevRef.get().swapchainSupportDetails().capabilities());

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
      return SwapchainKHR(vkDevRef.get().logical(), sci);
    })),
    _images(_swapchain.getImages()),
    // === Create ImageViews
    _imageViews(std::invoke([&]() {
      vector<ImageView> ret;
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
      VulkanImage::CreateInfo ci{vk::ImageType::e2D,
                                 _extent.width,
                                 _extent.height,
                                 vkDevRef.get().depthFormat().format,
                                 vk::ImageTiling::eOptimal,
                                 vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                                 vk::ImageAspectFlagBits::eDepth,
                                 true};
      return VulkanImage(vkDevRef.get(), ci);
    }))
{
  log::dbg("Swapchain created with extent {}x{}", _extent.width, _extent.height);
}

uint32_t VulkanSwapchain::nextImageIndex(Semaphore &imgAvailable,
                                         optional<reference_wrapper<VulkanFence>> fence,
                                         uint64_t timeout)
{
  optional<pair<vk::Result, uint32_t>> res;
  if (fence.has_value()) {
    res = _swapchain.acquireNextImage(timeout, *imgAvailable,
                                      *fence.value().get().handle());
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
                 resultToString(res->first);
      throw runtime_error(s);
  }
}

void VulkanSwapchain::present(Queue &presentQueue,
                              Queue &,
                              Semaphore &renderComplete,
                              uint32_t imageIndex)
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
          string("Failed to present swapchain image! Error: ") + resultToString(res);
      throw runtime_error(s);
  }
}

void VulkanSwapchain::recreate(VulkanSwapchain &loc,
                               VulkanDevice &dev,
                               SurfaceKHR &surface,
                               GlfwWindow &window)
{
  loc.~VulkanSwapchain();
  ::new (&loc) VulkanSwapchain(dev, surface, window);
}

VulkanSwapchain::~VulkanSwapchain()
{
  if (*_swapchain != vk::SwapchainKHR{}) {
    vkDevRef.get().logical().waitIdle();
    log::dbg("Destroying swapchain");
  }
}
