#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class Device;
class Swapchain;
class CommandBuffer;
class Framebuffer;

/**
 * Description of an attachment It basically contains all fields of
 * vk::AttachmentDescription plus some extra stuff like usage layout and clear value
 */
struct Attachment {
  vk::Format format;
  vk::SampleCountFlagBits samples;
  vk::AttachmentLoadOp loadOp;
  vk::AttachmentStoreOp storeOp;
  vk::AttachmentLoadOp stencilLoadOp;
  vk::AttachmentStoreOp stencilStoreOp;
  vk::ImageLayout initialLayout;
  vk::ImageLayout finalLayout;
  vk::ImageLayout usage;
  vk::ClearValue clearValue;
  bool resolve = false;
};

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
   * Create and allocate a new render pass that uses the given renderpass
   */
  RenderPass(const Device& device, std::vector<Attachment> attachments);

  /// Create a null RenderPass
  RenderPass(std::nullptr_t);

  RenderPass(const RenderPass&) = delete;
  RenderPass(RenderPass&&) = default;
  ~RenderPass();

  RenderPass& operator=(const RenderPass&) = delete;
  RenderPass& operator=(RenderPass&&) = default;

  // Accessors
  const vk::raii::RenderPass& handle() const { return m_renderPass; }

  /**
   * Begin a render pass one the given extent and offeset.
   */
  void begin(const CommandBuffer& buf,
             const Framebuffer& fb,
             vk::Extent2D extent,
             vk::Offset2D offset) const;

  /**
   * End the render pass.
   */
  void end(const CommandBuffer& buf) const;

 private:
  std::vector<Attachment> m_attachments;
  vk::raii::RenderPass m_renderPass;
};

}  // namespace seng::rendering
