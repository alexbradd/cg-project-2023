#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/image.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
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

Image::~Image()
{
  if (*m_handle != vk::Image{}) log::dbg("Destroying image");
  if (*m_view != vk::ImageView{}) log::dbg("Destroying image view");
}
