#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/command_buffer.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/resources/texture.hpp>

#include <stb_image.h>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <stdexcept>

using namespace seng;
namespace fs = std::filesystem;

static vk::BufferUsageFlags STAGING_BUFFER_USAGE = vk::BufferUsageFlagBits::eTransferSrc;
static vk::MemoryPropertyFlags STAGING_BUFFER_MEM =
    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

SamplerOptions SamplerOptions::optimal(const rendering::Renderer &renderer)
{
  SamplerOptions opts;
  opts.useAnisotropy = renderer.useAnisotropy();
  opts.anisotropyLevel = renderer.anisotropyLevel();
  return opts;
}

Texture::Texture() :
    m_type{TextureType::e1D},
    m_width(0),
    m_height(0),
    m_image(nullptr),
    m_sampler(nullptr)
{
}

void Texture::fill(Texture &tex,
                   rendering::Renderer &renderer,
                   TextureType typ,
                   SamplerOptions opts,
                   void *pixelData,
                   vk::DeviceSize size,
                   unsigned int width,
                   unsigned int height)
{
  tex.m_type = typ;

  tex.m_width = width;
  tex.m_height = height;
  if (typ == TextureType::e1D && height != 1)
    throw std::runtime_error("2D image loaded as 1D");

  rendering::Image::CreateInfo imgInfo;
  switch (typ) {
    case seng::TextureType::e1D:
      imgInfo.type = vk::ImageType::e1D;
      imgInfo.viewType = vk::ImageViewType::e1D;
      break;
    case seng::TextureType::e2D:
      imgInfo.type = vk::ImageType::e2D;
      imgInfo.viewType = vk::ImageViewType::e2D;
      break;
  }
  imgInfo.extent = vk::Extent3D(tex.m_width, tex.m_height, 1);
  imgInfo.format = vk::Format::eR8G8B8A8Srgb;
  imgInfo.tiling = vk::ImageTiling::eOptimal;
  imgInfo.usage = vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
  imgInfo.memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
  imgInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
  imgInfo.mipped = true;
  imgInfo.createView =
      false;  // We create the view later manually, but the info is still useful
  tex.m_image = rendering::Image(renderer.device(), imgInfo);

  seng::log::dbg("Uploading pixel data to device");
  rendering::Buffer staging(renderer.device(), STAGING_BUFFER_USAGE, size,
                            STAGING_BUFFER_MEM, true);
  staging.load(pixelData, 0, size, {});
  rendering::CommandBuffer::recordSingleUse(
      renderer.device(), renderer.commandPool(), renderer.device().graphicsQueue(),
      [&](auto &cmd) {
        tex.image().transitionLayout(cmd, imgInfo.format, vk::ImageLayout::eUndefined,
                                     vk::ImageLayout::eTransferDstOptimal);
        tex.image().copyFromBuffer(cmd, staging);
        if (renderer.useMipMaps()) {
          tex.image().generateMipMapsBeforeShader(cmd, imgInfo.format);
        } else {
          tex.image().transitionLayout(cmd, imgInfo.format,
                                       vk::ImageLayout::eTransferDstOptimal,
                                       vk::ImageLayout::eShaderReadOnlyOptimal);
        }
      });
  tex.m_image.createView(imgInfo.viewType, imgInfo.format, imgInfo.aspectFlags);

  vk::SamplerCreateInfo samplerInfo;
  samplerInfo.magFilter = opts.filtering;
  samplerInfo.minFilter = opts.filtering;
  samplerInfo.addressModeU = opts.addressMode;
  samplerInfo.addressModeV = opts.addressMode;
  samplerInfo.addressModeW = opts.addressMode;
  samplerInfo.anisotropyEnable = opts.useAnisotropy;
  samplerInfo.maxAnisotropy = opts.anisotropyLevel;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = false;
  samplerInfo.compareEnable = false;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = tex.m_image.mipLevels();
  tex.m_sampler = renderer.requestSampler(samplerInfo);
}

Texture::Texture(rendering::Renderer &renderer,
                 TextureType type,
                 glm::vec<4, unsigned char> color) :
    seng::Texture()
{
  fill(*this, renderer, type, {}, &color, sizeof(color), 1, 1);
}

Texture Texture::loadFromDisk(rendering::Renderer &renderer,
                              TextureType typ,
                              SamplerOptions opts,
                              const std::string &assetPath,
                              const std::string &name)
{
  std::string texPath{fs::path{assetPath} / fs::path{name}};
  if (!fs::exists(texPath)) {
    seng::log::error("Could not locate {}, allocating fallback texture", name);
    return Texture(renderer, typ);
  }

  int texWidth, texHeight, texChannels;
  stbi_uc *pixels =
      stbi_load(texPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  if (!pixels) {
    seng::log::error("Could not load {}, allocating fallback texture", name);
    return Texture(renderer, typ);
  }
  seng::log::dbg("Loaded {} from disk", name);

  Texture ret;
  fill(ret, renderer, typ, opts, pixels, texWidth * texHeight * 4, texWidth, texHeight);

  stbi_image_free(pixels);
  return ret;
}
