#pragma once

#include <functional>
#include <seng/glfw_window.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_debug_messenger.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_fence.hpp>
#include <seng/vulkan_framebuffer.hpp>
#include <seng/vulkan_render_pass.hpp>
#include <seng/vulkan_swapchain.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

/**
 * Exception thrown if, for some reason, starting the frame was not possible.
 */
class BeginFrameException : public std::exception {
 public:
  BeginFrameException(std::string_view error) : err{error} {}
  const char *what() const noexcept override { return err.c_str(); }

 private:
  std::string err{};
};

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
  ~VulkanRenderer();

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

  /**
   * Signal that the window has been resized and the swapchain/frambuffers need
   * to be regenerated
   */
  void signalResize();

  /**
   * Starts recording a frame. If recording cannot be started, raise a
   * `BeginFrameException`.
   */
  void beginFrame();

  /**
   * Finishes recording a frame.
   */
  void endFrame();

  /**
   * Draws a frame.
   */
  void draw();

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
  std::vector<VulkanFramebuffer> framebuffers;
  std::vector<vk::raii::Semaphore> imageAvailableSems;
  std::vector<vk::raii::Semaphore> queueCompleteSems;
  std::vector<VulkanFence> inFlightFences;
  std::vector<VulkanFence *> imgsInFlight;
  uint64_t fbGeneration = 0, lastFbGeneration = 0;
  uint32_t currentFrame = 0;
  uint32_t imageIndex = 0;
  bool recreatingSwapchain = false;

  /**
   * Recreate the current swapchain and framebuffers
   */
  void recreateSwapchain();
};

}  // namespace seng::rendering
