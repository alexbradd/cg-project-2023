#pragma once

#include <functional>
#include <seng/glfw_window.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_debug_messenger.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_render_pass.hpp>
#include <seng/vulkan_swapchain.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

/**
 * Class containing the entirety of the vulkan rendering context. Instantiating
 * creates the vulkan context and allocates all the necessary resoruces, while
 * destruction destroy the vulkan context.
 *
 * It is non-copyable and non-movable.
 */
class VulkanRenderer {
 public:
  VulkanRenderer(GlfwWindow &window);
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = default;

  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(VulkanRenderer &&) = default;

  static const std::vector<const char *> requiredDeviceExtensions;
  static const std::vector<const char *> validationLayers;

#ifndef NDEBUG
  static constexpr bool useValidationLayers{true};
#else
  static constexpr bool useValidationLayers{false};
#endif  // !NDEBUG

  vk::raii::Instance &instance() { return _instance; }
  vk::raii::SurfaceKHR &surface() { return _surface; }

 private:
  std::reference_wrapper<GlfwWindow> window;
  vk::raii::Context context;
  vk::raii::Instance _instance;
  DebugMessenger debugMessenger;
  vk::raii::SurfaceKHR _surface;
  VulkanDevice device;
  VulkanSwapchain swapchain;
  VulkanRenderPass renderPass;
  vk::raii::CommandPool cmdPool;
  std::vector<VulkanCommandBuffer> graphicsCmdBufs;
};

}  // namespace seng::rendering
