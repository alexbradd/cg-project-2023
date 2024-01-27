#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <string.h>  // for memcpy
#include <functional>
#include <string>
#include <utility>

#define BAIL_OUT_ON_UNINITIALIZED(ret)                                     \
  do {                                                                     \
    if (m_device == nullptr) {                                             \
      seng::log::error("Calling emthod on unitialized buffer, biling..."); \
      return ret;                                                          \
    }                                                                      \
  } while (0)

using namespace std;
using namespace seng::rendering;

Buffer::Buffer(std::nullptr_t) :
    m_device(nullptr),
    m_usage{},
    m_size{},
    m_handle(nullptr),
    m_memRequirements{},
    m_memIndex{},
    m_memory(nullptr)
{
}

Buffer::Buffer(const Device &dev,
               vk::BufferUsageFlags usage,
               vk::DeviceSize size,
               vk::MemoryPropertyFlags memFlags,
               bool bind) :
    m_device(std::addressof(dev)),
    m_usage(usage),
    m_size(size),
    m_handle(
        vk::raii::Buffer(dev.logical(), {{}, size, usage, vk::SharingMode::eExclusive})),
    m_memRequirements(m_handle.getMemoryRequirements()),
    m_memIndex(dev.findMemoryIndex(m_memRequirements.memoryTypeBits, memFlags)),
    m_memory(dev.logical(), vk::MemoryAllocateInfo{m_memRequirements.size, m_memIndex})
{
  log::dbg("Allocated buffer");
  if (bind) this->bind(0);
}

void Buffer::bind(vk::DeviceSize offset) const
{
  BAIL_OUT_ON_UNINITIALIZED();
  m_handle.bindMemory(*m_memory, offset);
}

void Buffer::resize(vk::DeviceSize size,
                    const vk::raii::Queue &queue,
                    const vk::raii::CommandPool &pool)
{
  BAIL_OUT_ON_UNINITIALIZED();

  // Create new buffer
  vk::BufferCreateInfo newInfo{{}, size, m_usage, vk::SharingMode::eExclusive};
  vk::raii::Buffer newBuffer(m_device->logical(), newInfo);

  // Allocate new buffer
  vk::MemoryRequirements newMemRequirements(newBuffer.getMemoryRequirements());
  vk::MemoryAllocateInfo newAllocateInfo{newMemRequirements.size, m_memIndex};
  vk::raii::DeviceMemory newMemory(m_device->logical(), newAllocateInfo);
  newBuffer.bindMemory(*newMemory, 0);

  // Copy old contents to new buffer
  this->rawCopy(newBuffer, {0, 0, this->m_size}, pool, queue);
  m_device->logical().waitIdle();

  // Replace the old handles (RAII takes care of deallocation)
  this->m_size = size;
  this->m_memRequirements = newMemRequirements;
  this->m_memory = std::move(newMemory);
  this->m_handle = std::move(newBuffer);
}

void *Buffer::lockMemory(vk::DeviceSize offset,
                         vk::DeviceSize size,
                         vk::MemoryMapFlags flags) const
{
  BAIL_OUT_ON_UNINITIALIZED(nullptr);
  return m_memory.mapMemory(offset, size, flags);
}

void Buffer::unlockMemory() const
{
  BAIL_OUT_ON_UNINITIALIZED();
  m_memory.unmapMemory();
}

void Buffer::load(const void *data,
                  vk::DeviceSize offset,
                  vk::DeviceSize size,
                  vk::MemoryMapFlags flags) const
{
  BAIL_OUT_ON_UNINITIALIZED();
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
  CommandBuffer::recordSingleUse(*m_device, pool, queue, [&](auto &buf) {
    buf.buffer().copyBuffer(*m_handle, *dest, copyRegion);
  });
}

void Buffer::copy(const Buffer &dest,
                  vk::BufferCopy copyRegion,
                  const vk::raii::CommandPool &pool,
                  const vk::raii::Queue &queue,
                  const vk::raii::Fence *fence) const
{
  BAIL_OUT_ON_UNINITIALIZED();
  rawCopy(dest.m_handle, copyRegion, pool, queue, fence);
}

Buffer::~Buffer()
{
  if (*m_handle != vk::Buffer{}) log::dbg("Destroying buffer");
}
