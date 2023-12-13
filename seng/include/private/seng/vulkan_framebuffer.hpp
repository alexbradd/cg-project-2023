#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;
class VulkanRenderPass;
class VulkanSwapchain;

/**
 * Wrapper for a Vulkan framebuffer. It implements the RAII pattern, meaning
 * that instantiating the class allocates, all resources, while destruction of
 * the class deallocate them.
 *
 * It is movable, not copyable
 */
class VulkanFramebuffer {
 public:
  /**
   * Create and allocate a new framebuffer.
   */
  VulkanFramebuffer(const VulkanDevice& dev,
                    const VulkanRenderPass& pass,
                    vk::Extent2D size,
                    const std::vector<vk::ImageView>& attachments);
  VulkanFramebuffer(const VulkanFramebuffer&) = delete;
  VulkanFramebuffer(VulkanFramebuffer&&) = default;
  ~VulkanFramebuffer();

  VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
  VulkanFramebuffer& operator=(VulkanFramebuffer&&) = default;

  // Accessors
  const vk::raii::Framebuffer& handle() const { return _handle; }

  /**
   * Create and allocate a new framebuffers taking as attachments the views and
   * depth buffer from the given swapchain
   */
  static std::vector<VulkanFramebuffer> fromSwapchain(const VulkanDevice& device,
                                                      const VulkanRenderPass& pass,
                                                      const VulkanSwapchain& chain);

 private:
  const VulkanDevice* vulkanDev;
  const VulkanRenderPass* vulkanRenderPass;
  std::vector<vk::ImageView> attachments;
  vk::raii::Framebuffer _handle;
};

}  // namespace seng::rendering
