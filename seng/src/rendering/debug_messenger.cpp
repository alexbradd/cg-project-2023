#include <seng/log.hpp>
#include <seng/rendering/debug_messenger.hpp>

#include <memory>
#include <string>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

DebugMessenger::DebugMessenger(Instance &instance, bool allocate)
    : debugMessenger() {
  if (allocate)
    debugMessenger =
        make_unique<DebugUtilsMessengerEXT>(instance, createInfo());
}

vk::DebugUtilsMessengerCreateInfoEXT DebugMessenger::createInfo() {
  using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;

  vk::DebugUtilsMessengerCreateInfoEXT ci{};
  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
      Severity::eVerbose | Severity::eWarning | Severity::eError);
  vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      Type::eGeneral | Type::ePerformance | Type::eValidation);
  ci.messageSeverity = severityFlags;
  ci.messageType = messageTypeFlags;
  ci.pfnUserCallback = debugCallback;
  return ci;
}

VkBool32 DebugMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *) {
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      log::info("Validation layer: {}", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      log::warning("Validation layer: {}", pCallbackData->pMessage);
      break;
    default:
      log::error("Validation layer: {}", pCallbackData->pMessage);
  }
  return VK_FALSE;
}
