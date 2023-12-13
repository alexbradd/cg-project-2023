#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

/**
 * Wrapper around a Vulkan CommandBuffer. It implements the RAII pattern,
 * meaning that instantiating the class will allocate a new buffer from the
 * pool, while destroying it will deallocate the buffer.
 *
 * It is not copyable, only movable.
 */
class VulkanCommandBuffer {
 public:

  /**
   * Create and allocate from the given pool a new CommandBuffer.
   */
  VulkanCommandBuffer(VulkanDevice& dev,
                      vk::raii::CommandPool& pool,
                      bool primary = true);
  VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer(VulkanCommandBuffer&&) = default;
  ~VulkanCommandBuffer();

  VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) = default;

  /**
   * Start recording the command buffer. The arguments mirror the flags one can
   * insert into CommandBufferBeginInfo.
   */
  void begin(bool singleUse = false,
             bool renderPassContinue = false,
             bool simultaneousUse = false);

  /**
   * End recording the command buffer.
   */
  void end();

  /**
   * Reset the command buffer.
   */
  void reset();

  // Access the underlying handle
  vk::raii::CommandBuffer& buffer() { return buf; }

  /**
   * Factory method for allocating multiple command buffers with one call.
   */
  static std::vector<VulkanCommandBuffer> createMultiple(VulkanDevice& dev,
                                                         vk::raii::CommandPool& pool,
                                                         uint32_t n,
                                                         bool primary = true);

  /**
   * Allocate a throwaway single-use buffer and start recording it. Then execute
   * the given function. Once done, end the recording and deallocate the buffer.
   *
   * The lambda will receive a reference to the temporary VulkanBuffer.
   */
  static void recordSingleUse(VulkanDevice& dev,
                              vk::raii::CommandPool& pool,
                              vk::raii::Queue& queue,
                              std::function<void(VulkanCommandBuffer&)> usage);

 private:
  VulkanCommandBuffer(vk::raii::CommandBuffer&& buf);

  vk::raii::CommandBuffer buf;
};

}  // namespace seng::rendering
