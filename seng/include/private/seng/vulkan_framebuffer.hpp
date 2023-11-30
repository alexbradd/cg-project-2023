#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;
class VulkanRenderPass;

class VulkanFramebuffer {
 public:
  VulkanFramebuffer(VulkanDevice& dev,
                    VulkanRenderPass& pass,
                    vk::Extent2D size,
                    std::vector<vk::ImageView>& attachments);
  VulkanFramebuffer(const VulkanFramebuffer&) = delete;
  VulkanFramebuffer(VulkanFramebuffer&&) = default;

  VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
  VulkanFramebuffer& operator=(VulkanFramebuffer&&) = default;

  vk::raii::Framebuffer& handle() { return _handle; }

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  std::reference_wrapper<VulkanRenderPass> vkRenderPass;
  std::vector<vk::ImageView> attachments;
  vk::raii::Framebuffer _handle;
};

}  // namespace seng::rendering
