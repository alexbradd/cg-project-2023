#include <seng/application.hpp>
#include <seng/log.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/scene.hpp>
#include <seng/rendering/shader_stage.hpp>

#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <memory>
#include <unordered_set>

using namespace seng::rendering;
using namespace std;

#define VERT_NAME "simple_vert"
#define FRAG_NAME "simple_frag"
#define SHADER_NAME "default"

// Initializer functions
static vk::raii::DescriptorSetLayout createGlobalDescriptorLayout(const Device &device);

Scene::Scene(const Application &app,
             const CameraParams &cameraParams,
             [[maybe_unused]] const std::unordered_set<std::string> &stageNames,
             [[maybe_unused]] const std::unordered_set<std::string> &shaderNames,
             const std::unordered_set<std::string> &meshNames) :
    renderer(app.renderer().get()),
    globalDescriptorSetLayout(createGlobalDescriptorLayout(renderer->getDevice())),
    camera(
        cameraParams.aspectRatio, cameraParams.near, cameraParams.far, cameraParams.fov)
{
  auto &config = app.config();

  renderer->requestDescriptorSet(*globalDescriptorSetLayout);

  camera.transform().setPos(0.0, 0.0, 10.0f);  // FIXME: stub

  // Load ShaderStages
  // FIXME: stub
  stages.try_emplace(VERT_NAME, renderer->getDevice(), config.shaderPath, VERT_NAME,
                     ShaderStage::Type::eVertex);
  stages.try_emplace(FRAG_NAME, renderer->getDevice(), config.shaderPath, FRAG_NAME,
                     ShaderStage::Type::eFragment);

  // Load ObjectShaders
  // FIXME: stub
  vector<const ShaderStage *> s{&stages.at(VERT_NAME), &stages.at(FRAG_NAME)};
  shaders.try_emplace(SHADER_NAME, renderer->getDevice(), renderer->getRenderPass(),
                      globalDescriptorSetLayout, SHADER_NAME, s);

  // TODO: load shader instances

  // Load meshes
  for (const auto &s : meshNames) {
    meshes.emplace(s, Mesh::loadFromDisk(*renderer, config, s));
  }

  // TODO: load entities
}

vk::raii::DescriptorSetLayout createGlobalDescriptorLayout(const Device &device)
{
  vk::DescriptorSetLayoutBinding guboBinding{};
  guboBinding.binding = 0;
  guboBinding.descriptorCount = 1;
  guboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  guboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

  vk::DescriptorSetLayoutCreateInfo info{};
  info.bindingCount = 1;
  info.pBindings = &guboBinding;
  return vk::raii::DescriptorSetLayout(device.logical(), info);
}

Scene Scene::loadFromDisk(const Application &app,
                          std::string sceneName,
                          float cameraAspectRatio)
{
  // Parse config file
  filesystem::path scene{filesystem::path{app.config().scenePath} /
                         filesystem::path{sceneName + ".yml"}};
  YAML::Node sceneConfig = YAML::LoadFile(scene.string());
  unordered_set<string> shaderStages;
  unordered_set<string> shaders;
  // TODO: how to go about instances?
  unordered_set<string> meshPaths;

  // TODO: Shaders
  if (sceneConfig["Shaders"]) seng::log::dbg("Got shaders");

  // TODO: Shaders isntances
  if (sceneConfig["ShaderInstances"]) seng::log::dbg("Got shader instances");

  // Parse mesh list
  if (sceneConfig["Meshes"]) {
    auto meshes = sceneConfig["Meshes"];
    for (YAML::const_iterator i = meshes.begin(); i != meshes.end(); i++) {
      meshPaths.insert(i->as<string>());
    }
  }

  // Parse camera parameters
  CameraParams cameraParams;
  cameraParams.aspectRatio = cameraAspectRatio;
  if (sceneConfig["Camera"]) {
    auto cam = sceneConfig["Camera"];
    if (cam["near"]) cameraParams.near = cam["near"].as<float>();
    if (cam["far"]) cameraParams.far = cam["far"].as<float>();
    if (cam["fov"]) cameraParams.fov = cam["fov"].as<float>();
    // TODO: parse initial position
  }

  // TODO: parse entities
  if (sceneConfig["Entities"]) seng::log::dbg("Got entities");

  return Scene(app, cameraParams, shaderStages, shaders, meshPaths);
}

void Scene::draw(const FrameHandle &handle)
{
  const auto &cmd = renderer->getCommandBuffer(handle);
  auto &shader = shaders.at(SHADER_NAME);  // FIXME: stub

  shader.globalUniformObject().projection = camera.projectionMatrix();
  shader.globalUniformObject().view = camera.viewMatrix();
  shader.uploadGlobalState(
      cmd, renderer->getDescriptorSet(handle, *globalDescriptorSetLayout));

  for (const auto &mesh : meshes) {  // FIXME: should be per game object not per mesh
    shader.updateModelState(cmd, glm::mat4(1));
    cmd.buffer().bindVertexBuffers(0, *mesh.second.vertexBuffer().buffer(), {0});
    cmd.buffer().bindIndexBuffer(*mesh.second.indexBuffer().buffer(), 0,
                                 vk::IndexType::eUint32);
    shaders.at(SHADER_NAME).use(cmd);  // FIXME: stub
    cmd.buffer().drawIndexed(mesh.second.indexCount(), 1, 0, 0, 0);
  }
}

Scene::~Scene()
{
  // Clear descriptors only if we are not in a moved-from state
  if (*globalDescriptorSetLayout != vk::DescriptorSetLayout{}) {
    // Ensure that every operation relative to this scene has been completed
    renderer->getDevice().logical().waitIdle();

    // Clear requested resources
    renderer->clearDescriptorSets();
  }
}
