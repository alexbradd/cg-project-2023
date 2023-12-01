#pragma once

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
   * Possible states in which the command buffer may be.
   */
  enum struct State {
    eReady,
    eRecording,
    eInRenderPass,
    eRecordingEnded,
    eSubmitted
  };

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

  // State modifiers
  void setRecording();
  void setInRenderPass();
  void setSubmitted();

  // Access the underlying handle
  vk::raii::CommandBuffer& buffer() { return buf; }

  /**
   * Factory method for allocating multiple command buffers with one call.
   */
  static std::vector<VulkanCommandBuffer> createMultiple(
      VulkanDevice& dev,
      vk::raii::CommandPool& pool,
      uint32_t n,
      bool primary = true);

  /**
   * Allocate a throwaway single-use buffer and start recording it. Should be
   * matched by a call to VulkanCommandBuffer::endSingleUse.
   */
  static VulkanCommandBuffer beginSingleUse(VulkanDevice& dev,
                                            vk::raii::CommandPool& pool);

  /**
   * End recording and submit a single use CommandBuffer created by
   * VulkanCommandBuffer::beginSingleUse.
   */
  static void endSingleUse(VulkanCommandBuffer& buf, vk::raii::Queue& queue);

 private:
  VulkanCommandBuffer(vk::raii::CommandBuffer&& buf);

  vk::raii::CommandBuffer buf;
  enum State state;
};

}  // namespace seng::rendering
