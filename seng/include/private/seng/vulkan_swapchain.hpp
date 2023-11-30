#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;
class GlfwWindow;

/**
 * Wrapper for the current swapchain. It uses the RAII pattern, meaning that
 * creating allocates all resources, while destroying frees them. This allows us
 * to reallocate the swapchain by simply re-instantiating the class.
 *
 * Like all resources, it non-copyable but movable.
 */
class VulkanSwapchain {
 public:
  VulkanSwapchain(VulkanDevice &, vk::raii::SurfaceKHR &, GlfwWindow &);
  VulkanSwapchain(const VulkanSwapchain &) = delete;
  VulkanSwapchain(VulkanSwapchain &&) = default;
  ~VulkanSwapchain();

  VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;
  VulkanSwapchain &operator=(VulkanSwapchain &&) = default;

  static const uint8_t MAX_FRAMES_IN_FLIGHT = 2;

  static void recreate(VulkanSwapchain &loc,
                       VulkanDevice &dev,
                       vk::raii::SurfaceKHR &surface,
                       GlfwWindow &window);

  vk::raii::SwapchainKHR &swapchain() { return _swapchain; }
  std::vector<vk::raii::ImageView> &images() { return _imageViews; }
  vk::SurfaceFormatKHR &format() { return _format; }
  vk::Extent2D &extent() { return _extent; }

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  vk::SurfaceFormatKHR _format;
  vk::Extent2D _extent;
  vk::raii::SwapchainKHR _swapchain;
  std::vector<vk::Image> _images;
  std::vector<vk::raii::ImageView> _imageViews;

  // FIXME: add depth view
};

}  // namespace seng::rendering
