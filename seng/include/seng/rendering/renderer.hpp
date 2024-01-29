#pragma once

#include <seng/application_config.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/debug_messenger.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/fence.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/image.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/swapchain.hpp>
#include <seng/resources/mesh.hpp>
#include <seng/utils.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace seng {
class Application;
}

namespace seng::rendering {

// Forward declarations
class GlfwWindow;

/**
 * A handle referring to an in-construction frame. Can be implicitly constructed
 * from a size_t index.
 */
class FrameHandle {
  friend class Renderer;

 public:
  FrameHandle() = default;
  FrameHandle(std::nullptr_t) : m_index(-1) {}
  FrameHandle(size_t index) : m_index(index) {}

  bool invalid(size_t maxValue) const;
  void invalidate();
  size_t asIndex() const;

 private:
  ssize_t m_index = -1;
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
  Renderer(Application &app, const GlfwWindow &window);
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
  const Device &device() const { return m_device; }
  const RenderPass &renderPass() const { return m_renderPass; }
  const vk::raii::CommandPool &commandPool() const { return m_commandPool; }
  const vk::raii::DescriptorPool &descriptorPool() const { return m_descriptorPool; }

  const GlobalUniform &globalUniform() const { return m_gubo; }
  GlobalUniform &globalUniform() { return m_gubo; }

  size_t framesInFlight() const { return m_frames.size(); }

  /**
   * Signal that the window has been resized and the swapchain/frambuffers need
   * to be regenerated.
   */
  void signalResize();

  /**
   * Get a descriptor set layout with the given CreateInfo from the cache then
   * return a reference to it. If a matching layout cannot be found, allocate a
   * new one.
   */
  const vk::DescriptorSetLayout requestDescriptorSetLayout(
      vk::DescriptorSetLayoutCreateInfo info);

  /**
   * Destroy all allocated descriptor set layouts
   */
  void clearDescriptorSetLayouts();

  /**
   * Get a descriptor set with the given layout and buffers/images from the
   * frame-cache then return a reference to it. If a matching descriptor
   * cannot be found, allocate a new one.
   */
  const vk::DescriptorSet requestDescriptorSet(
      FrameHandle frameHandle,
      vk::DescriptorSetLayout layout,
      const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
      const std::vector<vk::DescriptorImageInfo> &imageInfo);

  /**
   * Get a descriptor set with the given layout and buffers/images from the
   * frame-cache then return a reference to it. If a matching descriptor cannot
   * be found, return a NULL handle.
   */
  const vk::DescriptorSet getDescriptorSet(
      FrameHandle frameHandle,
      vk::DescriptorSetLayout layout,
      const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
      const std::vector<vk::DescriptorImageInfo> &imageInfo) const;

  /**
   * Destroy the allocated descriptor sets with the given layout and info
   * from the frame with the given index.
   */
  void clearDescriptorSet(FrameHandle frameHandle,
                          vk::DescriptorSetLayout layout,
                          const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
                          const std::vector<vk::DescriptorImageInfo> &imageInfo);

  /**
   * Destroy all allocated descriptor sets and clear the pool.
   */
  void clearDescriptorSets();

  /**
   * Fetch the mesh with the given name from the mesh cache. If such mesh cannot
   * be found, load it from disk and save it in cache for later use.
   *
   * Freshly loaded meshes are not automatically synced.
   */
  Mesh &requestMesh(const std::string &name);

  /**
   * Delete the mesh with the given name from cache.
   */
  void clearMesh(const std::string &name);

  /**
   * Delete all cached meshes.
   */
  void clearMeshes();

  /// FIXME: stub, do not use.
  const std::unordered_map<std::string, Mesh> &meshes() const { return m_meshes; }

  /*
   * Get the command buffer of the current in-progress frame. If the passed handle is
   * invalid, a runtime error is thrown.
   */
  const CommandBuffer &getCommandBuffer(const FrameHandle &frame) const;

  /**
   * Starts recording a frame and returns a handle to it. If recording cannot be
   * started, return an empty optional.
   */
  std::optional<FrameHandle> beginFrame();

  /**
   * Finishes recording the frame referred by the given handle. The handle is
   * invalidated after the call. If an invalid handle is passed, throw a
   * runtime_error.
   */
  void endFrame(FrameHandle &frame);

  /**
   * Mimicking std::scoped_lock, try starting recording a frame and, if successfull,
   * execute the given function. At function exit (due to return or exception)
   * always end the frame.
   *
   * If the function has been executed return true, else false.
   *
   * Using this function is ALWAYS preferred over manually calling beginFrame() and
   * endFrame(), since we are guaranteed that the frame recording will always be stopped.
   */
  bool scopedFrame(std::function<void(const FrameHandle &)> func);

 private:
  /**
   * A render target is basically something that can be drawn to. In our specific
   * case the targets coincide with the swapchain images.
   */
  struct RenderTarget {
    const Image *m_swapchainImage;
    Image m_depthBuffer;
    Framebuffer m_framebuffer;

    RenderTarget(const Device &device,
                 const Image &swapchainImage,
                 vk::Extent2D extent,
                 const RenderPass &pass);

    static Image createDepthBuffer(const Device &device, vk::Extent2D extent);
  };

  /**
   * A frame is where the resources for drawing an image reside. Usually the
   * renderer will keep many frames, so that it can minimize waiting time.
   */
  struct Frame {
    CommandBuffer m_commandBuffer;
    vk::raii::Semaphore m_imageAvailableSem;
    vk::raii::Semaphore m_queueCompleteSem;
    Fence m_inFlightFence;
    std::unordered_map<size_t, vk::raii::DescriptorSet> m_descriptorCache;
    ssize_t m_index;

    Frame(const Device &device, const vk::raii::CommandPool &commandPool);
  };

  static const std::array<vk::DescriptorPoolSize, 1> POOL_SIZES;

  const Application *m_app;
  const GlfwWindow *m_window;
  vk::raii::Context m_context;
  vk::raii::Instance m_instance;
  DebugMessenger m_dbgMessenger;
  vk::raii::SurfaceKHR m_surface;
  Device m_device;
  Swapchain m_swapchain;
  std::vector<Attachment> m_attachments;
  RenderPass m_renderPass;
  vk::raii::CommandPool m_commandPool;
  vk::raii::DescriptorPool m_descriptorPool;

  std::vector<RenderTarget> m_targets;
  std::vector<Frame> m_frames;

  std::unordered_map<size_t, vk::raii::DescriptorSetLayout> m_layoutCache;

  // Mesh cache
  std::unordered_map<std::string, Mesh> m_meshes;
  Mesh m_fallbackMesh;

  GlobalUniform m_gubo;

  uint64_t m_fbGeneration = 0;
  uint64_t m_lastFbGeneration = 0;
  uint32_t m_currentFrame = 0;
  bool m_recreatingSwap = false;

  /**
   * Recreate the current swapchain and framebuffers
   */
  void recreateSwapchain();
};

}  // namespace seng::rendering
