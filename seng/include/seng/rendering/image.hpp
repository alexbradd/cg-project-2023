#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class Device;
class CommandBuffer;
class Buffer;

/**
 * A wrapper for a vulkan image and its view (if present). The view can be
 * both "managed" (allocated and managed with the RAII pattern) or "unmanaged"
 * (no memory management is done).
 */
class Image {
 public:
  /**
   * Struct to hold all paparameters used during creation.
   */
  struct CreateInfo {
    vk::ImageType type;
    vk::Extent3D extent;
    vk::Format format;
    vk::ImageTiling tiling;
    vk::ImageUsageFlags usage;
    vk::MemoryPropertyFlags memoryFlags;
    vk::ImageViewType viewType;
    vk::ImageAspectFlags aspectFlags;
    bool mipped;
    bool createView;
  };

  /// Create a null image
  Image(std::nullptr_t);

  /**
   * Create and allocate an image. If CreateInfo.createView is set, then the
   * corresponding view will also be allocated.
   */
  Image(const Device &dev, const CreateInfo &info);

  /**
   * Wrap the given handle. Deallocation is not carried on when this object
   * goes out of scope.
   */
  Image(const Device &dev, vk::Image wrapped, bool mipped = false);

  Image(const Image &) = delete;
  Image(Image &&) = default;
  ~Image();

  Image &operator=(const Image &) = delete;
  Image &operator=(Image &&) = default;

  // Accessors
  const vk::Image image() const
  {
    return m_unmanaged != nullptr ? m_unmanaged : *m_handle;
  }
  const vk::ImageView imageView() const { return *m_view; }
  bool hasView() const { return *m_view != nullptr; }

  /**
   * Create a new image view with the specified parameters.
   */
  void createView(vk::ImageViewType type, vk::Format format, vk::ImageAspectFlags aspect);

  /**
   * Steal ownership of the given view.
   */
  void stealView(vk::raii::ImageView &&view) { m_view = std::move(view); }

  /**
   * Copy contents of the given buffer into this image.
   */
  void copyFromBuffer(const CommandBuffer &commandBuf, const Buffer &buf) const;

  /**
   * Transition the layout of this image from the old layout to the new one.
   */
  void transitionLayout(const CommandBuffer &commandBuf,
                        vk::Format format,
                        vk::ImageLayout oldLayout,
                        vk::ImageLayout newLayout) const;

 private:
  const Device *m_device;

  vk::raii::Image m_handle;
  vk::raii::DeviceMemory m_memory;
  vk::Extent3D m_extent;
  bool m_mipped;

  vk::Image m_unmanaged;

  vk::raii::ImageView m_view;
};

}  // namespace seng::rendering
