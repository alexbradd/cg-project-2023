#pragma once

#include <cstdint>
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
      const VulkanDevice &dev,
      vk::BufferUsageFlags usage,
      vk::DeviceSize size,
      vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
      bool bind = true);
  VulkanBuffer(const VulkanBuffer &) = delete;
  VulkanBuffer(VulkanBuffer &&) = default;
  ~VulkanBuffer();

  VulkanBuffer &operator=(const VulkanBuffer &) = delete;
  VulkanBuffer &operator=(VulkanBuffer &&) = default;

  const vk::raii::Buffer &buffer() const { return handle; }

  /**
   * Bind the buffer at the give offset
   */
  void bind(vk::DeviceSize offset) const;

  /**
   * Resize the buffer to the new size.
   */
  void resize(vk::DeviceSize size,
              const vk::raii::Queue &queue,
              const vk::raii::CommandPool &pool);

  /**
   * Lock the memory of the buffer.
   */
  void *lockMemory(vk::DeviceSize size,
                   vk::DeviceSize offset,
                   vk::MemoryMapFlags flags) const;

  /**
   * Unlock the buffer.
   */
  void unlockMemory() const;

  /**
   * Lock the buffer and copy the given amount of data into it at the given
   * offset
   */
  void load(const void *data,
            vk::DeviceSize offset,
            vk::DeviceSize size,
            vk::MemoryMapFlags flags) const;

  /**
   * Copy a region of this buffer into one of the destination buffer.
   */
  void copy(const VulkanBuffer &dest,
            vk::BufferCopy copyRegion,
            const vk::raii::CommandPool &pool,
            const vk::raii::Queue &queue,
            const vk::raii::Fence *fence = nullptr) const;

 private:
  const VulkanDevice *vulkanDev;
  vk::BufferUsageFlags usage;
  vk::DeviceSize size;
  vk::raii::Buffer handle;
  vk::MemoryRequirements memRequirements;
  uint32_t memIndex;
  vk::raii::DeviceMemory memory;

  /**
   * Copy a region of this buffer into one of the destination buffer.
   */
  void rawCopy(const vk::raii::Buffer &dest,
               vk::BufferCopy copyRegion,
               const vk::raii::CommandPool &pool,
               const vk::raii::Queue &queue,
               const vk::raii::Fence *fence = nullptr) const;
};

}  // namespace seng::rendering
