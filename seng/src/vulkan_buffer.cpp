#include <seng/log.hpp>
#include <seng/vulkan_buffer.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_device.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <string.h>  // for memcpy
#include <functional>
#include <string>
#include <utility>

using namespace std;
using namespace vk::raii;
using namespace seng::rendering;

VulkanBuffer::VulkanBuffer(const VulkanDevice &dev,
                           vk::BufferUsageFlags usage,
                           vk::DeviceSize size,
                           vk::MemoryPropertyFlags memFlags,
                           bool bind) :
    vulkanDev(std::addressof(dev)),
    usage(usage),
    size(size),
    handle(Buffer(dev.logical(), {{}, size, usage, vk::SharingMode::eExclusive})),
    memRequirements(handle.getMemoryRequirements()),
    memIndex(dev.findMemoryIndex(memRequirements.memoryTypeBits, memFlags)),
    memory(dev.logical(), vk::MemoryAllocateInfo{memRequirements.size, memIndex})
{
  log::dbg("Allocated buffer");
  if (bind) this->bind(0);
}

void VulkanBuffer::bind(vk::DeviceSize offset) const
{
  handle.bindMemory(*memory, offset);
}

void VulkanBuffer::resize(vk::DeviceSize size,
                          const Queue &queue,
                          const CommandPool &pool)
{
  // Create new buffer
  vk::BufferCreateInfo newInfo{{}, size, usage, vk::SharingMode::eExclusive};
  Buffer newBuffer(vulkanDev->logical(), newInfo);

  // Allocate new buffer
  vk::MemoryRequirements newMemRequirements(newBuffer.getMemoryRequirements());
  vk::MemoryAllocateInfo newAllocateInfo{newMemRequirements.size, memIndex};
  DeviceMemory newMemory(vulkanDev->logical(), newAllocateInfo);
  newBuffer.bindMemory(*newMemory, 0);

  // Copy old contents to new buffer
  this->rawCopy(newBuffer, {0, 0, this->size}, pool, queue);
  vulkanDev->logical().waitIdle();

  // Replace the old handles (RAII takes care of deallocation)
  this->size = size;
  this->memRequirements = newMemRequirements;
  this->memory = std::move(newMemory);
  this->handle = std::move(newBuffer);
}

void *VulkanBuffer::lockMemory(vk::DeviceSize offset,
                               vk::DeviceSize size,
                               vk::MemoryMapFlags flags) const
{
  return memory.mapMemory(offset, size, flags);
}

void VulkanBuffer::unlockMemory() const
{
  memory.unmapMemory();
}

void VulkanBuffer::load(const void *data,
                        vk::DeviceSize offset,
                        vk::DeviceSize size,
                        vk::MemoryMapFlags flags) const
{
  void *mem = lockMemory(offset, size, flags);
  memcpy(mem, data, size);
  unlockMemory();
}

void VulkanBuffer::rawCopy(const Buffer &dest,
                           vk::BufferCopy copyRegion,
                           const CommandPool &pool,
                           const Queue &queue,
                           [[maybe_unused]] const Fence *fence) const
{
  queue.waitIdle();
  VulkanCommandBuffer::recordSingleUse(*vulkanDev, pool, queue, [&](auto &buf) {
    buf.buffer().copyBuffer(*handle, *dest, copyRegion);
  });
}

void VulkanBuffer::copy(const VulkanBuffer &dest,
                        vk::BufferCopy copyRegion,
                        const CommandPool &pool,
                        const Queue &queue,
                        const Fence *fence) const
{
  rawCopy(dest.handle, copyRegion, pool, queue, fence);
}

VulkanBuffer::~VulkanBuffer()
{
  if (*handle != vk::Buffer{}) log::dbg("Destroying buffer");
}
