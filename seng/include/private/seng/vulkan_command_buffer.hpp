#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

class VulkanCommandBuffer {
 public:
  enum struct State {
    eReady,
    eRecording,
    eInRenderPass,
    eRecordingEnded,
    eSubmitted
  };

  VulkanCommandBuffer(VulkanDevice& dev,
                      vk::raii::CommandPool& pool,
                      bool primary = true);
  VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer(VulkanCommandBuffer&&) = default;

  VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) = default;

  void begin(bool singleUse = false,
             bool renderPassContinue = false,
             bool simultaneousUse = false);
  void end();

  void reset();
  void setRecording();
  void setInRenderPass();
  void setSubmitted();

  vk::raii::CommandBuffer& buffer() { return buf; }

  static std::vector<VulkanCommandBuffer> createMultiple(
      VulkanDevice& dev,
      vk::raii::CommandPool& pool,
      uint32_t n,
      bool primary = true);

  static VulkanCommandBuffer beginSingleUse(VulkanDevice& dev,
                                            vk::raii::CommandPool& pool);
  static void endSingleUse(VulkanCommandBuffer& buf, vk::raii::Queue& queue);

 private:
  VulkanCommandBuffer(vk::raii::CommandBuffer&& buf);

  vk::raii::CommandBuffer buf;
  enum State state;
};

}  // namespace seng::rendering
