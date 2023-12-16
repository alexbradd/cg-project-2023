#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <string.h>  // for memcpy
#include <functional>
#include <string>
#include <utility>

using namespace std;
using namespace seng::rendering;

Buffer::Buffer(const Device &dev,
               vk::BufferUsageFlags usage,
               vk::DeviceSize size,
               vk::MemoryPropertyFlags memFlags,
               bool bind) :
    vulkanDev(std::addressof(dev)),
    usage(usage),
    size(size),
    handle(
        vk::raii::Buffer(dev.logical(), {{}, size, usage, vk::SharingMode::eExclusive})),
    memRequirements(handle.getMemoryRequirements()),
    memIndex(dev.findMemoryIndex(memRequirements.memoryTypeBits, memFlags)),
    memory(dev.logical(), vk::MemoryAllocateInfo{memRequirements.size, memIndex})
{
  log::dbg("Allocated buffer");
  if (bind) this->bind(0);
}

void Buffer::bind(vk::DeviceSize offset) const
{
  handle.bindMemory(*memory, offset);
}

void Buffer::resize(vk::DeviceSize size,
                    const vk::raii::Queue &queue,
                    const vk::raii::CommandPool &pool)
{
  // Create new buffer
  vk::BufferCreateInfo newInfo{{}, size, usage, vk::SharingMode::eExclusive};
  vk::raii::Buffer newBuffer(vulkanDev->logical(), newInfo);

  // Allocate new buffer
  vk::MemoryRequirements newMemRequirements(newBuffer.getMemoryRequirements());
  vk::MemoryAllocateInfo newAllocateInfo{newMemRequirements.size, memIndex};
  vk::raii::DeviceMemory newMemory(vulkanDev->logical(), newAllocateInfo);
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

void *Buffer::lockMemory(vk::DeviceSize offset,
                         vk::DeviceSize size,
                         vk::MemoryMapFlags flags) const
{
  return memory.mapMemory(offset, size, flags);
}

void Buffer::unlockMemory() const
{
  memory.unmapMemory();
}

void Buffer::load(const void *data,
                  vk::DeviceSize offset,
                  vk::DeviceSize size,
                  vk::MemoryMapFlags flags) const
{
  void *mem = lockMemory(offset, size, flags);
  memcpy(mem, data, size);
  unlockMemory();
}

void Buffer::rawCopy(const vk::raii::Buffer &dest,
                     vk::BufferCopy copyRegion,
                     const vk::raii::CommandPool &pool,
                     const vk::raii::Queue &queue,
                     [[maybe_unused]] const vk::raii::Fence *fence) const
{
  queue.waitIdle();
  CommandBuffer::recordSingleUse(*vulkanDev, pool, queue, [&](auto &buf) {
    buf.buffer().copyBuffer(*handle, *dest, copyRegion);
  });
}

void Buffer::copy(const Buffer &dest,
                  vk::BufferCopy copyRegion,
                  const vk::raii::CommandPool &pool,
                  const vk::raii::Queue &queue,
                  const vk::raii::Fence *fence) const
{
  rawCopy(dest.handle, copyRegion, pool, queue, fence);
}

Buffer::~Buffer()
{
  if (*handle != vk::Buffer{}) log::dbg("Destroying buffer");
}
