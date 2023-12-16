#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class Device;
class Swapchain;
class CommandBuffer;
class Framebuffer;

/**
 * Wrapper around a render pass. It implements the RAII pattern so
 * allocation means creation of a new renderpass and destruction means
 * deallocation.
 *
 * It is movable but not copyable.
 */
class RenderPass {
 public:
  /**
   * Create and allocate a new render pass with the format and extent of the
   * given swapchain.
   */
  RenderPass(const Device& dev, const Swapchain& swap);
  /**
   * Create and allocate a new render pass with the specified parameters.
   */
  RenderPass(const Device& device,
             vk::Format colorFormat,
             vk::Format depthFormat,
             vk::Offset2D offset,
             vk::Extent2D extent,
             vk::ClearColorValue clearColor,
             vk::ClearDepthStencilValue clearDepth);
  RenderPass(const RenderPass&) = delete;
  RenderPass(RenderPass&&) = default;
  ~RenderPass();

  RenderPass& operator=(const RenderPass&) = delete;
  RenderPass& operator=(RenderPass&&) = default;

  // Accessors
  const vk::raii::RenderPass& handle() const { return _pass; }
  vk::Viewport fullViewport() const;
  vk::Rect2D fullScissor() const;

  // Update the offset and extent
  void updateOffset(vk::Offset2D offset);
  void updateExtent(vk::Extent2D extent);

  /**
   * Begin a render pass. The command buffer will be set to eInRenderPass
   */
  void begin(const CommandBuffer& buf, const Framebuffer& fb) const;

  /**
   * End the render pass. The command buffer will be set to `eRecording`
   */
  void end(const CommandBuffer& buf) const;

 private:
  const Device* vulkanDev;
  vk::raii::RenderPass _pass;
  vk::Offset2D offset;
  vk::Extent2D extent;
  vk::ClearColorValue clearColor;
  vk::ClearDepthStencilValue clearDepth;
};

}  // namespace seng::rendering
