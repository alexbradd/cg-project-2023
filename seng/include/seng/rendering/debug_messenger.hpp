#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

/**
 * Class that holds a reference the DebugUtilMessengerEXT object. Intantiating
 * it will allocate a DebugUtilMessengerEXT, while destroying it will destroy
 * the underlying structure.
 */
class DebugMessenger {
 public:
  DebugMessenger(vk::raii::Instance &instance);

  static const vk::DebugUtilsMessengerCreateInfoEXT info;

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);

 private:
  vk::raii::DebugUtilsMessengerEXT m_debugMessenger;
};

}  // namespace seng::rendering
