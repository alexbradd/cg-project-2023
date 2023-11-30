#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class GlfwWindow;

class SwapchainSupportDetails {
 public:
  SwapchainSupportDetails(vk::raii::PhysicalDevice &device,
                          vk::raii::SurfaceKHR &surface);

  vk::SurfaceFormatKHR chooseFormat();
  vk::Extent2D chooseSwapchainExtent(GlfwWindow &window);

  vk::SurfaceCapabilitiesKHR &capabilities() { return _capabilities; }
  std::vector<vk::SurfaceFormatKHR> &formats() { return _formats; };
  std::vector<vk::PresentModeKHR> &presentModes() { return _presentModes; };

 private:
  vk::SurfaceCapabilitiesKHR _capabilities;
  std::vector<vk::SurfaceFormatKHR> _formats;
  std::vector<vk::PresentModeKHR> _presentModes;
};

}  // namespace seng::rendering
