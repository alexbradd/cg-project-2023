#pragma once

#include <seng/rendering/image.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <exception>
#include <limits>
#include <vector>

namespace seng::rendering {

class VulkanDevice;
class VulkanFence;
class GlfwWindow;

/**
 * Exception singaling an out of date or suboptimal swapchain. Users of the
 * class should recreate the swapchain and relative framebuffers if they catch
 * it.
 */
class InadequateSwapchainException : public std::exception {
 public:
  InadequateSwapchainException(std::string_view error, vk::Result res) :
      err{error}, r{res}
  {
  }

  const char *what() const noexcept override { return err.c_str(); }
  vk::Result result() const noexcept { return r; }

 private:
  std::string err{};
  vk::Result r;
};

/**
 * Wrapper for a vulkan swapchain. It implements the RAII pattern, meaning that
 * creation allocates resources, while destruction deallocates them.
 *
 * It non-copyable but movable.
 */
class VulkanSwapchain {
 public:
  VulkanSwapchain(const VulkanDevice &, const vk::raii::SurfaceKHR &, const GlfwWindow &);
  VulkanSwapchain(const VulkanSwapchain &) = delete;
  VulkanSwapchain(VulkanSwapchain &&) = default;
  ~VulkanSwapchain();

  VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;
  VulkanSwapchain &operator=(VulkanSwapchain &&) = default;

  static constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 2;

  /**
   * Acquire the next image index in the swapchain. The given semaphore and
   * fence (if given) will be signaled when done.
   *
   * If the swapchain is out of date or suboptimal,
   * `InadequateSwapchainException` is thrown.
   */
  uint32_t nextImageIndex(const vk::raii::Semaphore &imgAvailable,
                          const VulkanFence *fence = nullptr,
                          uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;

  /**
   * Presents the frame with index `imageIndex` on the present Queue. If the
   * current swapchain is inadequate/out of date `InadequateSwapchainException`
   * is thrown.
   */
  void present(const vk::raii::Queue &presentQueue,
               const vk::raii::Queue &graphicsQueue,
               const vk::raii::Semaphore &renderComplete,
               uint32_t imageIndex) const;

  // Accessors
  const vk::raii::SwapchainKHR &swapchain() const { return _swapchain; }
  const std::vector<vk::raii::ImageView> &images() const { return _imageViews; }
  const vk::SurfaceFormatKHR &format() const { return _format; }
  const vk::Extent2D &extent() const { return _extent; }
  const VulkanImage &depthBuffer() const { return _depthBufferImage; }

 private:
  const VulkanDevice *vulkanDev;
  vk::SurfaceFormatKHR _format;
  vk::Extent2D _extent;
  vk::raii::SwapchainKHR _swapchain;
  std::vector<vk::Image> _images;
  std::vector<vk::raii::ImageView> _imageViews;
  VulkanImage _depthBufferImage;
};

}  // namespace seng::rendering
