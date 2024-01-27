#include <seng/application_config.hpp>
#include <seng/hashes.hpp>
#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/debug_messenger.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/rendering/swapchain.hpp>
#include <seng/utils.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <string.h>   // for strcmp
#include <algorithm>  // for all_of, any_of
#include <array>      // for array
#include <cstdint>    // for uint32_t
#include <exception>  // for exception
#include <iterator>
#include <optional>   // for optional
#include <stdexcept>  // for runtime_error
#include <string>     // for basic_string, allocator
#include <utility>    // for pair, addressof
#include <vector>     // for vector

using namespace seng::rendering;
using namespace seng::internal;
using namespace std;

// Intializer functions
static vk::raii::Instance createInstance(const vk::raii::Context &, const GlfwWindow &);
static bool supportsAllLayers(const vector<const char *> &);

// Definitions for RenderTarget
Renderer::RenderTarget::RenderTarget(const Device &device,
                                     const vk::ImageView swapchainImage,
                                     vk::Extent2D extent,
                                     const RenderPass &pass) :
    swapchainImage(swapchainImage),
    depthBuffer(createDepthBuffer(device, extent)),
    framebuffer(device, pass, extent, {swapchainImage, **depthBuffer.imageView()})
{
  log::dbg("Allocated render target");
}

Image Renderer::RenderTarget::createDepthBuffer(const Device &device, vk::Extent2D extent)
{
  Image::CreateInfo info{vk::ImageType::e2D,
                         extent,
                         device.depthFormat().format,
                         vk::ImageTiling::eOptimal,
                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         vk::ImageAspectFlagBits::eDepth,
                         true};
  return Image(device, info);
}

// Definitions for Frame
Renderer::Frame::Frame(const Device &device, const vk::raii::CommandPool &pool) :
    commandBuffer(device, pool, true),
    imageAvailableSem(device.logical(), vk::SemaphoreCreateInfo{}),
    queueCompleteSem(device.logical(), vk::SemaphoreCreateInfo{}),
    inFlightFence(device, true),
    descriptorSets(),
    imageIndex(-1)
{
  log::dbg("Allocated resources for a frame");
}

// Definitions for FrameHandle
bool FrameHandle::invalid(size_t maxValue) const
{
  return m_index == -1 || m_index >= static_cast<ssize_t>(maxValue);
}

void FrameHandle::invalidate()
{
  m_index = -1;
}

// Defintions for renderer
static constexpr vk::CommandPoolCreateFlags cmdPoolFlags =
    vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

static constexpr vk::DescriptorPoolCreateFlags descriptorPoolFlags =
    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

const std::vector<const char *> Renderer::VALIDATION_LAYERS{
    "VK_LAYER_KHRONOS_validation"};

// TODO: add other sizes
const std::array<vk::DescriptorPoolSize, 1> Renderer::POOL_SIZES = {{
    {vk::DescriptorType::eUniformBuffer, 1024},
}};

Renderer::Renderer([[maybe_unused]] ApplicationConfig config, const GlfwWindow &window) :
    m_window(std::addressof(window)),
    m_context(),
    // Instance creation
    m_instance(createInstance(m_context, window)),
    m_dbgMessenger(m_instance, USE_VALIDATION),
    m_surface(window.createVulkanSurface(m_instance)),

    // Device and swapchain
    m_device(m_instance, m_surface),
    m_swapchain(m_device, m_surface, window),

    // Renderpass
    m_attachments{
        // Color attachment
        {{m_swapchain.format().format, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
          vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR,
          vk::ImageLayout::eColorAttachmentOptimal,
          vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f}},
         // Depth attachment
         {m_device.depthFormat().format, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
          vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
          vk::ImageLayout::eDepthStencilAttachmentOptimal,
          vk::ClearDepthStencilValue{1.0f, 0}}}},
    m_renderPass(m_device, m_attachments),

    // Pools
    m_commandPool(m_device.logical(),
                  {cmdPoolFlags, *m_device.queueFamilyIndices().graphicsFamily}),
    m_descriptorPool(std::invoke([&]() {
      vk::DescriptorPoolCreateInfo info{};
      info.flags = descriptorPoolFlags;
      info.maxSets = 1024;  // FIXME: not really sure about this
      info.setPoolSizes(Renderer::POOL_SIZES);
      return vk::raii::DescriptorPool(m_device.logical(), info);
    })),

    m_samplerLayout(nullptr),
    m_gubo(*this)
{
  log::dbg("Allocating render targets");
  m_targets.reserve(m_swapchain.images().size());
  for (auto &img : m_swapchain.images())
    m_targets.emplace_back(m_device, *img, m_swapchain.extent(), m_renderPass);

  log::dbg("Allocating render frames");
  m_frames =
      many<Renderer::Frame>(m_swapchain.MAX_FRAMES_IN_FLIGHT, m_device, m_commandPool);

  // Sampler layout info
  log::dbg("Allocating sampler set layout");
  vk::DescriptorSetLayoutBinding samplerBinding{0,
                                                vk::DescriptorType::eCombinedImageSampler,
                                                1, vk::ShaderStageFlagBits::eFragment};
  vk::DescriptorSetLayoutCreateInfo samplerInfo;
  samplerInfo.setBindings(samplerBinding);
  m_samplerLayout = vk::raii::DescriptorSetLayout(m_device.logical(), samplerInfo);

  // Registering the descriptor sets for the GUBO
  log::dbg("Allocating sets for GUBO");
  requestDescriptorSet(*m_gubo.layout(), m_gubo.bufferInfos(), {});

  log::dbg("Vulkan context is up and running!");
}

vk::raii::Instance createInstance(const vk::raii::Context &context,
                                  const GlfwWindow &window)
{
  auto &LAYERS = Renderer::VALIDATION_LAYERS;
  auto &VALIDATE = Renderer::USE_VALIDATION;

  vk::ApplicationInfo ai{};
  ai.pApplicationName = window.appName().c_str();
  ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.pEngineName = "seng";
  ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  ai.apiVersion = VK_API_VERSION_1_0;

  if (VALIDATE && !supportsAllLayers(LAYERS))
    throw runtime_error("Validation layers requested, but not available");

  vector<const char *> extensions{};
  vector<const char *> windowExtensions{window.extensions()};

  extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.insert(extensions.end(), make_move_iterator(windowExtensions.begin()),
                    make_move_iterator(windowExtensions.end()));
  if (VALIDATE)
    // extension always present if validation layers are present
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  vk::InstanceCreateInfo ci;
  ci.pApplicationInfo = &ai;
  ci.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ci.ppEnabledExtensionNames = extensions.data();
  if (VALIDATE) {
    ci.setPEnabledLayerNames(LAYERS);
    vk::DebugUtilsMessengerCreateInfoEXT dbg{DebugMessenger::createInfo()};
    ci.pNext = &dbg;
  } else {
    ci.enabledLayerCount = 0;
    ci.pNext = nullptr;
  }

  return vk::raii::Instance(context, ci);
}

bool supportsAllLayers(const vector<const char *> &l)
{
  const vector<vk::LayerProperties> a = vk::enumerateInstanceLayerProperties();
  return all_of(l.begin(), l.end(), [&](const char *name) {
    return any_of(a.begin(), a.end(), [&](const vk::LayerProperties &property) {
      return strcmp(property.layerName, name) == 0;
    });
  });
}

void Renderer::signalResize()
{
  m_fbGeneration++;
}

void Renderer::requestDescriptorSet(
    vk::DescriptorSetLayout layout,
    const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
    const std::vector<vk::DescriptorImageInfo> &imageInfo)
{
  size_t hash{0};
  internal::hashCombine(hash, layout, bufferInfo, imageInfo);

  for (auto &f : m_frames) {
    auto iter = f.descriptorSets.find(hash);

    // If a descriptor set for the given layout has already been allocated,
    // skip the frame (should never happen)
    if (iter != f.descriptorSets.end()) continue;

    array<vk::DescriptorSetLayout, 1> descs = {layout};

    vk::DescriptorSetAllocateInfo info{};
    info.descriptorPool = *m_descriptorPool;
    info.setSetLayouts(descs);
    vk::raii::DescriptorSets sets(m_device.logical(), info);
    f.descriptorSets.emplace(hash, std::move(sets[0]));
  }
}

void Renderer::clearDescriptorSet(vk::DescriptorSetLayout layout,
                                  const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
                                  const std::vector<vk::DescriptorImageInfo> &imageInfo)
{
  size_t hash{0};
  internal::hashCombine(hash, layout, bufferInfo, imageInfo);
  for (auto &f : m_frames) f.descriptorSets.erase(hash);
}

void Renderer::clearDescriptorSets()
{
  for (auto &f : m_frames) f.descriptorSets.clear();
  m_descriptorPool.reset();
}

optional<FrameHandle> Renderer::beginFrame()
{
  if (m_recreatingSwap) {
    m_device.logical().waitIdle();
    seng::log::dbg("Already recreating swapchain, waiting...");
    return nullopt;
  }

  if (m_lastFbGeneration != m_fbGeneration) {
    m_device.logical().waitIdle();
    recreateSwapchain();
    seng::log::dbg("Framebuffer changed, aborting...");
    return nullopt;
  }

  auto &frame = m_frames[m_currentFrame];
  try {
    frame.inFlightFence.wait();
    frame.imageIndex = m_swapchain.nextImageIndex(frame.imageAvailableSem);

    Framebuffer &curFb = m_targets[frame.imageIndex].framebuffer;
    CommandBuffer &curBuf = frame.commandBuffer;
    curBuf.reset();
    curBuf.begin();

    // Begin the render pass
    m_renderPass.begin(curBuf, curFb, m_swapchain.extent(), {0, 0});

    // Dynamic states
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain.extent().width);
    viewport.height = static_cast<float>(m_swapchain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    curBuf.buffer().setViewport(0, viewport);

    vk::Rect2D scissor{{0, 0}, m_swapchain.extent()};
    curBuf.buffer().setScissor(0, scissor);

    return optional(FrameHandle{m_currentFrame});
  } catch (const exception &e) {
    log::warning("Caught exception: {}", e.what());
    return nullopt;
  }
}

const vk::raii::DescriptorSet &Renderer::getDescriptorSet(
    const FrameHandle &handle,
    vk::DescriptorSetLayout layout,
    const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
    const std::vector<vk::DescriptorImageInfo> &imageInfo) const
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");
  size_t hash{0};
  internal::hashCombine(hash, layout, bufferInfo, imageInfo);
  return m_frames[handle.m_index].descriptorSets.at(hash);
}

const CommandBuffer &Renderer::getCommandBuffer(const FrameHandle &handle) const
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");
  return m_frames[handle.m_index].commandBuffer;
}

void Renderer::endFrame(FrameHandle &handle)
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");

  auto &frame = m_frames[handle.m_index];

  m_renderPass.end(frame.commandBuffer);
  frame.commandBuffer.end();

  // Reset the fence for use on the next frame
  frame.inFlightFence.reset();

  // Start submitting to queue
  vk::SubmitInfo submitInfo{};
  std::array<vk::CommandBuffer, 1> commandBuffers = {*frame.commandBuffer.buffer()};
  std::array<vk::Semaphore, 1> queueCompleteSems = {*frame.queueCompleteSem};
  std::array<vk::Semaphore, 1> imageAvailableSems = {*frame.imageAvailableSem};

  submitInfo.setCommandBuffers(commandBuffers);
  // The semaphore(s) to be signaled when the queue is complete.
  submitInfo.setSignalSemaphores(queueCompleteSems);
  // Wait semaphore ensures that the operation cannot begin until the image is available
  submitInfo.setWaitSemaphores(imageAvailableSems);

  // Each semaphore waits on the corresponding pipeline stage to complete.
  // 1:1 ratio. VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents
  // subsequent colour attachment writes from executing until the semaphore
  // signals (i.e. one frame is presented at a time)
  array<vk::PipelineStageFlags, 1> flags = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.setWaitDstStageMask(flags);

  m_device.graphicsQueue().submit(submitInfo, *frame.inFlightFence.handle());

  try {
    m_swapchain.present(m_device.presentQueue(), m_device.graphicsQueue(),
                        frame.queueCompleteSem, frame.imageIndex);
  } catch (const InadequateSwapchainException &e) {
    log::dbg("Error while presenting: swapchain out of date. Recreating...");
    recreateSwapchain();
  }

  frame.imageIndex = -1;  // Forget the image
  handle.invalidate();

  // Advance cyclical iterator
  m_currentFrame = (m_currentFrame + 1) % m_swapchain.MAX_FRAMES_IN_FLIGHT;
}

bool Renderer::scopedFrame(std::function<void(const FrameHandle &)> func)
{
  std::optional<FrameHandle> h = beginFrame();
  if (!h)
    return false;
  else {
    try {
      func(*h);
      endFrame(*h);
      return true;
    } catch (const exception &e) {
      endFrame(*h);  // always end the frame, so that our application doesn't stall
      throw;         // then rethrow whatever we caught
    }
  }
}

void Renderer::recreateSwapchain()
{
  // If already recreating, do nothing.
  if (m_recreatingSwap) return;

  // Get the new framebuffer size, if null do nothing
  pair<unsigned int, unsigned int> fbSize = m_window->framebufferSize();
  if (fbSize.first == 0 || fbSize.second == 0) {
    log::dbg("Null framebuffer, aborting...");
    return;
  }

  // Start the creation process
  m_recreatingSwap = true;
  log::dbg("Started swapchain recreation");

  // Wait for any pending work to finish and flush out temporary data
  m_device.logical().waitIdle();

  // Requery swapchain support data (it might have changed)
  m_device.requerySupport();
  m_device.requeryDepthFormat();

  // Recreate the swapchain and clear the currentFrame counter
  m_swapchain = Swapchain(m_device, m_surface, *m_window, m_swapchain.swapchain());
  m_currentFrame = 0;

  // Sync framebuffer generation
  m_lastFbGeneration = m_fbGeneration;

  // Recreate render targets
  m_targets.clear();
  for (auto &img : m_swapchain.images())
    m_targets.emplace_back(m_device, *img, m_swapchain.extent(), m_renderPass);

  // Finish the recreation process
  m_recreatingSwap = false;
  log::dbg("Finished swapchain recreation");
}

Renderer::~Renderer()
{
  // Just checking if the instance handle is valid is enough
  // since all objects are valid or none are.
  if (*m_instance != vk::Instance{}) {
    log::dbg("Destroying vulkan context");
    m_device.logical().waitIdle();
    clearDescriptorSets();
  }
}
