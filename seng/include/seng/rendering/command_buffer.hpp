#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <functional>

namespace seng::rendering {

class Device;

/**
 * Wrapper around a Vulkan CommandBuffer. It implements the RAII pattern,
 * meaning that instantiating the class will allocate a new buffer from the
 * pool, while destroying it will deallocate the buffer.
 *
 * It is not copyable, only movable.
 */
class CommandBuffer {
 public:
  enum struct SingleUse : bool { eOn = true, eOff = false };
  enum struct RenderPassContinue : bool { eOn = true, eOff = false };
  enum struct SimultaneousUse : bool { eOn = true, eOff = false };

  /**
   * Create and allocate from the given pool a new CommandBuffer.
   */
  CommandBuffer(const Device& dev,
                const vk::raii::CommandPool& pool,
                bool primary = true);
  CommandBuffer(const CommandBuffer&) = delete;
  CommandBuffer(CommandBuffer&&) = default;
  ~CommandBuffer();

  CommandBuffer& operator=(const CommandBuffer&) = delete;
  CommandBuffer& operator=(CommandBuffer&&) = default;

  /**
   * Start recording the command buffer. The arguments mirror the flags one can
   * insert into CommandBufferBeginInfo.
   */
  void begin(SingleUse singleUse = SingleUse::eOff,
             RenderPassContinue renderPassContinue = RenderPassContinue::eOff,
             SimultaneousUse simultaneousUse = SimultaneousUse::eOff) const;

  /**
   * End recording the command buffer.
   */
  void end() const;

  /**
   * Reset the command buffer.
   */
  void reset() const;

  // Access the underlying handle
  const vk::raii::CommandBuffer& buffer() const { return m_buf; }

  /**
   * Factory method for allocating multiple command buffers with one call.
   */
  static std::vector<CommandBuffer> createMultiple(const Device& dev,
                                                   const vk::raii::CommandPool& pool,
                                                   uint32_t n,
                                                   bool primary = true);

  /**
   * Allocate a throwaway single-use buffer and start recording it. Then execute
   * the given function. Once done, end the recording and deallocate the buffer.
   *
   * The lambda will receive a reference to the temporary VulkanBuffer.
   */
  static void recordSingleUse(const Device& dev,
                              const vk::raii::CommandPool& pool,
                              const vk::raii::Queue& queue,
                              std::function<void(CommandBuffer&)> usage);

 private:
  CommandBuffer(vk::raii::CommandBuffer&& buf);
  vk::raii::CommandBuffer m_buf;
};

}  // namespace seng::rendering
