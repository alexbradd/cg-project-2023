#include <seng/application_config.hpp>
#include <seng/log.hpp>
#include <seng/rendering/buffer.hpp>
#include <seng/rendering/debug_messenger.hpp>
#include <seng/rendering/device.hpp>
#include <seng/rendering/framebuffer.hpp>
#include <seng/rendering/glfw_window.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/render_pass.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/rendering/swapchain.hpp>
#include <seng/utils.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
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

// Stub geometry uploading
static void uploadTo(const Device &device,
                     const vk::raii::CommandPool &pool,
                     const vk::raii::Queue &queue,
                     const Buffer &to,
                     vk::DeviceSize size,
                     vk::DeviceSize offset,
                     const void *data)
{
  vk::MemoryPropertyFlags hostVisible = vk::MemoryPropertyFlagBits::eHostCoherent |
                                        vk::MemoryPropertyFlagBits::eHostVisible;

  Buffer temp(device, vk::BufferUsageFlagBits::eTransferSrc, size, hostVisible, true);
  temp.load(data, 0, size, {});
  temp.copy(to, {0, offset, size}, pool, queue);
}

// Definitions for RenderTarget
Renderer::RenderTarget::RenderTarget(const Device &device,
                                     const vk::ImageView swapchainImage,
                                     vk::Extent2D extent,
                                     const RenderPass &pass) :
    swapchainImage(swapchainImage),
    depthBuffer(createDepthBuffer(device, extent)),
    framebuffer(device, pass, extent, {swapchainImage, **depthBuffer.imageView()})
{
  log::info("Allocated render target");
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
    imageIndex(-1)
{
  log::info("Allocated resources for a frame");
}

// Definitions for FrameHandle
bool FrameHandle::invalid(size_t maxValue) const
{
  return frameIndex == -1 || frameIndex >= static_cast<ssize_t>(maxValue);
}

void FrameHandle::invalidate()
{
  frameIndex = -1;
}

// Defintions for renderer
static constexpr vk::CommandPoolCreateFlags cmdPoolFlags =
    vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

static constexpr vk::BufferUsageFlags vertexBufferUsage =
    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags indexBufferUsage =
    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;

const std::vector<const char *> Renderer::VALIDATION_LAYERS{
    "VK_LAYER_KHRONOS_validation"};

Renderer::Renderer(ApplicationConfig config, const GlfwWindow &window) :
    window(std::addressof(window)),
    context(),
    // Instance creation
    instance(createInstance(context, window)),
    debugMessenger(instance, USE_VALIDATION),
    surface(window.createVulkanSurface(instance)),

    // Device and swapchain
    device(instance, surface),
    swapchain(device, surface, window),

    // Renderpass
    attachments{
        // Color attachment
        {{swapchain.format().format, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
          vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR,
          vk::ImageLayout::eColorAttachmentOptimal,
          vk::ClearColorValue{0.0f, 0.0f, 1.0f, 0.0f}},
         // Depth attachment
         {device.depthFormat().format, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
          vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal,
          vk::ImageLayout::eDepthStencilAttachmentOptimal,
          vk::ClearDepthStencilValue{1.0f, 0}}}},
    renderPass(device, attachments),

    // Pools
    cmdPool(device.logical(),
            {cmdPoolFlags, *device.queueFamilyIndices().graphicsFamily}),

    // Buffers
    vertexBuffer(device, vertexBufferUsage, sizeof(Vertex) * 1024 * 1024),
    indexBuffer(device, indexBufferUsage, sizeof(Vertex) * 1024 * 1024)
{
  // Targets and frames
  for (auto &img : swapchain.images())
    targets.emplace_back(device, *img, swapchain.extent(), renderPass);

  frames = many<Renderer::Frame>(swapchain.MAX_FRAMES_IN_FLIGHT, device, cmdPool);
  targets.reserve(swapchain.images().size());

  log::info("Vulkan context is up and running!");

  // FIXME: Temporary geometry
  log::dbg("Loading test geometry");
  verts[0].pos = glm::vec3(-0.5, -0.5, 0.0);
  verts[1].pos = glm::vec3(0.5, -0.5, 0.0);
  verts[2].pos = glm::vec3(0.5, 0.5, 0.0);
  verts[3].pos = glm::vec3(-0.5, 0.5, 0.0);
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 2;
  indices[5] = 3;
  uploadTo(device, cmdPool, device.graphicsQueue(), vertexBuffer, sizeof(Vertex) * 4, 0,
           verts.data());
  uploadTo(device, cmdPool, device.graphicsQueue(), indexBuffer, sizeof(uint32_t) * 6, 0,
           indices.data());
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
  fbGeneration++;
}

optional<FrameHandle> Renderer::beginFrame()
{
  if (recreatingSwapchain) {
    device.logical().waitIdle();
    seng::log::dbg("Already recreating swapchain, waiting...");
    return nullopt;
  }

  if (lastFbGeneration != fbGeneration) {
    device.logical().waitIdle();
    recreateSwapchain();
    seng::log::dbg("Framebuffer changed, aborting...");
    return nullopt;
  }

  auto &frame = frames[currentFrame];
  try {
    frame.inFlightFence.wait();
    frame.imageIndex = swapchain.nextImageIndex(frame.imageAvailableSem);

    Framebuffer &curFb = targets[frame.imageIndex].framebuffer;
    CommandBuffer &curBuf = frame.commandBuffer;
    curBuf.reset();
    curBuf.begin();

    // Begin the render pass
    renderPass.begin(curBuf, curFb, swapchain.extent(), {0, 0});

    // Dynamic states
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent().width);
    viewport.height = static_cast<float>(swapchain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    curBuf.buffer().setViewport(0, viewport);

    vk::Rect2D scissor{{0, 0}, swapchain.extent()};
    curBuf.buffer().setScissor(0, scissor);

    return optional(FrameHandle{currentFrame});
  } catch (const exception &e) {
    log::warning("Caught exception: {}", e.what());
    return nullopt;
  }
}

// FIXME: start of stub
void Renderer::updateGlobalState(glm::mat4 projection, glm::mat4 view) const
{
  /* auto &commandBuffer = graphicsCmdBufs[imageIndex]; */
  /* auto shader = shaderLoader.getShader("default"); */
  /**/
  /* shader->globalUniformObject().projection = projection; */
  /* shader->globalUniformObject().view = view; */
  /**/
  /* // TODO: add other properties */
  /**/
  /* shader->uploadGlobalState(commandBuffer, imageIndex); */
}

void Renderer::updateModel(glm::mat4 model) const
{
  /* auto &commandBuffer = graphicsCmdBufs[imageIndex]; */
  /* auto shader = shaderLoader.getShader("default"); */
  /* shader->updateModelState(commandBuffer, model); */
}

void Renderer::draw() const
{
  /* auto &commandBuffer = graphicsCmdBufs[imageIndex]; */
  /* auto shader = shaderLoader.getShader("default"); */
  /**/
  /* shader->use(commandBuffer); */
  /* commandBuffer.buffer().bindVertexBuffers(0, {*vertexBuffer.buffer()}, {0}); */
  /* commandBuffer.buffer().bindIndexBuffer(*indexBuffer.buffer(), 0, */
  /*                                        vk::IndexType::eUint32); */
  /* commandBuffer.buffer().drawIndexed(6, 1, 0, 0, 0); */
}
// FIXME: end of stub

void Renderer::endFrame(FrameHandle &handle)
{
  if (handle.invalid(frames.size())) throw runtime_error("Invalid handle passed");

  auto &frame = frames[handle.frameIndex];

  renderPass.end(frame.commandBuffer);
  frame.commandBuffer.end();

  // Reset the fence for use on the next frame
  frame.inFlightFence.reset();

  // Start submitting to queue
  vk::SubmitInfo submitInfo{};
  std::array<vk::CommandBuffer, 1> commandBuffers{*frame.commandBuffer.buffer()};
  std::array<vk::Semaphore, 1> queueCompleteSems{*frame.queueCompleteSem};
  std::array<vk::Semaphore, 1> imageAvailableSems{*frame.imageAvailableSem};

  submitInfo.setCommandBuffers(commandBuffers);
  // The semaphore(s) to be signaled when the queue is complete.
  submitInfo.setSignalSemaphores(queueCompleteSems);
  // Wait semaphore ensures that the operation cannot begin until the image is available
  submitInfo.setWaitSemaphores(imageAvailableSems);

  // Each semaphore waits on the corresponding pipeline stage to complete.
  // 1:1 ratio. VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents
  // subsequent colour attachment writes from executing until the semaphore
  // signals (i.e. one frame is presented at a time)
  array<vk::PipelineStageFlags, 1> flags{
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.setWaitDstStageMask(flags);

  device.graphicsQueue().submit(submitInfo, *frame.inFlightFence.handle());

  try {
    swapchain.present(device.presentQueue(), device.graphicsQueue(),
                      frame.queueCompleteSem, frame.imageIndex);
  } catch (const InadequateSwapchainException &e) {
    log::info("Error while presenting: swapchain out of date. Recreating...");
    recreateSwapchain();
  }

  frame.imageIndex = -1;  // Forget the image
  handle.invalidate();

  // Advance cyclical iterator
  currentFrame = (currentFrame + 1) % swapchain.MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recreateSwapchain()
{
  // If already recreating, do nothing.
  if (recreatingSwapchain) return;

  // Get the new framebuffer size, if null do nothing
  pair<unsigned int, unsigned int> fbSize = window->framebufferSize();
  if (fbSize.first == 0 || fbSize.second == 0) {
    log::dbg("Null framebuffer, aborting...");
    return;
  }

  // Start the creation process
  recreatingSwapchain = true;
  log::info("Started swapchain recreation");

  // Wait for any pending work to finish and flush out temporary data
  device.logical().waitIdle();

  // Requery swapchain support data (it might have changed)
  device.requerySupport();
  device.requeryDepthFormat();

  // Recreate the swapchain and clear the currentFrame counter
  swapchain = Swapchain(device, surface, *window, swapchain.swapchain());
  currentFrame = 0;

  // Sync framebuffer generation
  lastFbGeneration = fbGeneration;

  // Recreate render targets
  targets.clear();
  for (auto &img : swapchain.images())
    targets.emplace_back(device, *img, swapchain.extent(), renderPass);

  // Finish the recreation process
  recreatingSwapchain = false;
  log::info("Finished swapchain recreation");
}

Renderer::~Renderer()
{
  // Just checking if the instance handle is valid is enough
  // since all objects are valid or none are.
  if (*instance != vk::Instance{}) {
    device.logical().waitIdle();
    log::info("Destroying vulkan context");
  }
}
