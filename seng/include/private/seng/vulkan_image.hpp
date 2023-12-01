#pragma once

#include <functional>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

/**
 * A vulkan image, bundled with its underlying memory and (if present) view.
 * It implements the RAII pattern, meaning that instantiating the class
 * allocates, all resources, while destruction of the class deallocate them.
 */
class VulkanImage {
 public:
  /**
   * Struct to hold all paparameters used during creation.
   */
  struct CreateInfo {
    vk::ImageType type;
    uint32_t width;
    uint32_t height;
    vk::Format format;
    vk::ImageTiling tiling;
    vk::ImageUsageFlags usage;
    vk::MemoryPropertyFlags memoryFlags;
    vk::ImageAspectFlags aspectFlags;
    bool createView;
  };

  /**
   * Create and allocate an image. If CreateInfo.createView is set, then the
   * corresponding view will also be allocated.
   */
  VulkanImage(VulkanDevice &dev, CreateInfo &info);
  VulkanImage(const VulkanImage &) = delete;
  VulkanImage(VulkanImage &&) = default;
  ~VulkanImage();

  VulkanImage &operator=(const VulkanImage &) = delete;
  VulkanImage &operator=(VulkanImage &&) = default;

  // Accessors
  vk::raii::Image &image() { return handle; }
  std::optional<vk::raii::ImageView> &imageView() { return view; }

  /**
   * Create the relative image view. If a view has already been created, the do
   * nothing.
   */
  void createView();

 private:
  CreateInfo info;
  std::reference_wrapper<VulkanDevice> vkDevRef;
  uint32_t width, height;
  vk::raii::Image handle;
  vk::raii::DeviceMemory memory;
  std::optional<vk::raii::ImageView> view;
};

}  // namespace seng::rendering
