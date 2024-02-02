#include <limits>
#include <seng/application.hpp>
#include <seng/application_config.hpp>
#include <seng/hashes.hpp>
#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/debug_messenger.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/global_uniform.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/swapchain.hpp>
#include <seng/resources/mesh.hpp>
#include <seng/resources/texture.hpp>
#include <seng/utils.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <string.h>   // for strcmp
#include <algorithm>  // for all_of, any_of
#include <array>      // for array
#include <cstddef>
#include <cstdint>    // for uint32_t
#include <exception>  // for exception
#include <optional>   // for optional
#include <stdexcept>  // for runtime_error
#include <string>     // for basic_string, allocator
#include <utility>    // for pair, addressof
#include <vector>     // for vector

using namespace seng;
using namespace seng::rendering;
using namespace std;

// Intializer functions
static vk::raii::Instance createInstance(const vk::raii::Context &, const GlfwWindow &);
static bool supportsAllLayers(const vector<const char *> &);

// Definitions for Frame
Renderer::Frame::Frame(const Device &device, const vk::raii::CommandPool &pool) :
    m_commandBuffer(device, pool, true),
    m_imageAvailableSem(device.logical(), vk::SemaphoreCreateInfo{}),
    m_queueCompleteSem(device.logical(), vk::SemaphoreCreateInfo{}),
    m_inFlightFence(device.logical(),
                    vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}),
    m_descriptorCache(),
    m_index(-1)
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

size_t FrameHandle::asIndex() const
{
  if (m_index < 0) throw runtime_error("Converting invalid handle to index");
  return static_cast<size_t>(m_index);
}

// Defintions for renderer
static constexpr vk::CommandPoolCreateFlags cmdPoolFlags =
    vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

static constexpr vk::DescriptorPoolCreateFlags descriptorPoolFlags =
    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

const std::vector<const char *> Renderer::VALIDATION_LAYERS{
    "VK_LAYER_KHRONOS_validation"};

// TODO: add other sizes
const std::array<vk::DescriptorPoolSize, 2> Renderer::POOL_SIZES = {{
    {vk::DescriptorType::eUniformBuffer, 1024},
    {vk::DescriptorType::eCombinedImageSampler, 1024},
}};

Renderer::Renderer(Application &app, const GlfwWindow &window) :
    m_app(std::addressof(app)),
    m_window(std::addressof(window)),
    m_context(),
    // Instance creation
    m_instance(createInstance(m_context, window)),
    m_dbgMessenger(m_instance, USE_VALIDATION),
    m_surface(window.createVulkanSurface(m_instance)),

    // Device and swapchain
    m_device(app.config(), m_instance, m_surface),
    m_swapchain(m_device, m_surface, window),

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

    // Renderpass is intialized later
    m_renderPass(nullptr),

    // Other stuff
    m_fallbackMesh(*this),
    m_gubo(nullptr)
{
  log::dbg("Storing configuration options");
  m_useAnisotropy = app.config().useAnisotropy;
  m_useMips = app.config().useMipMaps;

  VkSampleCountFlags naked = 1 << static_cast<int>(floor(log2(app.config().samples)));
  vk::SampleCountFlags req = vk::SampleCountFlags{naked};
  req &= m_device.supportedSampleCounts();
  m_samples = vk::SampleCountFlagBits(static_cast<VkSampleCountFlags>(req));

  log::dbg("Basic resources acquires, setting up drawing resources");

  log::dbg("Creating render pass");
  createRenderPass();

  log::dbg("Allocating swapchain framebuffer");
  allocateSwapchainFramebuffers();

  log::dbg("Allocating render frames");
  m_frames = seng::internal::many<Renderer::Frame>(m_swapchain.MAX_FRAMES_IN_FLIGHT,
                                                   m_device, m_commandPool);

  log::dbg("Allocating GUBO");
  m_gubo = GlobalUniform(*this);

  log::dbg("Reading shaders");
  m_shaders.fromSchema(*this, app.config().shaderDefinitions, m_app->config().shaderPath);

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

void Renderer::createRenderPass()
{
  std::vector<Attachment> attachments;
  attachments.reserve(3);

  Attachment depthAttachment;
  depthAttachment.format = m_device.depthFormat().format;
  depthAttachment.samples = samples();
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  depthAttachment.usage = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  depthAttachment.clearValue = vk::ClearDepthStencilValue{1.0f, 0};
  attachments.push_back(depthAttachment);

  Attachment colorAttachment;
  colorAttachment.format = m_swapchain.format().format;
  colorAttachment.samples = samples();
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  colorAttachment.usage = vk::ImageLayout::eColorAttachmentOptimal;
  colorAttachment.clearValue =
      vk::ClearColorValue{m_app->config().clearColorRed, m_app->config().clearColorGreen,
                          m_app->config().clearColorBlue, 1.0f};
  colorAttachment.resolve = false;
  if (m_samples > vk::SampleCountFlagBits::e1) {
    seng::log::dbg("Multisampled renderpass requested");

    Attachment colorResolveAttachment = colorAttachment;
    colorResolveAttachment.samples = vk::SampleCountFlagBits::e1;
    colorResolveAttachment.resolve = true;

    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    attachments.push_back(colorAttachment);
    attachments.push_back(colorResolveAttachment);
  } else {
    attachments.push_back(colorAttachment);
  }
  m_renderPass = RenderPass(m_device, attachments);
}

void Renderer::allocateSwapchainFramebuffers()
{
  // Drop old resources
  m_swapchainFbs.clear();
  m_fbImages.clear();

  // Framebuffer info
  vk::FramebufferCreateInfo fbInfo{};
  fbInfo.renderPass = *m_renderPass.handle();
  fbInfo.width = m_swapchain.extent().width;
  fbInfo.height = m_swapchain.extent().height;
  fbInfo.layers = 1;

  // Common Image info
  Image::CreateInfo imageInfo;
  imageInfo.type = vk::ImageType::e2D;
  imageInfo.extent = vk::Extent3D{m_swapchain.extent(), 1};
  imageInfo.samples = samples();
  imageInfo.tiling = vk::ImageTiling::eOptimal;
  imageInfo.memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
  imageInfo.viewType = vk::ImageViewType::e2D;
  imageInfo.mipped = false;
  imageInfo.createView = true;

  for (auto &swapImage : m_swapchain.images()) {
    imageInfo.format = m_device.depthFormat().format;
    imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageInfo.aspectFlags = vk::ImageAspectFlagBits::eDepth;
    auto depthView = m_fbImages.emplace_back(m_device, imageInfo).imageView();

    std::vector<vk::ImageView> attachments;
    attachments.reserve(3);
    attachments.push_back(depthView);

    if (samples() > vk::SampleCountFlagBits::e1) {
      imageInfo.format = m_swapchain.format().format;
      imageInfo.usage = vk::ImageUsageFlagBits::eTransientAttachment |
                        vk::ImageUsageFlagBits::eColorAttachment;
      imageInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
      auto colorView = m_fbImages.emplace_back(m_device, imageInfo).imageView();

      attachments.push_back(colorView);
    }

    attachments.push_back(swapImage.imageView());

    fbInfo.setAttachments(attachments);
    m_swapchainFbs.emplace_back(m_device.logical(), fbInfo);
  }
}

float Renderer::anisotropyLevel() const
{
  return std::clamp(m_app->config().anisotropyLevel, 1.0f,
                    m_device.maxSamplerAnisotropy());
}

void Renderer::signalResize()
{
  m_fbGeneration++;
}

const vk::DescriptorSetLayout Renderer::requestDescriptorSetLayout(
    vk::DescriptorSetLayoutCreateInfo info)
{
  size_t hash{0};
  seng::internal::hashCombine(hash, info.flags);
  for (size_t i = 0; i < info.bindingCount; i++)
    seng::internal::hashCombine(hash, info.pBindings[i]);

  // Emplace already creates the item only if it doesn't already exist
  auto i = m_layoutCache.try_emplace(hash, m_device.logical(), info);
  if (i.second) seng::log::dbg("Allocated new descriptor layout");
  return *i.first->second;
}

void Renderer::clearDescriptorSetLayouts()
{
  m_layoutCache.clear();
}

const vk::DescriptorSet Renderer::requestDescriptorSet(
    FrameHandle frameHandle,
    vk::DescriptorSetLayout layout,
    const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
    const std::vector<vk::DescriptorImageInfo> &imageInfo)
{
  if (frameHandle.invalid(m_frames.size()))
    throw runtime_error("Invalid frame handle passed");

  size_t hash{0};
  internal::hashCombine(hash, layout, bufferInfo, imageInfo);

  auto &f = m_frames[frameHandle.m_index];
  auto iter = f.m_descriptorCache.find(hash);

  // If a descriptor set for the given layout has already been allocated,
  // return it
  if (iter != f.m_descriptorCache.end()) return *iter->second;

  // Else allocate it
  array<vk::DescriptorSetLayout, 1> descs = {layout};

  vk::DescriptorSetAllocateInfo info{};
  info.descriptorPool = *m_descriptorPool;
  info.setSetLayouts(descs);

  vk::raii::DescriptorSets sets(m_device.logical(), info);
  auto ret = f.m_descriptorCache.emplace(hash, std::move(sets[0]));
  return *ret.first->second;
}

const vk::DescriptorSet Renderer::getDescriptorSet(
    FrameHandle frameHandle,
    vk::DescriptorSetLayout layout,
    const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
    const std::vector<vk::DescriptorImageInfo> &imageInfo) const
{
  if (frameHandle.invalid(m_frames.size()))
    throw runtime_error("Invalid frame handle passed");

  size_t hash{0};
  internal::hashCombine(hash, layout, bufferInfo, imageInfo);

  auto &f = m_frames[frameHandle.m_index];
  auto iter = f.m_descriptorCache.find(hash);

  if (iter != f.m_descriptorCache.end()) return *iter->second;
  seng::log::warning("Query for unknown descriptor set, returning null handle");
  return vk::DescriptorSet(nullptr);
}

void Renderer::clearDescriptorSet(FrameHandle frameHandle,
                                  vk::DescriptorSetLayout layout,
                                  const std::vector<vk::DescriptorBufferInfo> &bufferInfo,
                                  const std::vector<vk::DescriptorImageInfo> &imageInfo)
{
  size_t hash{0};
  internal::hashCombine(hash, layout, bufferInfo, imageInfo);

  if (frameHandle.invalid(m_frames.size()))
    throw runtime_error("Invalid frame handle passed");

  m_frames[frameHandle.m_index].m_descriptorCache.erase(hash);
}

void Renderer::clearDescriptorSets()
{
  for (auto &f : m_frames) f.m_descriptorCache.clear();
  m_descriptorPool.reset();
}

Mesh &Renderer::requestMesh(const std::string &name)
{
  auto iter = m_meshes.find(name);

  if (iter != m_meshes.end()) return iter->second;
  auto ret = m_meshes.try_emplace(
      name, Mesh::loadFromDisk(*this, m_app->config().assetPath, name));
  return ret.first->second;
}

void Renderer::clearMesh(const std::string &name)
{
  m_meshes.erase(name);
}

void Renderer::clearMeshes()
{
  m_meshes.clear();
}

vk::Sampler Renderer::requestSampler(vk::SamplerCreateInfo info)
{
  size_t hash{0};
  seng::internal::hashCombine(hash, info);

  auto iter = m_samplerCache.find(hash);

  if (iter != m_samplerCache.end()) return *iter->second;

  seng::log::dbg("Creating image sampler");
  auto ret =
      m_samplerCache.try_emplace(hash, vk::raii::Sampler(m_device.logical(), info));
  return *ret.first->second;
}

void Renderer::clearSamplers()
{
  m_samplerCache.clear();
}

const Texture &Renderer::requestTexture(const std::string &name, TextureType type)
{
  size_t hash{0};
  seng::internal::hashCombine(hash, name, type);

  auto iter = m_textures.find(hash);

  if (iter != m_textures.end()) return iter->second;

  auto ret = m_textures.try_emplace(
      hash, Texture::loadFromDisk(*this, type, SamplerOptions::optimal(*this),
                                  m_app->config().assetPath, name));
  return ret.first->second;
}

void Renderer::clearTexture(const std::string &name, TextureType type)
{
  size_t hash{0};
  seng::internal::hashCombine(hash, name, type);
  m_textures.erase(hash);
}

void Renderer::clearTextures()
{
  m_textures.clear();
}

const CommandBuffer &Renderer::getCommandBuffer(const FrameHandle &handle) const
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");
  return m_frames[handle.m_index].m_commandBuffer;
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
    auto res = m_device.logical().waitForFences(*frame.m_inFlightFence, true,
                                                std::numeric_limits<uint64_t>::max());
    switch (res) {
      case vk::Result::eSuccess:
        break;
      default:
        log::error("{}", vk::to_string(res));
        return nullopt;
    }

    frame.m_index = m_swapchain.nextImageIndex(frame.m_imageAvailableSem);

    CommandBuffer &cmd = frame.m_commandBuffer;
    cmd.reset();
    cmd.begin();

    return optional(FrameHandle{m_currentFrame});
  } catch (const exception &e) {
    log::warning("Caught exception: {}", e.what());
    return nullopt;
  }
}

void Renderer::beginMainRenderPass(const FrameHandle &handle) const
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");

  auto &frame = m_frames[handle.m_index];
  auto &fb = m_swapchainFbs[frame.m_index];
  auto &cmd = frame.m_commandBuffer;

  m_renderPass.begin(cmd, fb, m_swapchain.extent(), {0, 0});

  // Dynamic states
  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(m_swapchain.extent().width);
  viewport.height = static_cast<float>(m_swapchain.extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  cmd.buffer().setViewport(0, viewport);

  vk::Rect2D scissor{{0, 0}, m_swapchain.extent()};
  cmd.buffer().setScissor(0, scissor);
}

void Renderer::endMainRenderPass(const FrameHandle &handle) const
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");

  auto &frame = m_frames[handle.m_index];
  m_renderPass.end(frame.m_commandBuffer);
}

void Renderer::endFrame(FrameHandle &handle)
{
  if (handle.invalid(m_frames.size())) throw runtime_error("Invalid handle passed");

  auto &frame = m_frames[handle.m_index];

  frame.m_commandBuffer.end();

  // Reset the fence for use on the next frame
  m_device.logical().resetFences(*frame.m_inFlightFence);

  // Start submitting to queue
  vk::SubmitInfo submitInfo{};
  std::array<vk::CommandBuffer, 1> commandBuffers = {*frame.m_commandBuffer.buffer()};
  std::array<vk::Semaphore, 1> queueCompleteSems = {*frame.m_queueCompleteSem};
  std::array<vk::Semaphore, 1> imageAvailableSems = {*frame.m_imageAvailableSem};

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

  m_device.graphicsQueue().submit(submitInfo, *frame.m_inFlightFence);

  try {
    m_swapchain.present(m_device.presentQueue(), m_device.graphicsQueue(),
                        frame.m_queueCompleteSem, frame.m_index);
  } catch (const InadequateSwapchainException &e) {
    log::dbg("Error while presenting: swapchain out of date. Recreating...");
    recreateSwapchain();
  }

  frame.m_index = -1;  // Forget the image
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

  // Recreate swapchain framebuffers
  allocateSwapchainFramebuffers();

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
