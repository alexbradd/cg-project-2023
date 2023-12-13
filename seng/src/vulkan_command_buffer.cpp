#include <seng/log.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_device.hpp>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

static CommandBuffers allocateBuffers(Device &device,
                                      CommandPool &pool,
                                      uint32_t n,
                                      bool primary);

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice &dev,
                                         CommandPool &pool,
                                         bool primary) :
    VulkanCommandBuffer(std::move(allocateBuffers(dev.logical(), pool, 1, primary)[0]))
{
}

VulkanCommandBuffer::VulkanCommandBuffer(CommandBuffer &&b) : buf(std::move(b))
{
  log::dbg("Allocated command buffer");
}

CommandBuffers allocateBuffers(Device &device,
                               CommandPool &pool,
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
    VulkanDevice &dev, vk::raii::CommandPool &pool, uint32_t n, bool primary)
{
  CommandBuffers bufs(allocateBuffers(dev.logical(), pool, n, primary));

  vector<VulkanCommandBuffer> ret;
  ret.reserve(n);
  for (auto &b : bufs) ret.emplace_back(VulkanCommandBuffer(std::move(b)));

  return ret;
}

void VulkanCommandBuffer::begin(bool singleUse,
                                bool renderPassContinue,
                                bool simultaneousUse)
{
  vk::CommandBufferBeginInfo info{};

  if (singleUse) info.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  if (renderPassContinue)
    info.flags |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
  if (simultaneousUse) info.flags |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;

  buf.begin(info);
}

void VulkanCommandBuffer::end()
{
  buf.end();
}

void VulkanCommandBuffer::recordSingleUse(VulkanDevice &dev,
                                          CommandPool &pool,
                                          Queue &q,
                                          function<void(VulkanCommandBuffer &)> usage)
{
  VulkanCommandBuffer buf(dev, pool);
  buf.begin(true);

  usage(buf);

  buf.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &(*buf.buffer());
  q.submit(submitInfo);
  q.waitIdle();
}

void VulkanCommandBuffer::reset()
{
  buf.reset();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
  if (*buf != vk::CommandBuffer{}) {
    log::dbg("Deallocating command buffer");
  }
}
