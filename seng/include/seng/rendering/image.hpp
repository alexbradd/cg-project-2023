#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
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
  Image(const Device &dev, const CreateInfo &info);
  Image(const Image &) = delete;
  Image(Image &&) = default;
  ~Image();

  Image &operator=(const Image &) = delete;
  Image &operator=(Image &&) = default;

  // Accessors
  const vk::raii::Image &image() const { return handle; }
  const std::optional<vk::raii::ImageView> &imageView() const { return view; }

  /**
   * Create the relative image view. If a view has already been created, the do
   * nothing.
   */
  void createView();

 private:
  CreateInfo info;
  const Device *vulkanDev;
  uint32_t width, height;
  vk::raii::Image handle;
  vk::raii::DeviceMemory memory;
  std::optional<vk::raii::ImageView> view;
};

}  // namespace seng::rendering
