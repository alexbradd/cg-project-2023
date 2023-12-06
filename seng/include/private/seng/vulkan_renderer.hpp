#pragma once

#include <functional>
#include <seng/application_config.hpp>
#include <seng/primitive_types.hpp>
#include <seng/shader_loader.hpp>
#include <seng/vulkan_buffer.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_debug_messenger.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_fence.hpp>
#include <seng/vulkan_framebuffer.hpp>
#include <seng/vulkan_render_pass.hpp>
#include <seng/vulkan_swapchain.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace seng::rendering {

// Forward declarations
class GlfwWindow;

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
 * destruction deallocates it.
 *
 * It is movable, but non-copyable.
 */
class VulkanRenderer {
 public:
  /**
   * Boot up the vulkan renderer and draw into the given window.
   */
  VulkanRenderer(ApplicationConfig config, GlfwWindow &window);
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = default;
  ~VulkanRenderer();

  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(VulkanRenderer &&) = default;

  static const std::vector<const char *> VALIDATION_LAYERS;

#ifndef NDEBUG
  static constexpr bool USE_VALIDATION{true};
#else
  static constexpr bool USE_VALIDATION{false};
#endif  // !NDEBUG

  /**
   * Signal that the window has been resized and the swapchain/frambuffers need
   * to be regenerated.
   */
  void signalResize();

  /**
   * Starts recording a frame. If recording cannot be started, raise a
   * `BeginFrameException`.
   */
  void beginFrame();

  void update(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPosition);

  /**
   * Finishes recording a frame.
   */
  void endFrame();

  /**
   * Draws a frame.
   */
  void draw(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPosition);

 private:
  std::reference_wrapper<GlfwWindow> window;
  vk::raii::Context context;
  vk::raii::Instance instance;
  DebugMessenger debugMessenger;
  vk::raii::SurfaceKHR surface;
  VulkanDevice device;
  VulkanSwapchain swapchain;
  VulkanRenderPass renderPass;
  std::vector<VulkanFramebuffer> framebuffers;
  vk::raii::CommandPool cmdPool;
  std::vector<VulkanCommandBuffer> graphicsCmdBufs;
  std::vector<vk::raii::Semaphore> imageAvailableSems;
  std::vector<vk::raii::Semaphore> queueCompleteSems;
  std::vector<VulkanFence> inFlightFences;
  std::vector<VulkanFence *> imgsInFlight;

  VulkanBuffer vertexBuffer, indexBuffer;

  uint64_t fbGeneration = 0, lastFbGeneration = 0;
  uint32_t currentFrame = 0;
  uint32_t imageIndex = 0;
  bool recreatingSwapchain = false;

  ShaderLoader shaderLoader;  // FIXME: add dynamic loading

  // FIXME: Test Geometry
  Vertex verts[4];
  uint32_t indices[6];

  /**
   * Recreate the current swapchain and framebuffers
   */
  void recreateSwapchain();
};

}  // namespace seng::rendering
