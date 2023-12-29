#include <seng/log.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/image.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

using namespace std;
using namespace seng::rendering;

static vk::raii::ImageView createView(const vk::raii::Device &device,
                                      const vk::raii::Image &image,
                                      vk::Format fmt,
                                      vk::ImageAspectFlags aspect)
{
  vk::ImageViewCreateInfo ci{};
  ci.image = *image;
  ci.viewType = vk::ImageViewType::e2D;
  ci.format = fmt;
  ci.subresourceRange.aspectMask = aspect;
  ci.subresourceRange.baseMipLevel = 0;
  ci.subresourceRange.levelCount = 1;
  ci.subresourceRange.baseArrayLayer = 0;
  ci.subresourceRange.layerCount = 1;
  return vk::raii::ImageView(device, ci);
}

Image::Image(const Device &dev, const Image::CreateInfo &info) :
    m_info(info),
    m_device(std::addressof(dev)),
    // Create image handle
    m_handle(std::invoke([&]() {
      vk::ImageCreateInfo ci{};
      ci.imageType = vk::ImageType::e2D;
      ci.extent = vk::Extent3D{info.extent, 1};
      ci.mipLevels = 4;
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
    // Create the view if told to do so
    m_view()
{
  if (info.createView)
    m_view = ::createView(dev.logical(), m_handle, info.format, info.aspectFlags);
  log::dbg("Created new image {}", info.createView ? "with view" : "without view");
}

void Image::createView()
{
  if (m_view.has_value()) return;
  m_view = ::createView(m_device->logical(), m_handle, m_info.format, m_info.aspectFlags);
}

Image::~Image()
{
  // Just checking if the device handle is valid is enough
  // since all or none handles are valid
  if (*m_handle != vk::Image{}) log::dbg("Destroying image");
}
