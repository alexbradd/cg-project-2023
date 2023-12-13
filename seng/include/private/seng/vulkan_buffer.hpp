#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

/**
 * Wrapper class around a generic vulkan buffer. It implements the RAII pattern,
 * meaning that instantiation of the class allocates the resources, while
 * destruction deallocates them.
 *
 * It is movable, not copyable.
 */
class VulkanBuffer {
 public:
  /**
   * Allocate a new buffer, also bind it if instructed to do so.
   */
  VulkanBuffer(
      VulkanDevice &dev,
      vk::BufferUsageFlags usage,
      vk::DeviceSize size,
      vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
      bool bind = true);
  VulkanBuffer(const VulkanBuffer &) = delete;
  VulkanBuffer(VulkanBuffer &&) = default;
  ~VulkanBuffer();

  VulkanBuffer &operator=(const VulkanBuffer &) = delete;
  VulkanBuffer &operator=(VulkanBuffer &&) = default;

  vk::raii::Buffer &buffer() { return handle; }

  /**
   * Bind the buffer at the give offset
   */
  void bind(vk::DeviceSize offset);

  /**
   * Resize the buffer to the new size.
   */
  void resize(vk::DeviceSize size, vk::raii::Queue &queue, vk::raii::CommandPool &pool);

  /**
   * Lock the memory of the buffer.
   */
  void *lockMemory(vk::DeviceSize size, vk::DeviceSize offset, vk::MemoryMapFlags flags);

  /**
   * Unlock the buffer.
   */
  void unlockMemory();

  /**
   * Lock the buffer and copy the given amount of data into it at the given
   * offset
   */
  void load(const void *data,
            vk::DeviceSize offset,
            vk::DeviceSize size,
            vk::MemoryMapFlags flags);

  /**
   * Copy a region of this buffer into one of the destination buffer.
   */
  void copy(VulkanBuffer &dest,
            vk::BufferCopy copyRegion,
            vk::raii::CommandPool &pool,
            vk::raii::Queue &queue,
            std::optional<std::reference_wrapper<vk::raii::Fence>> fence = std::nullopt);

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  vk::BufferUsageFlags usage;
  vk::DeviceSize size;
  vk::raii::Buffer handle;
  vk::MemoryRequirements memRequirements;
  uint32_t memIndex;
  vk::raii::DeviceMemory memory;
  bool locked;

  /**
   * Copy a region of this buffer into one of the destination buffer.
   */
  void rawCopy(
      vk::raii::Buffer &dest,
      vk::BufferCopy copyRegion,
      vk::raii::CommandPool &pool,
      vk::raii::Queue &queue,
      std::optional<std::reference_wrapper<vk::raii::Fence>> fence = std::nullopt);
};

}  // namespace seng::rendering
