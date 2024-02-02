#include <seng/log.hpp>
#include <seng/rendering/debug_messenger.hpp>

#include <string>

using namespace std;
using namespace seng::rendering;
using namespace vk::raii;

using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
using CType = VkDebugUtilsMessageTypeFlagBitsEXT;

const vk::DebugUtilsMessengerCreateInfoEXT DebugMessenger::info{
    {},
    Severity::eVerbose | Severity::eWarning | Severity::eError,
    Type::eGeneral | Type::ePerformance | Type::eValidation,
    DebugMessenger::debugCallback};

DebugMessenger::DebugMessenger(Instance &instance) : m_debugMessenger(instance, info) {}

VkBool32 DebugMessenger::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                       VkDebugUtilsMessageTypeFlagsEXT types,
                                       const VkDebugUtilsMessengerCallbackDataEXT *cbData,
                                       [[maybe_unused]] void *userData)
{
  switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      if (types & (CType::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   CType::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        seng::log::dbg("Validation later: {}", cbData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      log::info("Validation layer: {}", cbData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      log::warning("Validation layer: {}", cbData->pMessage);
      break;
    default:
      log::error("Validation layer: {}", cbData->pMessage);
  }
  return VK_FALSE;
}
