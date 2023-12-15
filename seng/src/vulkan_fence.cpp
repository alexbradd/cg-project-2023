#include <seng/log.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_fence.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <functional>
#include <string>

using namespace seng::rendering;
using namespace vk::raii;
using namespace std;

VulkanFence::VulkanFence(const VulkanDevice &device, bool signaled) :
    vulkanDev(std::addressof(device)),
    _signaled(signaled),
    // Create the handle
    _handle(std::invoke([&]() {
      vk::FenceCreateInfo info{};
      if (signaled) info.flags |= vk::FenceCreateFlagBits::eSignaled;
      return Fence(vulkanDev->logical(), info);
    }))
{
}

void VulkanFence::wait(uint64_t timeout)
{
  if (!_signaled) {
    auto res = vulkanDev->logical().waitForFences(*_handle, true, timeout);
    string err;
    switch (res) {
      case vk::Result::eSuccess:
        _signaled = true;
        break;
      case vk::Result::eTimeout:
        err = "VulkanFence.wait - Timed out";
        log::warning("{}", err);
        throw FenceWaitException(err, res);
        break;
      case vk::Result::eErrorDeviceLost:
        err = "VulkanFence.wait - VK_ERROR_DEVICE_LOST";
        [[fallthrough]];
      case vk::Result::eErrorOutOfHostMemory:
        err = "VulkanFence.wait - VK_ERROR_OUT_OF_HOST_MEMORY";
        [[fallthrough]];
      case vk::Result::eErrorOutOfDeviceMemory:
        err = "VulkanFence.wait - VK_ERROR_OUT_OF_DEVICE_MEMORY";
        [[fallthrough]];
      default:
        err = "VulkanFence.wait - An unknown error has occurred";
        log::error("{}", err);
        throw FenceWaitException(err, res);
    }
  }
}

void VulkanFence::reset()
{
  if (_signaled) {
    vulkanDev->logical().resetFences(*_handle);
    _signaled = false;
  }
}
