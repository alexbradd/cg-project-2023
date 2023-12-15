#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <exception>
#include <limits>

namespace seng::rendering {

class VulkanDevice;

/**
 * Exception signaling an error occurred while waiting for a fence. It carries
 * the vk::Result returned by the correponding FenceWait.
 */
class FenceWaitException : std::exception {
 public:
  FenceWaitException(std::string_view error, vk::Result res) : err{error}, r{res} {}

  const char* what() const noexcept override { return err.c_str(); }
  vk::Result result() const noexcept { return r; }

 private:
  std::string err;
  vk::Result r;
};

/**
 * Wrapper class for a Vulkan fence. It implements the RAII pattern, meaning
 * that instantiating the class allocates, all resources, while destruction of
 * the class deallocate them.
 *
 * It is not copyable, only movable.
 */
class VulkanFence {
 public:
  /**
   * Create and allocate a new fence. If `makeSignaled` is set, the fence will
   * be instatiated as singaled.
   */
  VulkanFence(const VulkanDevice& device, bool makeSignaled = false);
  VulkanFence(const VulkanFence&) = delete;
  VulkanFence(VulkanFence&&) = default;

  VulkanFence& operator=(const VulkanFence&) = delete;
  VulkanFence& operator=(VulkanFence&&) = default;

  /**
   * Wait on this fence until the timeout is reached. If an error occurs while
   * waiting, a FenceWaitException is throuwn.
   */
  void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max());

  /**
   * Reset the fence
   */
  void reset();

  // Accessors
  const vk::raii::Fence& handle() const { return _handle; }
  bool signaled() const { return _signaled; }

 private:
  const VulkanDevice* vulkanDev;
  bool _signaled;
  vk::raii::Fence _handle;
};

}  // namespace seng::rendering
