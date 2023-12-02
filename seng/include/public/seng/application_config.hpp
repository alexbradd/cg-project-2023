#pragma once

#include <string>

namespace seng {

/**
 * Wrapper struct containing useful configuration data
 */
struct ApplicationConfig {
  std::string appName = "Vulkan";
  std::string shaderPath = "./shaders/";
  std::string assetPath = "./assets/";
};

}  // namespace seng
