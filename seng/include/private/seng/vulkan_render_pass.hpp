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
  VulkanRenderPass(VulkanDevice& dev, VulkanSwapchain& swap);
  /**
   * Create and allocate a new render pass with the specified parameters.
   */
  VulkanRenderPass(VulkanDevice& device,
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
  vk::raii::RenderPass& handle() { return _pass; }
  vk::Viewport fullViewport();
  vk::Rect2D fullScissor();

  // Update the offset and extent
  void updateOffset(vk::Offset2D offset);
  void updateExtent(vk::Extent2D extent);

  /**
   * Begin a render pass. The command buffer will be set to eInRenderPass
   */
  void begin(VulkanCommandBuffer& buf, VulkanFramebuffer& fb);

  /**
   * End the render pass. The command buffer will be set to `eRecording`
   */
  void end(VulkanCommandBuffer& buf);

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  vk::raii::RenderPass _pass;
  vk::Offset2D offset;
  vk::Extent2D extent;
  vk::ClearColorValue clearColor;
  vk::ClearDepthStencilValue clearDepth;
};

}  // namespace seng::rendering
