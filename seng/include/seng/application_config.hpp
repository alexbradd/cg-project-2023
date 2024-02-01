#pragma once

#include <string>

namespace seng {

/**
 * Configuration of various application parameters. Most values have been given a
 * generic default. Documentation is provided for each of the fields.
 */
struct ApplicationConfig {
  // ====
  // General
  // ====

  /// Name to be used by the application window
  std::string appName = "Vulkan";

  /// File containing the shader definition
  std::string shaderDefinitions = "./shaders/shaders.yml";

  /// Directory where the engine will look for COMPILED shaders
  std::string shaderPath = "./shaders/";

  /// Directory where the engine will look for generic assets (i.e. meshes/textures)
  std::string assetPath = "./assets/";

  /// Directory where the engine will look for scene YAML definition files
  std::string scenePath = "./scenes/";

  // ====
  // Graphics
  // ====

  /// Require anisotropy at device initialization.
  bool useAnisotropy = true;

  /// Anisotropy level used in texutre sampling. Used only if anisotropy has been
  /// requested.
  float anisotropyLevel = 8.0f;

  /// Enable/disable creation of mipmaps
  bool useMipMaps = true;

  /// Red component of the color used to clear frames
  float clearColorRed = 0.0f;
  /// Blue component of the color used to clear frames
  float clearColorBlue = 0.0f;
  /// Green component of the color used to clear frames
  float clearColorGreen = 0.0f;
};

}  // namespace seng
