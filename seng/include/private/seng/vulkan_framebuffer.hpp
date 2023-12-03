#pragma once

#include <functional>
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
  VulkanFramebuffer(VulkanDevice& dev,
                    VulkanRenderPass& pass,
                    vk::Extent2D size,
                    std::vector<vk::ImageView>& attachments);
  VulkanFramebuffer(const VulkanFramebuffer&) = delete;
  VulkanFramebuffer(VulkanFramebuffer&&) = default;
  ~VulkanFramebuffer();

  VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
  VulkanFramebuffer& operator=(VulkanFramebuffer&&) = default;

  // Accessors
  vk::raii::Framebuffer& handle() { return _handle; }

  /**
   * Create and allocate a new framebuffers taking as attachments the views and
   * depth buffer from the given swapchain
   */
  static std::vector<VulkanFramebuffer> fromSwapchain(VulkanDevice& device,
                                                      VulkanRenderPass& pass,
                                                      VulkanSwapchain& chain);

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  std::reference_wrapper<VulkanRenderPass> vkRenderPass;
  std::vector<vk::ImageView> attachments;
  vk::raii::Framebuffer _handle;
};

}  // namespace seng::rendering
