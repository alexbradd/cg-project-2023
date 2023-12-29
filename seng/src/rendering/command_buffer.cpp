#include <seng/log.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace seng::rendering;

using SingleUse = CommandBuffer::SingleUse;
using RenderPassContinue = CommandBuffer::RenderPassContinue;
using SimultanousUse = CommandBuffer::SimultaneousUse;

static vk::raii::CommandBuffers allocateBuffers(const vk::raii::Device &device,
                                                const vk::raii::CommandPool &pool,
                                                uint32_t n,
                                                bool primary);

CommandBuffer::CommandBuffer(const Device &dev,
                             const vk::raii::CommandPool &pool,
                             bool primary) :
    CommandBuffer(std::move(allocateBuffers(dev.logical(), pool, 1, primary)[0]))
{
}

CommandBuffer::CommandBuffer(vk::raii::CommandBuffer &&b) : m_buf(std::move(b))
{
  log::dbg("Allocated command buffer");
}

vk::raii::CommandBuffers allocateBuffers(const vk::raii::Device &device,
                                         const vk::raii::CommandPool &pool,
                                         uint32_t n,
                                         bool primary)
{
  vk::CommandBufferAllocateInfo allocate_info{};
  allocate_info.commandPool = *pool;
  allocate_info.level =
      primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary;
  allocate_info.commandBufferCount = n;

  return vk::raii::CommandBuffers(device, allocate_info);
}

vector<CommandBuffer> CommandBuffer::createMultiple(const Device &dev,
                                                    const vk::raii::CommandPool &pool,
                                                    uint32_t n,
                                                    bool primary)
{
  vk::raii::CommandBuffers bufs(allocateBuffers(dev.logical(), pool, n, primary));

  vector<CommandBuffer> ret;
  ret.reserve(n);
  for (auto &b : bufs) ret.emplace_back(CommandBuffer(std::move(b)));

  return ret;
}

void CommandBuffer::begin(SingleUse single,
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

  m_buf.begin(info);
}

void CommandBuffer::end() const
{
  m_buf.end();
}

void CommandBuffer::recordSingleUse(const Device &dev,
                                    const vk::raii::CommandPool &pool,
                                    const vk::raii::Queue &q,
                                    function<void(CommandBuffer &)> usage)
{
  CommandBuffer buf(dev, pool);
  buf.begin(SingleUse::eOn);

  usage(buf);

  buf.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &(*buf.buffer());
  q.submit(submitInfo);
  q.waitIdle();
}

void CommandBuffer::reset() const
{
  m_buf.reset();
}

CommandBuffer::~CommandBuffer()
{
  if (*m_buf != vk::CommandBuffer{}) {
    log::dbg("Deallocating command buffer");
  }
}
