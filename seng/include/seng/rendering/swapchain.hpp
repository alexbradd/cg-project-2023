#pragma once

#include <seng/rendering/image.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

namespace seng::rendering {

class Device;
class GlfwWindow;

/**
 * Wrapper for a vulkan swapchain. It implements the RAII pattern, meaning that
 * creation allocates resources, while destruction deallocates them.
 *
 * It non-copyable but movable.
 */
class Swapchain {
 public:
  Swapchain(const Device &dev,
            const vk::raii::SurfaceKHR &surface,
            const GlfwWindow &window,
            const vk::raii::SwapchainKHR &old = vk::raii::SwapchainKHR{nullptr});
  Swapchain(const Swapchain &) = delete;
  Swapchain(Swapchain &&) = default;
  ~Swapchain();

  Swapchain &operator=(const Swapchain &) = delete;
  Swapchain &operator=(Swapchain &&) = default;

  static constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 2;

  // Accessors
  const vk::raii::SwapchainKHR &swapchain() const { return m_swapchain; }
  const std::vector<Image> &images() const { return m_images; }
  const vk::SurfaceFormatKHR &format() const { return m_format; }
  const vk::Extent2D &extent() const { return m_extent; }

 private:
  const Device *m_device;
  vk::SurfaceFormatKHR m_format;
  vk::Extent2D m_extent;
  vk::raii::SwapchainKHR m_swapchain;
  std::vector<Image> m_images;
};

}  // namespace seng::rendering
