#include <seng/log.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_device.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

using SingleUse = VulkanCommandBuffer::SingleUse;
using RenderPassContinue = VulkanCommandBuffer::RenderPassContinue;
using SimultanousUse = VulkanCommandBuffer::SimultaneousUse;

static CommandBuffers allocateBuffers(const Device &device,
                                      const CommandPool &pool,
                                      uint32_t n,
                                      bool primary);

VulkanCommandBuffer::VulkanCommandBuffer(const VulkanDevice &dev,
                                         const CommandPool &pool,
                                         bool primary) :
    VulkanCommandBuffer(std::move(allocateBuffers(dev.logical(), pool, 1, primary)[0]))
{
}

VulkanCommandBuffer::VulkanCommandBuffer(CommandBuffer &&b) : buf(std::move(b))
{
  log::dbg("Allocated command buffer");
}

CommandBuffers allocateBuffers(const Device &device,
                               const CommandPool &pool,
                               uint32_t n,
                               bool primary)
{
  vk::CommandBufferAllocateInfo allocate_info{};
  allocate_info.commandPool = *pool;
  allocate_info.level =
      primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary;
  allocate_info.commandBufferCount = n;

  return CommandBuffers(device, allocate_info);
}

vector<VulkanCommandBuffer> VulkanCommandBuffer::createMultiple(
    const VulkanDevice &dev, const vk::raii::CommandPool &pool, uint32_t n, bool primary)
{
  CommandBuffers bufs(allocateBuffers(dev.logical(), pool, n, primary));

  vector<VulkanCommandBuffer> ret;
  ret.reserve(n);
  for (auto &b : bufs) ret.emplace_back(VulkanCommandBuffer(std::move(b)));

  return ret;
}

void VulkanCommandBuffer::begin(SingleUse single,
                                RenderPassContinue passContinue,
                                SimultaneousUse simultaneous) const
{
  vk::CommandBufferBeginInfo info{};

  if (single == SingleUse::eOn)
    info.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  if (passContinue == RenderPassContinue::eOn)
    info.flags |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
  if (simultaneous == SimultaneousUse::eOn)
    info.flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;

  buf.begin(info);
}

void VulkanCommandBuffer::end() const
{
  buf.end();
}

void VulkanCommandBuffer::recordSingleUse(const VulkanDevice &dev,
                                          const CommandPool &pool,
                                          const Queue &q,
                                          function<void(VulkanCommandBuffer &)> usage)
{
  VulkanCommandBuffer buf(dev, pool);
  buf.begin(SingleUse::eOn);

  usage(buf);

  buf.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &(*buf.buffer());
  q.submit(submitInfo);
  q.waitIdle();
}

void VulkanCommandBuffer::reset() const
{
  buf.reset();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
  if (*buf != vk::CommandBuffer{}) {
    log::dbg("Deallocating command buffer");
  }
}
