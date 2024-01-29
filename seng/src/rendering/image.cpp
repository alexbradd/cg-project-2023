#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/image.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

#define BAIL_OUT_ON_UNINITIALIZED(ret)                                     \
  do {                                                                     \
    if (m_device == nullptr) {                                             \
      seng::log::error("Calling emthod on unitialized image, bailing..."); \
      return ret;                                                          \
    }                                                                      \
  } while (0)

using namespace std;
using namespace seng::rendering;

static bool hasStencilComponent(vk::Format format)
{
  return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

static uint32_t mipLevels(vk::Extent3D e)
{
  using namespace std;
  auto mips = floor(log2(max(max(e.width, e.height), e.depth))) + 1;
  return static_cast<uint32_t>(mips);
}

Image::Image(const Device &dev, const Image::CreateInfo &info) :
    m_device(std::addressof(dev)),
    // Create image handle
    m_handle(std::invoke([&]() {
      vk::ImageCreateInfo ci{};
      ci.imageType = info.type;
      ci.extent = info.extent;
      ci.mipLevels = info.mipped ? mipLevels(info.extent) : true;
      ci.arrayLayers = 1;
      ci.format = info.format;
      ci.tiling = info.tiling;
      ci.initialLayout = vk::ImageLayout::eUndefined;
      ci.usage = info.usage;
      ci.samples = vk::SampleCountFlagBits::e1;
      ci.sharingMode = vk::SharingMode::eExclusive;
      return vk::raii::Image(dev.logical(), ci);
    })),
    // Allocate memory
    m_memory(std::invoke([&]() {
      vk::MemoryRequirements requirements(m_handle.getMemoryRequirements());
      uint32_t memoryIndex =
          dev.findMemoryIndex(requirements.memoryTypeBits, info.memoryFlags);

      vk::MemoryAllocateInfo ai;
      ai.allocationSize = requirements.size;
      ai.memoryTypeIndex = memoryIndex;

      vk::raii::DeviceMemory ret(dev.logical(), ai);
      m_handle.bindMemory(*ret, 0);

      return ret;
    })),
    m_extent(info.extent),
    m_mipped(false),
    m_unmanaged(nullptr),
    m_view(nullptr)
{
  log::dbg("Created new image");
  if (info.createView) this->createView(info.viewType, info.format, info.aspectFlags);
}

Image::Image(const Device &dev, vk::Image wrapped, bool mipped) :
    m_device(std::addressof(dev)),
    m_handle(nullptr),
    m_memory(nullptr),
    m_extent{0, 0, 0},
    m_mipped(mipped),
    m_unmanaged(wrapped),
    m_view(nullptr)
{
}

Image::Image(std::nullptr_t) :
    m_device(nullptr),
    m_handle(nullptr),
    m_memory(nullptr),
    m_extent{0, 0, 0},
    m_mipped(false),
    m_unmanaged(nullptr),
    m_view(nullptr)
{
}

void Image::createView(vk::ImageViewType type,
                       vk::Format format,
                       vk::ImageAspectFlags aspect)
{
  BAIL_OUT_ON_UNINITIALIZED();

  vk::ImageViewCreateInfo ci{};
  ci.image = image();
  ci.viewType = type;
  ci.format = format;
  ci.subresourceRange.aspectMask = aspect;
  ci.subresourceRange.baseMipLevel = 0;
  ci.subresourceRange.levelCount = m_mipped ? mipLevels(m_extent) : 1;
  ci.subresourceRange.baseArrayLayer = 0;
  ci.subresourceRange.layerCount = 1;
  m_view = vk::raii::ImageView(m_device->logical(), ci);
  log::dbg("Created new image view");
}

void Image::copyFromBuffer(const CommandBuffer &commandBuf, const Buffer &buf) const
{
  BAIL_OUT_ON_UNINITIALIZED();

  vk::BufferImageCopy region;
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageExtent = m_extent;
  region.imageExtent.depth = 1;

  commandBuf.buffer().copyBufferToImage(*buf.buffer(), image(),
                                        vk::ImageLayout::eTransferDstOptimal, region);
}

void Image::transitionLayout(const CommandBuffer &commandBuf,
                             vk::Format format,
                             vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout) const
{
  BAIL_OUT_ON_UNINITIALIZED();

  vk::ImageMemoryBarrier barrier;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = *m_device->queueFamilyIndices().graphicsFamily;
  barrier.dstQueueFamilyIndex = *m_device->queueFamilyIndices().graphicsFamily;
  barrier.image = image();

  if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (hasStencilComponent(format))
      barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  } else {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = m_mipped ? mipLevels(m_extent) : 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vk::PipelineStageFlags srcStage;
  vk::PipelineStageFlags dstStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    srcStage = vk::PipelineStageFlagBits::eTransfer;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
  } else {
    throw runtime_error("Unsupported layout transition");
  }

  commandBuf.buffer().pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
}

Image::~Image()
{
  if (*m_handle != vk::Image{}) log::dbg("Destroying image");
  if (*m_view != vk::ImageView{}) log::dbg("Destroying image view");
}
