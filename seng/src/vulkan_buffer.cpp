#include <cstdint>
#include <seng/log.hpp>
#include <seng/vulkan_buffer.hpp>
#include <seng/vulkan_command_buffer.hpp>
#include <seng/vulkan_device.hpp>
#include <vulkan/vulkan_structs.hpp>

using namespace std;
using namespace vk::raii;
using namespace seng::rendering;

static Buffer create(VulkanDevice &device,
                     vk::BufferUsageFlags usage,
                     uint64_t size);
static DeviceMemory allocate(VulkanDevice &device,
                             vk::MemoryRequirements reqs,
                             int32_t memId);

VulkanBuffer::VulkanBuffer(VulkanDevice &dev,
                           vk::BufferUsageFlags usage,
                           vk::DeviceSize size,
                           vk::MemoryPropertyFlags memFlags,
                           bool bind)
    : vkDevRef(dev),
      usage(usage),
      size(size),
      handle(create(vkDevRef, usage, size)),
      memRequirements(handle.getMemoryRequirements()),
      memIndex(vkDevRef.get().findMemoryIndex(memRequirements.memoryTypeBits,
                                              memFlags)),
      memory(allocate(vkDevRef, memRequirements, memIndex)),
      locked(false) {
  log::dbg("Allocated buffer");
  if (bind) this->bind(0);
}

Buffer create(VulkanDevice &device,
              vk::BufferUsageFlags usage,
              vk::DeviceSize size) {
  return Buffer(device.logical(),
                {{}, size, usage, vk::SharingMode::eExclusive});
}

DeviceMemory allocate(VulkanDevice &device,
                      vk::MemoryRequirements reqs,
                      int32_t memId) {
  vk::MemoryAllocateInfo info{};
  info.allocationSize = reqs.size;
  info.memoryTypeIndex = memId;
  return DeviceMemory(device.logical(), info);
}

void VulkanBuffer::bind(vk::DeviceSize offset) {
  handle.bindMemory(*memory, offset);
}

void VulkanBuffer::resize(vk::DeviceSize size,
                          Queue &queue,
                          CommandPool &pool) {
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
  auto temp = VulkanCommandBuffer::beginSingleUse(vkDevRef, pool);
  temp.buffer().copyBuffer(*this->handle, *dest, copyRegion);
  VulkanCommandBuffer::endSingleUse(temp, queue);
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
