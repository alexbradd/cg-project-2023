#pragma once

#include <functional>
#include <seng/vulkan_queue_family_indices.hpp>
#include <seng/vulkan_swapchain_support_details.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

/**
 * Wrapper object that contains everything related to the rendering device we
 * are going to use: physical device handle, logical device handle and queues.
 *
 * It can only be moved, not copied.
 */
class VulkanDevice {
 public:
  VulkanDevice(vk::raii::Instance &instance, vk::raii::SurfaceKHR &surf);
  VulkanDevice(const VulkanDevice &) = delete;
  VulkanDevice(VulkanDevice &&) = default;

  VulkanDevice &operator=(const VulkanDevice &) = delete;
  VulkanDevice &operator=(VulkanDevice &&) = default;

  vk::raii::PhysicalDevice &physical() { return _physical; }
  vk::raii::Device &logical() { return _logical; }
  vk::raii::Queue &presentQueue() { return _presentQueue; }
  vk::raii::Queue &graphicsQueue() { return _graphicsQueue; }
  QueueFamilyIndices &queueFamiliyIndices() { return _queueIndices; }
  SwapchainSupportDetails &swapchainSupportDetails() {
    return _swapchainDetails;
  }
  /* vk::SurfaceFormatKHR depthFormat() { return _depthFormat; } */

  uint32_t findMemoryIndex(uint32_t filter, vk::MemoryPropertyFlags flags);

  void requerySupport();
  /* void requeryDepthFormat(); */

 private:
  std::reference_wrapper<vk::raii::SurfaceKHR> _surface;
  vk::raii::PhysicalDevice _physical;
  QueueFamilyIndices _queueIndices;
  SwapchainSupportDetails _swapchainDetails;
  vk::raii::Device _logical;
  vk::raii::Queue _presentQueue;
  vk::raii::Queue _graphicsQueue;
  /* vk::SurfaceFormatKHR _depthFormat; */
};

}  // namespace seng::rendering
