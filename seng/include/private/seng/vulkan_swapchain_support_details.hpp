#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class GlfwWindow;

/**
 * The supported capabilities/formats for a swapchain.
 */
class SwapchainSupportDetails {
 public:
  /**
   * Acquire the details from the given reources.
   */
  SwapchainSupportDetails(vk::raii::PhysicalDevice &device,
                          vk::raii::SurfaceKHR &surface);

  /**
   * Choose the most optimal format.
   */
  vk::SurfaceFormatKHR chooseFormat();

  /**
   * Chose the optimal swapchain extent.
   */
  vk::Extent2D chooseSwapchainExtent(GlfwWindow &window);

  // Accessors
  vk::SurfaceCapabilitiesKHR &capabilities() { return _capabilities; }
  std::vector<vk::SurfaceFormatKHR> &formats() { return _formats; };
  std::vector<vk::PresentModeKHR> &presentModes() { return _presentModes; };

 private:
  vk::SurfaceCapabilitiesKHR _capabilities;
  std::vector<vk::SurfaceFormatKHR> _formats;
  std::vector<vk::PresentModeKHR> _presentModes;
};

}  // namespace seng::rendering
