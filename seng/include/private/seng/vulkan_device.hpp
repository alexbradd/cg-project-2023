#pragma once

#include <functional>
#include <seng/vulkan_queue_family_indices.hpp>
#include <seng/vulkan_swapchain_support_details.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

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
class VulkanDevice {
 public:
  /**
   * Pick a suitable physical device, instatiate the relative logical one and
   * create the queues. If no suitable device can be found/costructed throw a
   * runtime_error().
   */
  VulkanDevice(vk::raii::Instance &instance, vk::raii::SurfaceKHR &surf);
  VulkanDevice(const VulkanDevice &) = delete;
  VulkanDevice(VulkanDevice &&) = default;
  ~VulkanDevice();

  VulkanDevice &operator=(const VulkanDevice &) = delete;
  VulkanDevice &operator=(VulkanDevice &&) = default;

  static const std::vector<const char *> REQUIRED_EXT;

  // Accessors to the underlying handles
  // FIXME: non-const overlaods ARE going to be removed once we are done constifying other
  // things
  vk::raii::PhysicalDevice &physical() { return _physical; }
  const vk::raii::PhysicalDevice &physical() const { return _physical; }
  vk::raii::Device &logical() { return _logical; }
  const vk::raii::Device &logical() const { return _logical; }
  vk::raii::Queue &presentQueue() { return _presentQueue; }
  const vk::raii::Queue &presentQueue() const { return _presentQueue; }
  vk::raii::Queue &graphicsQueue() { return _graphicsQueue; }
  const vk::raii::Queue &graphicsQueue() const { return _graphicsQueue; }
  vk::SurfaceFormatKHR depthFormat() { return _depthFormat; }
  vk::SurfaceFormatKHR depthFormat() const { return _depthFormat; }

  // Accessors to the support details
  QueueFamilyIndices &queueFamiliyIndices() { return _queueIndices; }
  const QueueFamilyIndices &queueFamiliyIndices() const { return _queueIndices; }
  SwapchainSupportDetails &swapchainSupportDetails() { return _swapchainDetails; }
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
  std::reference_wrapper<vk::raii::SurfaceKHR> _surface;
  vk::raii::PhysicalDevice _physical;
  QueueFamilyIndices _queueIndices;
  SwapchainSupportDetails _swapchainDetails;
  vk::raii::Device _logical;
  vk::raii::Queue _presentQueue;
  vk::raii::Queue _graphicsQueue;
  vk::SurfaceFormatKHR _depthFormat;
};

}  // namespace seng::rendering
