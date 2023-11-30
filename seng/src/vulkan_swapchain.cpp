#include <seng/vulkan_device.hpp>
#include <seng/vulkan_swapchain.hpp>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

static SwapchainKHR createSwapchain(VulkanDevice &,
                                    vk::SurfaceFormatKHR &,
                                    vk::Extent2D &,
                                    SurfaceKHR &);
static vector<ImageView> createImageViews(VulkanDevice &,
                                          vk::SurfaceFormatKHR &,
                                          vector<vk::Image> &);
static VulkanImage createDepth(VulkanDevice &dev, vk::Extent2D extent);

VulkanSwapchain::VulkanSwapchain(VulkanDevice &dev,
                                 SurfaceKHR &surface,
                                 GlfwWindow &window)
    : vkDevRef(dev),
      _format(dev.swapchainSupportDetails().chooseFormat()),
      _extent(dev.swapchainSupportDetails().chooseSwapchainExtent(window)),
      _swapchain(createSwapchain(vkDevRef, _format, _extent, surface)),
      _images(_swapchain.getImages()),
      _imageViews(createImageViews(vkDevRef, _format, _images)),
      _depthBufferImage(createDepth(dev, _extent)) {}

SwapchainKHR createSwapchain(VulkanDevice &device,
                             vk::SurfaceFormatKHR &format,
                             vk::Extent2D &extent,
                             SurfaceKHR &surface) {
  QueueFamilyIndices indices(device.queueFamiliyIndices());
  vk::SurfaceCapabilitiesKHR capabilities(
      device.swapchainSupportDetails().capabilities());

  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    imageCount = capabilities.maxImageCount;

  vk::SwapchainCreateInfoKHR sci{};
  sci.surface = *surface;
  sci.minImageCount = imageCount;
  sci.imageFormat = format.format;
  sci.imageColorSpace = format.colorSpace;
  sci.imageExtent = extent;
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

  return SwapchainKHR(device.logical(), sci);
}

vector<ImageView> createImageViews(VulkanDevice &dev,
                                   vk::SurfaceFormatKHR &format,
                                   vector<vk::Image> &images) {
  vector<ImageView> ret;
  ret.reserve(images.size());
  for (auto image : images) {
    vk::ImageViewCreateInfo ci;
    ci.image = image;
    ci.viewType = vk::ImageViewType::e2D;
    ci.format = format.format;
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
}

VulkanImage createDepth(VulkanDevice &dev, vk::Extent2D extent) {
  VulkanImage::CreateInfo ci{vk::ImageType::e2D,
                             extent.width,
                             extent.height,
                             dev.depthFormat().format,
                             vk::ImageTiling::eOptimal,
                             vk::ImageUsageFlagBits::eDepthStencilAttachment,
                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                             vk::ImageAspectFlagBits::eDepth,
                             true};
  return VulkanImage(dev, ci);
}

void VulkanSwapchain::recreate(VulkanSwapchain &loc,
                               VulkanDevice &dev,
                               SurfaceKHR &surface,
                               GlfwWindow &window) {
  loc.~VulkanSwapchain();
  ::new (&loc) VulkanSwapchain(dev, surface, window);
}

VulkanSwapchain::~VulkanSwapchain() { vkDevRef.get().logical().waitIdle(); }
