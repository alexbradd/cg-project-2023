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
  SwapchainSupportDetails(const vk::raii::PhysicalDevice &device,
                          const vk::raii::SurfaceKHR &surface);

  /**
   * Choose the most optimal format.
   */
  vk::SurfaceFormatKHR chooseFormat() const;

  /**
   * Chose the optimal swapchain extent.
   */
  vk::Extent2D chooseSwapchainExtent(const GlfwWindow &window) const;

  // Accessors
  const vk::SurfaceCapabilitiesKHR &capabilities() const { return _capabilities; }
  const std::vector<vk::SurfaceFormatKHR> &formats() const { return _formats; };
  const std::vector<vk::PresentModeKHR> &presentModes() const { return _presentModes; };

 private:
  vk::SurfaceCapabilitiesKHR _capabilities;
  std::vector<vk::SurfaceFormatKHR> _formats;
  std::vector<vk::PresentModeKHR> _presentModes;
};

}  // namespace seng::rendering
