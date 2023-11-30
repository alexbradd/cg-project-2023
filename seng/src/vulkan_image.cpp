#include <seng/vulkan_device.hpp>
#include <seng/vulkan_image.hpp>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

static Image createImage(VulkanDevice &device, VulkanImage::CreateInfo &info);
static DeviceMemory allocateImage(VulkanDevice &device,
                                  VulkanImage::CreateInfo &info,
                                  Image &img);
static optional<ImageView> maybeCreateView(VulkanDevice &dev,
                                           VulkanImage::CreateInfo &info,
                                           Image &img);

VulkanImage::VulkanImage(VulkanDevice &dev, VulkanImage::CreateInfo &info)
    : info(info),
      vkDevRef(dev),
      width(info.width),
      height(info.height),
      handle(createImage(vkDevRef, info)),
      memory(allocateImage(vkDevRef, info, handle)),
      view(maybeCreateView(vkDevRef, info, handle)) {}

Image createImage(VulkanDevice &device, VulkanImage::CreateInfo &info) {
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
  return Image(device.logical(), ci);
}

DeviceMemory allocateImage(VulkanDevice &device,
                           VulkanImage::CreateInfo &info,
                           Image &img) {
  vk::MemoryRequirements requirements(img.getMemoryRequirements());
  uint32_t memoryIndex =
      device.findMemoryIndex(requirements.memoryTypeBits, info.memoryFlags);

  vk::MemoryAllocateInfo ai;
  ai.allocationSize = requirements.size;
  ai.memoryTypeIndex = memoryIndex;

  DeviceMemory ret(device.logical(), ai);
  img.bindMemory(*ret, 0);

  return ret;
}

optional<ImageView> maybeCreateView(VulkanDevice &dev,
                                    VulkanImage::CreateInfo &info,
                                    Image &img) {
  if (!info.createView) return nullopt;
  vk::ImageViewCreateInfo ci{};
  ci.image = *img;
  ci.viewType = vk::ImageViewType::e2D;
  ci.format = info.format;
  ci.subresourceRange.aspectMask = info.aspectFlags;
  ci.subresourceRange.baseMipLevel = 0;
  ci.subresourceRange.levelCount = 1;
  ci.subresourceRange.baseArrayLayer = 0;
  ci.subresourceRange.layerCount = 1;
  return ImageView(dev.logical(), ci);
}

void VulkanImage::createView() {
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
