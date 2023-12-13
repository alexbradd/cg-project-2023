#include <optional>
#include <seng/log.hpp>
#include <seng/vulkan_device.hpp>
#include <seng/vulkan_image.hpp>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

VulkanImage::VulkanImage(VulkanDevice &dev, VulkanImage::CreateInfo &info) :
    info(info),
    vkDevRef(dev),
    width(info.width),
    height(info.height),
    // Create image handle
    handle(std::invoke([&]() {
      vk::ImageCreateInfo ci{};
      ci.imageType = vk::ImageType::e2D;
      ci.extent = vk::Extent3D{info.width, info.height, 1};
      ci.mipLevels = 4;
      ci.arrayLayers = 1;
      ci.format = info.format;
      ci.tiling = info.tiling;
      ci.initialLayout = vk::ImageLayout::eUndefined;
      ci.usage = info.usage;
      ci.samples = vk::SampleCountFlagBits::e1;
      ci.sharingMode = vk::SharingMode::eExclusive;
      return Image(vkDevRef.get().logical(), ci);
    })),
    // Allocate memory
    memory(std::invoke([&]() {
      vk::MemoryRequirements requirements(handle.getMemoryRequirements());
      uint32_t memoryIndex =
          vkDevRef.get().findMemoryIndex(requirements.memoryTypeBits, info.memoryFlags);

      vk::MemoryAllocateInfo ai;
      ai.allocationSize = requirements.size;
      ai.memoryTypeIndex = memoryIndex;

      DeviceMemory ret(vkDevRef.get().logical(), ai);
      handle.bindMemory(*ret, 0);

      return ret;
    })),
    // Create the view if told to do so
    view(std::invoke([&]() -> optional<ImageView> {
      if (!info.createView) return nullopt;
      vk::ImageViewCreateInfo ci{};
      ci.image = *handle;
      ci.viewType = vk::ImageViewType::e2D;
      ci.format = info.format;
      ci.subresourceRange.aspectMask = info.aspectFlags;
      ci.subresourceRange.baseMipLevel = 0;
      ci.subresourceRange.levelCount = 1;
      ci.subresourceRange.baseArrayLayer = 0;
      ci.subresourceRange.layerCount = 1;
      return ImageView(dev.logical(), ci);
    }))
{
  log::dbg("Created new image {}", info.createView ? "with view" : "without view");
}

void VulkanImage::createView()
{
  if (view.has_value()) return;

  vk::ImageViewCreateInfo ci;
  ci.image = *handle;
  ci.viewType = vk::ImageViewType::e2D;
  ci.subresourceRange.aspectMask = info.aspectFlags;
  ci.subresourceRange.baseMipLevel = 0;
  ci.subresourceRange.levelCount = 1;
  ci.subresourceRange.baseArrayLayer = 0;
  ci.subresourceRange.layerCount = 1;
  view = ImageView(vkDevRef.get().logical(), ci);
}

VulkanImage::~VulkanImage()
{
  // Just checking if the device handle is valid is enough
  // since all or none handles are valid
  if (*handle != vk::Image{}) log::dbg("Destroying image");
}
