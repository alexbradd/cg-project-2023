#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <optional>

namespace seng::rendering {

class Device;

/**
 * A vulkan image, bundled with its underlying memory and (if present) view.
 * It implements the RAII pattern, meaning that instantiating the class
 * allocates, all resources, while destruction of the class deallocate them.
 */
class Image {
 public:
  /**
   * Struct to hold all paparameters used during creation.
   */
  struct CreateInfo {
    vk::ImageType type;
    vk::Extent2D extent;
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
  Image(const Device &dev, const CreateInfo &info);
  Image(const Image &) = delete;
  Image(Image &&) = default;
  ~Image();

  Image &operator=(const Image &) = delete;
  Image &operator=(Image &&) = default;

  // Accessors
  const vk::raii::Image &image() const { return m_handle; }
  const std::optional<vk::raii::ImageView> &imageView() const { return m_view; }

  /**
   * Create the relative image view. If a view has already been created, the do
   * nothing.
   */
  void createView();

 private:
  CreateInfo m_info;
  const Device *m_device;
  vk::raii::Image m_handle;
  vk::raii::DeviceMemory m_memory;
  std::optional<vk::raii::ImageView> m_view;
};

}  // namespace seng::rendering
