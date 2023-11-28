#include <seng/glfw_window.hpp>
#include <seng/vulkan_swapchain_support_details.hpp>

using namespace seng::internal;
using namespace seng::rendering;
using namespace vk::raii;
using namespace std;

SwapchainSupportDetails::SwapchainSupportDetails(PhysicalDevice &dev,
                                                 SurfaceKHR &surface)
    : _capabilities(dev.getSurfaceCapabilitiesKHR(*surface)),
      _formats(dev.getSurfaceFormatsKHR(*surface)),
      _presentModes(dev.getSurfacePresentModesKHR(*surface)) {}

vk::SurfaceFormatKHR SwapchainSupportDetails::chooseFormat() {
  for (const auto &f : _formats) {
    if (f.format == vk::Format::eB8G8R8A8Srgb &&
        f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return f;
  }
  return _formats[0];
}

vk::Extent2D SwapchainSupportDetails::chooseSwapchainExtent(
    GlfwWindow &window) {
  if (_capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
    return _capabilities.currentExtent;
  } else {
    auto size = window.framebufferSize();

    vk::Extent2D actual = {static_cast<uint32_t>(size.first),
                           static_cast<uint32_t>(size.second)};
    actual.width = clamp(actual.width, _capabilities.minImageExtent.width,
                         _capabilities.maxImageExtent.width);
    actual.height = clamp(actual.height, _capabilities.minImageExtent.height,
                          _capabilities.maxImageExtent.height);
    return actual;
  }
}
