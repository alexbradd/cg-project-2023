#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

class VulkanDevice;

/**
 * Exception singaling an outof date or suboptimal swapchain. Users of the class
 * should recreate the swapchain and relative framebuffers if they catch this
 */
class FenceWaitException : std::exception {
 public:
  FenceWaitException(std::string_view error, vk::Result res)
      : err{error}, r{res} {}

  const char* what() const noexcept override { return err.c_str(); }
  vk::Result result() const noexcept { return r; }

 private:
  std::string err;
  vk::Result r;
};

class VulkanFence {
 public:
  VulkanFence(VulkanDevice& device, bool makeSignaled = false);
  VulkanFence(const VulkanFence&) = delete;
  VulkanFence(VulkanFence&&) = default;

  VulkanFence& operator=(const VulkanFence&) = delete;
  VulkanFence& operator=(VulkanFence&&) = default;

  void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max());
  void reset();

  vk::raii::Fence& handle() { return _handle; }
  bool signaled() { return _signaled; }

 private:
  std::reference_wrapper<VulkanDevice> vkDevRef;
  bool _signaled;
  vk::raii::Fence _handle;
};

}  // namespace seng::rendering
