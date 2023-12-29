#pragma once

#include <memory>
#include <vulkan/vulkan_raii.hpp>

namespace seng::rendering {

/**
 * Class that holds a reference the DebugUtilMessengerEXT object. Intantiating
 * it will allocate a DebugUtilMessengerEXT, while destroying it will destroy
 * the underlying structure.
 */
class DebugMessenger {
 public:
  DebugMessenger(vk::raii::Instance &instance, bool allocate = true);

  static vk::DebugUtilsMessengerCreateInfoEXT createInfo();

 private:
  std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> m_debugMessenger;

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
};

}  // namespace seng::rendering
