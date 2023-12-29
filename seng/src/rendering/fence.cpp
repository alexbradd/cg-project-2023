#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/fence.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <functional>
#include <string>

using namespace seng::rendering;
using namespace std;

Fence::Fence(const Device &device, bool signaled) :
    m_device(std::addressof(device)),
    m_signaled(signaled),
    // Create the handle
    m_handle(std::invoke([&]() {
      vk::FenceCreateInfo info{};
      if (signaled) info.flags |= vk::FenceCreateFlagBits::eSignaled;
      return vk::raii::Fence(m_device->logical(), info);
    }))
{
}

void Fence::wait(uint64_t timeout)
{
  if (!m_signaled) {
    auto res = m_device->logical().waitForFences(*m_handle, true, timeout);
    string err;
    switch (res) {
      case vk::Result::eSuccess:
        m_signaled = true;
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

void Fence::reset()
{
  if (m_signaled) {
    m_device->logical().resetFences(*m_handle);
    m_signaled = false;
  }
}
