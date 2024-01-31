#pragma once

#include <seng/rendering/image.hpp>

#include <glm/detail/type_vec4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstddef>
#include <string>

namespace seng {

namespace rendering {
class Renderer;
}

/// Dimensions of a Texture
enum class TextureType { e1D, e2D };

struct SamplerOptions {
  /// Texture filtering mor minimization/magnification
  vk::Filter filtering = vk::Filter::eLinear;

  /// Texel addressing mode
  vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;

  /// Flag to enable/disable anisotropy for the sampler
  bool useAnisotropy = false;

  /// Anisotropy level to use. Ignored if Anisotropy has been disabled.
  float anisotropyLevel = 1.0f;

  /// Return the "optimal" parameters based on the parameters of the given
  /// Renderer.
  static SamplerOptions optimal(const rendering::Renderer &renderer);
};

/**
 * A series of pixel data stored on the device and sample-able by shaders. Can
 * be 1 dimensional or two dimensional (see TextureType).
 */
class Texture {
 public:
  /// Create a single-pixel texture of the given type with the given color (in
  /// R8G8B8A8 format). By default it is a bright magenta.
  Texture(rendering::Renderer &renderer,
          TextureType type,
          glm::vec<4, unsigned char> color = {255, 0, 255, 255});
  Texture(const Texture &) = delete;
  Texture(Texture &&) = default;

  Texture &operator=(const Texture &) = delete;
  Texture &operator=(Texture &&) = default;

  // Accessors
  TextureType type() const { return m_type; }
  std::pair<unsigned int, unsigned int> size() const { return {m_width, m_height}; }
  const rendering::Image &image() const { return m_image; }
  const vk::Sampler sampler() const { return m_sampler; }

  /**
   * Factory method that creates a Texture by loading the image with the given name.
   *
   * Images are searched inside the asset path (defined in ApplicationConfig) and
   * filename construction is done like this: `${assetPath}/${name}`
   */
  static Texture loadFromDisk(rendering::Renderer &renderer,
                              TextureType typ,
                              SamplerOptions opts,
                              const std::string &assetPath,
                              const std::string &name);

 private:
  TextureType m_type;
  unsigned int m_width, m_height;
  rendering::Image m_image;
  vk::Sampler m_sampler;

  /// Creates an empty object. To be filled by an appropriate call to `fill()`
  Texture();

  /// Fills in the objct with the data given, allocating all that is necessary
  static void fill(Texture &tex,
                   rendering::Renderer &renderer,
                   TextureType typ,
                   SamplerOptions opts,
                   void *pixelData,
                   vk::DeviceSize size,
                   unsigned int width,
                   unsigned int height);
};

};  // namespace seng

namespace std {
template <>
struct hash<seng::TextureType> {
  std::size_t operator()(const seng::TextureType &t) const
  {
    return static_cast<std::size_t>(t);
  }
};
}  // namespace std
