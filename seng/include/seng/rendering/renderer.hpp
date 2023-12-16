#pragma once

#include <seng/application_config.hpp>
#include <seng/primitive_types.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/debug_messenger.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/fence.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/image.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/swapchain.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

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
 * A handle referring to an in-construction frame
 */
class FrameHandle {
  friend class Renderer;

 public:
  FrameHandle() = default;

  bool invalid(size_t maxValue) const;
  void invalidate();

 private:
  FrameHandle(ssize_t value) : frameIndex(value) {}
  ssize_t frameIndex = -1;
};

/**
 * Class containing the entirety of the vulkan rendering context. Instantiating
 * creates the vulkan context and allocates all the necessary resoruces, while
 * destruction deallocates it.
 *
 * It is movable, but non-copyable.
 */
class Renderer {
 public:
  /**
   * Boot up the vulkan renderer and draw into the given window.
   */
  Renderer(ApplicationConfig config, const GlfwWindow &window);
  Renderer(const Renderer &) = delete;
  Renderer(Renderer &&) = default;
  ~Renderer();

  Renderer &operator=(const Renderer &) = delete;
  Renderer &operator=(Renderer &&) = default;

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
   * Starts recording a frame and returns a handle to it. If recording cannot be
   * started, raise a `BeginFrameException`.
   */
  FrameHandle beginFrame();

  void updateGlobalState(glm::mat4 projection, glm::mat4 view) const;
  void updateModel(glm::mat4 model) const;

  /**
   * Finishes recording the frame referred by the given handle. The handle is
   * invalidated after the call. If an invalid handle is passed, throw a
   * runtime_error.
   */
  void endFrame(FrameHandle &frame);

  /**
   * Draws a frame.
   */
  void draw() const;

 private:
  /**
   * A frame is where the resources for drawing an image reside. Usually the
   * renderer will keep many frames, so that it can minimize waiting time.
   */
  struct Frame {
    CommandBuffer commandBuffer;
    vk::raii::Semaphore imageAvailableSem;
    vk::raii::Semaphore queueCompleteSem;
    Fence inFlightFence;
    ssize_t imageIndex;

    Frame(const Device &device, const vk::raii::CommandPool &commandPool);
  };

  const GlfwWindow *window;
  vk::raii::Context context;
  vk::raii::Instance instance;
  DebugMessenger debugMessenger;
  vk::raii::SurfaceKHR surface;
  Device device;
  Swapchain swapchain;
  Image depthBuffer;
  RenderPass renderPass;
  std::vector<Framebuffer> framebuffers;
  vk::raii::CommandPool cmdPool;
  std::vector<Frame> frames;

  Buffer vertexBuffer, indexBuffer;

  uint64_t fbGeneration = 0, lastFbGeneration = 0;
  uint32_t currentFrame = 0;
  bool recreatingSwapchain = false;

  // FIXME: Test Geometry
  std::array<Vertex, 4> verts;
  std::array<uint32_t, 6> indices;

  /**
   * Recreate the current swapchain and framebuffers
   */
  void recreateSwapchain();
};

}  // namespace seng::rendering
