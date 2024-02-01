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
    m_extent(info.extent),
    m_mipLevels(info.mipped ? ::mipLevels(m_extent) : 1),
    // Create image handle
    m_handle(std::invoke([&]() {
      vk::ImageCreateInfo ci{};
      ci.imageType = info.type;
      ci.extent = info.extent;
      ci.mipLevels = m_mipLevels;
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
    m_unmanaged(nullptr),
    m_view(nullptr)
{
  log::dbg("Created new image");
  if (info.createView) this->createView(info.viewType, info.format, info.aspectFlags);
}

Image::Image(const Device &dev, vk::Image wrapped, uint32_t mipLevels) :
    m_device(std::addressof(dev)),
    m_extent{0, 0, 0},
    m_mipLevels(mipLevels),
    m_handle(nullptr),
    m_memory(nullptr),
    m_unmanaged(wrapped),
    m_view(nullptr)
{
}

Image::Image(std::nullptr_t) :
    m_device(nullptr),
    m_extent{0, 0, 0},
    m_mipLevels(0),
    m_handle(nullptr),
    m_memory(nullptr),
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
  ci.subresourceRange.levelCount = m_mipLevels;
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
  barrier.subresourceRange.levelCount = m_mipLevels;
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

void Image::generateMipMapsBeforeShader(const CommandBuffer &cmd, vk::Format format) const
{
  BAIL_OUT_ON_UNINITIALIZED();
  if (m_mipLevels <= 1) {
    seng::log::warning(
        "Trying to generate mipmaps for an image that doesn't have them, aborting...");
    return;
  }
  vk::FormatProperties props = m_device->physical().getFormatProperties(format);
  if (!(props.optimalTilingFeatures &
        vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    throw std::runtime_error("Image format does not support linear blitting");
  }

  vk::ImageMemoryBarrier barrier;
  barrier.image = image();
  barrier.srcQueueFamilyIndex = *m_device->queueFamilyIndices().graphicsFamily;
  barrier.dstQueueFamilyIndex = *m_device->queueFamilyIndices().graphicsFamily;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = m_extent.width;
  int32_t mipHeight = m_extent.height;
  uint32_t mips = m_mipLevels;

  for (uint32_t i = 1; i < mips; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    cmd.buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                 barrier);

    vk::ImageBlit blit;
    blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1,
                                      mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    cmd.buffer().blitImage(image(), vk::ImageLayout::eTransferSrcOptimal, image(),
                           vk::ImageLayout::eTransferDstOptimal, blit,
                           vk::Filter::eLinear);

    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    cmd.buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                                 barrier);

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mips - 1;
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
  cmd.buffer().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                               barrier);
}

Image::~Image()
{
  if (*m_handle != vk::Image{}) log::dbg("Destroying image");
  if (*m_view != vk::ImageView{}) log::dbg("Destroying image view");
}
