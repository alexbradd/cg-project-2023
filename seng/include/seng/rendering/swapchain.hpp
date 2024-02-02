#pragma once

#include <seng/rendering/image.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <exception>
#include <limits>
#include <vector>

namespace seng::rendering {

class Device;
class GlfwWindow;

/**
 * Exception singaling an out of date or suboptimal swapchain. Users of the
 * class should recreate the swapchain and relative framebuffers if they catch
 * it.
 */
class InadequateSwapchainException : public std::exception {
 public:
  InadequateSwapchainException(std::string_view error, vk::Result res) :
      m_err{error}, m_res{res}
  {
  }

  const char *what() const noexcept override { return m_err.c_str(); }
  vk::Result result() const noexcept { return m_res; }

 private:
  std::string m_err;
  vk::Result m_res;
};

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

  /**
   * Acquire the next image index in the swapchain. The given semaphore and
   * fence (if given) will be signaled when done.
   *
   * If the swapchain is out of date or suboptimal,
   * `InadequateSwapchainException` is thrown.
   */
  uint32_t nextImageIndex(const vk::raii::Semaphore &imgAvailable,
                          const vk::raii::Fence *fence = nullptr,
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
