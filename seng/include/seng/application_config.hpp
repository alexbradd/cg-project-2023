#pragma once

#include <string>

namespace seng {

/**
 * Wrapper struct containing useful configuration data
 */
struct ApplicationConfig {
  // Generic
  std::string appName = "Vulkan";
  std::string shaderPath = "./shaders/";
  std::string assetPath = "./assets/";
  std::string scenePath = "./scenes/";

  // Grahpics
  bool useAnisotropy = true;
  float anisotropyLevel = 8.0f;
};

}  // namespace seng
