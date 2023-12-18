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
  vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
  vk::AttachmentLoadOp loadOp;
  vk::AttachmentStoreOp storeOp;
  vk::AttachmentLoadOp stencilLoadOp;
  vk::AttachmentStoreOp stencilStoreOp;
  vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
  vk::ImageLayout finalLayout;
  vk::ImageLayout usage;
  vk::ClearValue clearValue;
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
  RenderPass(const RenderPass&) = delete;
  RenderPass(RenderPass&&) = default;
  ~RenderPass();

  RenderPass& operator=(const RenderPass&) = delete;
  RenderPass& operator=(RenderPass&&) = default;

  // Accessors
  const vk::raii::RenderPass& handle() const { return _pass; }

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
  const Device* vulkanDev;
  std::vector<Attachment> attachments;
  vk::raii::RenderPass _pass;
};

}  // namespace seng::rendering