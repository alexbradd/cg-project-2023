#pragma once

#include <seng/application_config.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/debug_messenger.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/fence.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/image.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/swapchain.hpp>
#include <seng/utils.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Copied from Vulkan-Samples. Needed so that we can put a DescriptorSetLayout handle in a
// hashmap
namespace std {
template <>
struct hash<vk::DescriptorSetLayout> {
  std::size_t operator()(const vk::DescriptorSetLayout &descriptorSetLayout) const
  {
    std::size_t result = 0;
    seng::internal::hashCombine(result,
                                static_cast<VkDescriptorSetLayout>(descriptorSetLayout));
    return result;
  }
};
}  // namespace std

namespace seng::rendering {

// Forward declarations
class GlfwWindow;

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

  // Accessors
  const Device &getDevice() const { return device; }

  /**
   * Signal that the window has been resized and the swapchain/frambuffers need
   * to be regenerated.
   */
  void signalResize();

  /**
   * Allocate a new descriptor set with the given layout from the pool.
   */
  void requestDescriptorSet(vk::DescriptorSetLayout layout);

  /**
   * Destroy all allocated descriptor sets and clear the pool.
   */
  void clearDescriptorSets();

  /**
   * Starts recording a frame and returns a handle to it. If recording cannot be
   * started, return an empty optional.
   */
  std::optional<FrameHandle> beginFrame();

  /**
   * Fetches a descriptor set with the given layout usable during the current in-progress
   * frame.
   *
   * Note: the layout must have been previously registered with a call to
   * registerDescriptorLayout().
   */
  const vk::raii::DescriptorSet &getDescriptorSet(const FrameHandle &frame,
                                                  vk::DescriptorSetLayout layout) const;

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
   * A render target is basically something that can be drawn to. In our specific
   * case the targets coincide with the swapchain images.
   */
  struct RenderTarget {
    vk::ImageView swapchainImage;
    Image depthBuffer;
    Framebuffer framebuffer;

    RenderTarget(const Device &device,
                 const vk::ImageView swapchainImage,
                 vk::Extent2D extent,
                 const RenderPass &pass);

    static Image createDepthBuffer(const Device &device, vk::Extent2D extent);
  };

  /**
   * A frame is where the resources for drawing an image reside. Usually the
   * renderer will keep many frames, so that it can minimize waiting time.
   */
  struct Frame {
    CommandBuffer commandBuffer;
    vk::raii::Semaphore imageAvailableSem;
    vk::raii::Semaphore queueCompleteSem;
    Fence inFlightFence;
    std::unordered_map<vk::DescriptorSetLayout, vk::raii::DescriptorSet> descriptorSets;
    ssize_t imageIndex;

    Frame(const Device &device, const vk::raii::CommandPool &commandPool);
  };

  static const std::array<vk::DescriptorPoolSize, 1> POOL_SIZES;

  const GlfwWindow *window;
  vk::raii::Context context;
  vk::raii::Instance instance;
  DebugMessenger debugMessenger;
  vk::raii::SurfaceKHR surface;
  Device device;
  Swapchain swapchain;
  std::vector<Attachment> attachments;
  RenderPass renderPass;
  vk::raii::CommandPool cmdPool;
  vk::raii::DescriptorPool descriptorPool;

  std::vector<RenderTarget> targets;
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
