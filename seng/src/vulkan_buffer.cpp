#include <seng/log.hpp>
#include <seng/vulkan_buffer.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_device.hpp>
#include <vulkan/vulkan_enums.hpp>

using namespace std;
using namespace vk::raii;
using namespace seng::rendering;

VulkanBuffer::VulkanBuffer(VulkanDevice &dev,
                           vk::BufferUsageFlags usage,
                           vk::DeviceSize size,
                           vk::MemoryPropertyFlags memFlags,
                           bool bind) :
    vkDevRef(dev),
    usage(usage),
    size(size),
    handle(Buffer(dev.logical(), {{}, size, usage, vk::SharingMode::eExclusive})),
    memRequirements(handle.getMemoryRequirements()),
    memIndex(vkDevRef.get().findMemoryIndex(memRequirements.memoryTypeBits, memFlags)),
    memory(dev.logical(), vk::MemoryAllocateInfo{memRequirements.size, memIndex}),
    locked(false) {
  log::dbg("Allocated buffer");
  if (bind) this->bind(0);
}

void VulkanBuffer::bind(vk::DeviceSize offset) {
  handle.bindMemory(*memory, offset);
}

void VulkanBuffer::resize(vk::DeviceSize size, Queue &queue, CommandPool &pool) {
  // Create new buffer
  vk::BufferCreateInfo newInfo{{}, size, usage, vk::SharingMode::eExclusive};
  Buffer newBuffer(vkDevRef.get().logical(), newInfo);

  // Allocate new buffer
  vk::MemoryRequirements newMemRequirements(newBuffer.getMemoryRequirements());
  vk::MemoryAllocateInfo newAllocateInfo{newMemRequirements.size, memIndex};
  DeviceMemory newMemory(vkDevRef.get().logical(), newAllocateInfo);
  newBuffer.bindMemory(*newMemory, 0);

  // Copy old contents to new buffer
  this->rawCopy(newBuffer, {0, 0, this->size}, pool, queue);
  vkDevRef.get().logical().waitIdle();

  // Replace the old handles (RAII takes care of deallocation)
  this->size = size;
  this->memRequirements = newMemRequirements;
  this->memory = std::move(newMemory);
  this->handle = std::move(newBuffer);
}

void *VulkanBuffer::lockMemory(vk::DeviceSize offset,
                               vk::DeviceSize size,
                               vk::MemoryMapFlags flags) {
  locked = true;
  return memory.mapMemory(offset, size, flags);
}

void VulkanBuffer::unlockMemory() {
  locked = false;
  memory.unmapMemory();
}

void VulkanBuffer::load(const void *data,
                        vk::DeviceSize offset,
                        vk::DeviceSize size,
                        vk::MemoryMapFlags flags) {
  void *mem = lockMemory(offset, size, flags);
  memcpy(mem, data, size);
  unlockMemory();
}

void VulkanBuffer::rawCopy(Buffer &dest,
                           vk::BufferCopy copyRegion,
                           CommandPool &pool,
                           Queue &queue,
                           optional<reference_wrapper<Fence>>) {
  queue.waitIdle();
  VulkanCommandBuffer::recordSingleUse(vkDevRef, pool, queue, [&](auto &buf) {
    buf.buffer().copyBuffer(*handle, *dest, copyRegion);
  });
}

void VulkanBuffer::copy(VulkanBuffer &dest,
                        vk::BufferCopy copyRegion,
                        CommandPool &pool,
                        Queue &queue,
                        optional<reference_wrapper<Fence>> fence) {
  rawCopy(dest.handle, copyRegion, pool, queue, fence);
}

VulkanBuffer::~VulkanBuffer() {
  if (*handle != vk::Buffer{}) log::dbg("Destroying buffer");
}