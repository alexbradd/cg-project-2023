#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;
class VulkanSwapchain;
class VulkanCommandBuffer;
class VulkanFramebuffer;

/**
 * Wrapper around a render pass. It implements the RAII pattern so
 * allocation means creation of a new renderpass and destruction means
 * deallocation.
 *
 * It is movable but not copyable.
 */
class VulkanRenderPass {
 public:
  /**
   * Create and allocate a new render pass with the format and extent of the
   * given swapchain.
   */
  VulkanRenderPass(const VulkanDevice& dev, const VulkanSwapchain& swap);
  /**
   * Create and allocate a new render pass with the specified parameters.
   */
  VulkanRenderPass(const VulkanDevice& device,
                   vk::Format colorFormat,
                   vk::Format depthFormat,
                   vk::Offset2D offset,
                   vk::Extent2D extent,
                   vk::ClearColorValue clearColor,
                   vk::ClearDepthStencilValue clearDepth);
  VulkanRenderPass(const VulkanRenderPass&) = delete;
  VulkanRenderPass(VulkanRenderPass&&) = default;
  ~VulkanRenderPass();

  VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
  VulkanRenderPass& operator=(VulkanRenderPass&&) = default;

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
  void begin(const VulkanCommandBuffer& buf, const VulkanFramebuffer& fb) const;

  /**
   * End the render pass. The command buffer will be set to `eRecording`
   */
  void end(const VulkanCommandBuffer& buf) const;

 private:
  const VulkanDevice* vulkanDev;
  vk::raii::RenderPass _pass;
  vk::Offset2D offset;
  vk::Extent2D extent;
  vk::ClearColorValue clearColor;
  vk::ClearDepthStencilValue clearDepth;
};

}  // namespace seng::rendering
