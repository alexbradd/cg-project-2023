#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace seng::rendering {

class GlfwWindow;

/**
 * The graphics/presentation queue family indexes for a given
 * PhysicalDevice/Surface pair.
 */
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  /**
   * Query given device and surface for support.
   */
  QueueFamilyIndices(const vk::raii::PhysicalDevice &device,
                     const vk::raii::SurfaceKHR &surface);

  /**
   * Return true if both graphics and presentation indices have been found
   */
  bool isComplete() const;
};

/**
 * The supported capabilities/formats for a swapchain.
 */
struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;

  /**
   * Query the devices for the parameters of usable swapchains.
   */
  SwapchainSupportDetails(const vk::raii::PhysicalDevice &device,
                          const vk::raii::SurfaceKHR &surface);

  /**
   * Choose the optimal swapchain format.
   */
  vk::SurfaceFormatKHR chooseFormat() const;

  /**
   * Chose the optimal swapchain extent.
   */
  vk::Extent2D chooseExtent(const GlfwWindow &window) const;
};

/**
 * The rendering device we are going to use. It holds references to the physical
 * device and corresponding logical one, as well as the supported details
 * (queues and formats) and the graphics and presentation queues.
 *
 * It implements the RAII pattern, meaning that instatiation is allocation,
 * while destruction is deallocation of all underlying structures.
 *
 * It can only be moved, not copied.
 */
class Device {
 public:
  /**
   * Pick a suitable physical device, instatiate the relative logical one and
   * create the queues. If no suitable device can be found/costructed throw a
   * runtime_error().
   */
  Device(const vk::raii::Instance &instance, const vk::raii::SurfaceKHR &surf);
  Device(const Device &) = delete;
  Device(Device &&) = default;
  ~Device();

  Device &operator=(const Device &) = delete;
  Device &operator=(Device &&) = default;

  static const std::vector<const char *> REQUIRED_EXT;

  // Accessors to the underlying handles
  const vk::raii::PhysicalDevice &physical() const { return _physical; }
  const vk::raii::Device &logical() const { return _logical; }
  const vk::raii::Queue &presentQueue() const { return _presentQueue; }
  const vk::raii::Queue &graphicsQueue() const { return _graphicsQueue; }
  vk::SurfaceFormatKHR depthFormat() const { return _depthFormat; }

  // Accessors to the support details
  const QueueFamilyIndices &queueFamilyIndices() const { return _queueIndices; }
  const SwapchainSupportDetails &swapchainSupportDetails() const
  {
    return _swapchainDetails;
  }

  /**
   * Requery the swapchain support details.
   */
  void requerySupport();

  /**
   * Requery the depth format.
   */
  void requeryDepthFormat();

  /**
   * Query the underlying physical device for the memory index. Thorw an error
   * is the suitable memory type cannot be found.
   */
  uint32_t findMemoryIndex(uint32_t filter, vk::MemoryPropertyFlags flags) const;

 private:
  const vk::raii::SurfaceKHR *_surface;
  vk::raii::PhysicalDevice _physical;
  QueueFamilyIndices _queueIndices;
  SwapchainSupportDetails _swapchainDetails;
  vk::raii::Device _logical;
  vk::raii::Queue _presentQueue;
  vk::raii::Queue _graphicsQueue;
  vk::SurfaceFormatKHR _depthFormat;

  /**
   * Choose the optimal swapchain format.
   */
  vk::SurfaceFormatKHR chooseSwapchainFormat() const;

  /**
   * Chose the optimal swapchain extent.
   */
  vk::Extent2D chooseSwapchainExtent(const GlfwWindow &window) const;
};

}  // namespace seng::rendering
