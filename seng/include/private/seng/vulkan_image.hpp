#pragma once

#include <functional>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

/**
 * Utility class for creating `vk::Image`s
 */
class VulkanImage {
 public:
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

  VulkanImage(VulkanDevice &dev, CreateInfo &info);
  VulkanImage(const VulkanImage &) = delete;
  VulkanImage(VulkanImage &&) = default;
  ~VulkanImage();

  VulkanImage &operator=(const VulkanImage &) = delete;
  VulkanImage &operator=(VulkanImage &&) = default;

  vk::raii::Image &image() { return handle; }
  void createView();
  std::optional<vk::raii::ImageView> &imageView() { return view; }

 private:
  CreateInfo info;
  std::reference_wrapper<VulkanDevice> vkDevRef;
  uint32_t width, height;
  vk::raii::Image handle;
  vk::raii::DeviceMemory memory;
  std::optional<vk::raii::ImageView> view;
};

}  // namespace seng::rendering
