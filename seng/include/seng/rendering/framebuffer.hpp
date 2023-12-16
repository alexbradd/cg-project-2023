#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class Device;
class RenderPass;
class Swapchain;

/**
 * Wrapper for a Vulkan framebuffer. It implements the RAII pattern, meaning
 * that instantiating the class allocates, all resources, while destruction of
 * the class deallocate them.
 *
 * It is movable, not copyable
 */
class Framebuffer {
 public:
  /**
   * Create and allocate a new framebuffer.
   */
  Framebuffer(const Device& dev,
              const RenderPass& pass,
              vk::Extent2D size,
              const std::vector<vk::ImageView>& attachments);
  Framebuffer(const Framebuffer&) = delete;
  Framebuffer(Framebuffer&&) = default;
  ~Framebuffer();

  Framebuffer& operator=(const Framebuffer&) = delete;
  Framebuffer& operator=(Framebuffer&&) = default;

  // Accessors
  const vk::raii::Framebuffer& handle() const { return _handle; }

  /**
   * Create and allocate a new framebuffers taking as attachments the views and
   * depth buffer from the given swapchain
   */
  static std::vector<Framebuffer> fromSwapchain(const Device& device,
                                                const RenderPass& pass,
                                                const Swapchain& chain);

 private:
  const Device* vulkanDev;
  const RenderPass* vulkanRenderPass;
  std::vector<vk::ImageView> attachments;
  vk::raii::Framebuffer _handle;
};

}  // namespace seng::rendering
