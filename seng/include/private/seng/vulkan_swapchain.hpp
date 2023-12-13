#pragma once

#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <seng/vulkan_image.hpp>
#include <vulkan/vulkan_raii.hpp>

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
  VulkanSwapchain(VulkanDevice &, vk::raii::SurfaceKHR &, GlfwWindow &);
  VulkanSwapchain(const VulkanSwapchain &) = delete;
  VulkanSwapchain(VulkanSwapchain &&) = default;
  ~VulkanSwapchain();

  VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;
  VulkanSwapchain &operator=(VulkanSwapchain &&) = default;

  static const uint8_t MAX_FRAMES_IN_FLIGHT = 2;

  /**
   * Acquire the next image index in the swapchain. The given semaphore and
   * fence (if given) will be signaled when done.
   *
   * If the swapchain is out of date or suboptimal,
   * `InadequateSwapchainException` is thrown.
   */
  uint32_t nextImageIndex(
      vk::raii::Semaphore &imgAvailable,
      std::optional<std::reference_wrapper<VulkanFence>> fence = std::nullopt,
      uint64_t timeout = std::numeric_limits<uint64_t>::max());

  /**
   * Presents the frame with index `imageIndex` on the present Queue. If the
   * current swapchain is inadequate/out of date `InadequateSwapchainException`
   * is thrown.
   */
  void present(vk::raii::Queue &presentQueue,
               vk::raii::Queue &graphicsQueue,
               vk::raii::Semaphore &renderComplete,
               uint32_t imageIndex);

  /**
   * Recreate in-place the swapchain.
   *
   * @param loc refernce for where the swapchain will be recreated
   */
  static void recreate(VulkanSwapchain &loc,
                       VulkanDevice &dev,
                       vk::raii::SurfaceKHR &surface,
                       GlfwWindow &window);

  // Accessors
  vk::raii::SwapchainKHR &swapchain() { return _swapchain; }
  std::vector<vk::raii::ImageView> &images() { return _imageViews; }
  vk::SurfaceFormatKHR &format() { return _format; }
  vk::Extent2D &extent() { return _extent; }
  VulkanImage &depthBuffer() { return _depthBufferImage; }

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  vk::SurfaceFormatKHR _format;
  vk::Extent2D _extent;
  vk::raii::SwapchainKHR _swapchain;
  std::vector<vk::Image> _images;
  std::vector<vk::raii::ImageView> _imageViews;
  VulkanImage _depthBufferImage;
};

}  // namespace seng::rendering
