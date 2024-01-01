#include <seng/application.hpp>
#include <seng/components/camera.hpp>
#include <seng/components/transform.hpp>
#include <seng/log.hpp>
#include <seng/rendering/object_shader.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/rendering/shader_stage.hpp>
#include <seng/scene/entity.hpp>
#include <seng/scene/scene.hpp>

#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <tuple>

using namespace seng;
using namespace seng::rendering;
using namespace std;

// Misc functions
static vk::raii::DescriptorSetLayout createGlobalDescriptorLayout(const Device &device);
static bool isYamlNodeValidShader(const YAML::Node &node);

uint64_t Scene::EVENT_INDEX = 0;

Scene::Scene(Application &app) :
    m_app(std::addressof(app)),
    m_renderer(app.renderer().get()),
    m_globalDescriptorSetLayout(createGlobalDescriptorLayout(m_renderer->device())),
    m_mainCamera(nullptr)
{
  m_renderer->requestDescriptorSet(*m_globalDescriptorSetLayout);
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

std::unique_ptr<Scene> Scene::loadFromDisk(Application &app, std::string sceneName)
{
  std::unique_ptr<Scene> s = std::make_unique<Scene>(app);
  auto &config = s->m_app->config();

  // Parse config file
  filesystem::path scene{filesystem::path{config.scenePath} /
                         filesystem::path{sceneName + ".yml"}};
  YAML::Node sceneConfig = YAML::LoadFile(scene.string());

  if (sceneConfig["Shaders"] && sceneConfig["Shaders"].IsSequence()) {
    auto shaders = sceneConfig["Shaders"];
    for (YAML::const_iterator i = shaders.begin(); i != shaders.end(); ++i) {
      s->parseShader(config.shaderPath, *i);
    }
  }

  // TODO: Shaders instances
  if (sceneConfig["ShaderInstances"]) seng::log::dbg("Got shader instances");

  // Parse mesh list
  if (sceneConfig["Meshes"] && sceneConfig["Meshes"].IsSequence()) {
    auto meshes = sceneConfig["Meshes"];
    for (YAML::const_iterator i = meshes.begin(); i != meshes.end(); i++)
      s->parseMesh(config.assetPath, *i);
  }

  // Load entities
  if (sceneConfig["Entities"] && sceneConfig["Entities"].IsSequence()) {
    auto e = sceneConfig["Entities"];
    for (YAML::const_iterator i = e.begin(); i != e.end(); ++i) s->parseEntity(*i);
  }

  return s;
}

void Scene::parseShader(const string &shaderPath, const YAML::Node &shader)
{
  if (!isYamlNodeValidShader(shader)) {
    seng::log::warning("Malformed YAML shader, skipping");
    return;
  }

  std::string vertName = shader["vert"].as<string>();
  m_stages.try_emplace(vertName, m_renderer->device(), shaderPath, vertName,
                       ShaderStage::Type::eVertex);

  std::string fragName = shader["frag"].as<string>();
  m_stages.try_emplace(fragName, m_renderer->device(), shaderPath, fragName,
                       ShaderStage::Type::eFragment);

  std::string shaderName = shader["name"].as<string>();
  vector<const ShaderStage *> s{&m_stages.at(vertName), &m_stages.at(fragName)};
  m_shaders.try_emplace(shaderName, m_renderer->device(), m_renderer->renderPass(),
                        m_globalDescriptorSetLayout, shaderName, s);
}

bool isYamlNodeValidShader(const YAML::Node &node)
{
  return node.IsMap() && node["name"] && node["name"].IsScalar() && node["vert"] &&
         node["vert"].IsScalar() && node["frag"] && node["frag"].IsScalar();
}

void Scene::parseMesh(const std::string &assetPath, const YAML::Node &node)
{
  std::string name = node.as<string>();
  m_meshes.emplace(name, Mesh::loadFromDisk(*m_renderer, assetPath, name));
}

void Scene::parseEntity(const YAML::Node &node)
{
  if (!node.IsMap()) {
    seng::log::warning("Malformed YAML node");
    return;
  }

  std::string name = "Entity";
  if (node["name"] && node["name"].IsScalar()) name = node["name"].as<string>();
  Entity *ret = newEntity(name);

  if (node["transform"] && node["transform"].IsMap()) {
    auto &t = node["transform"];
    auto ptr = SceneConfigComponentFactory::create(*ret, Transform::componentId(), t);
    auto concrete = concreteUniquePtr<Transform>(std::move(ptr));
    ret->transform(std::move(concrete));
  }

  if (node["components"] && node["components"].IsSequence()) {
    auto &comps = node["components"];
    for (YAML::const_iterator i = comps.begin(); i != comps.end(); ++i) {
      auto &comp = *i;
      if (!comp.IsMap() || !comp["id"] || !comp["id"].IsScalar()) {
        seng::log::warning("Malformed YAML component, skipping...");
        continue;
      }
      std::string id = comp["id"].as<string>();
      std::unique_ptr<BaseComponent> ptr =
          SceneConfigComponentFactory::create(*ret, id, comp);
      ret->untypedInsert(id, std::move(ptr));
    }
  }
}

Scene::EntityList::const_iterator Scene::findByName(const std::string &name) const
{
  return std::find_if(m_entities.begin(), m_entities.end(),
                      [&](const auto &elem) { return elem.name() == name; });
}

Scene::EntityList::iterator Scene::findByName(const std::string &name)
{
  return std::find_if(m_entities.begin(), m_entities.end(),
                      [&](const auto &elem) { return elem.name() == name; });
}

std::vector<const Entity *> Scene::findAllByName(const std::string &name) const
{
  vector<const Entity *> ptrs;
  for (const auto &e : m_entities) {
    if (e.name() == name) ptrs.push_back(std::addressof(e));
  }
  return ptrs;
}

std::vector<Entity *> Scene::findAllByName(const std::string &name)
{
  vector<Entity *> ptrs;
  for (auto &e : m_entities) {
    if (e.name() == name) ptrs.push_back(std::addressof(e));
  }
  return ptrs;
}

Entity *Scene::newEntity(std::string name)
{
  m_entities.push_back(Entity(*m_app, *this, name));
  return &m_entities.back();
}

void Scene::removeEntity(EntityList::const_iterator i)
{
  m_entities.erase(i);
}

void Scene::removeEntity(const Entity *e)
{
  if (e == nullptr) {
    seng::log::warning("Tried to remove a null entity... Something is wrong");
    return;
  }
  auto it = std::find(m_entities.begin(), m_entities.end(), *e);
  if (it == m_entities.end()) {
    seng::log::warning(
        "Tried to remove an entity not registered in the scene graph... Something is "
        "wrong");
    return;
  }
  this->removeEntity(it);
}

void Scene::removeEntity(const string &name)
{
  auto it = findByName(name);
  if (it == m_entities.end()) {
    seng::log::warning("Could not find an entity with the given name to remove");
    return;
  }
  this->removeEntity(it);
}

void Scene::removeAllEntities()
{
  m_entities.clear();
}

void Scene::mainCamera(Camera *cam)
{
  if (cam == nullptr) {
    seng::log::warning("Trying to register null camera");
    return;
  }
  m_mainCamera = cam;
}

void Scene::draw(const FrameHandle &handle)
{
  const auto &cmd = m_renderer->getCommandBuffer(handle);

  if (m_mainCamera == nullptr) return;

  for (auto &shaderNamePair : m_shaders) {
    auto &shader = shaderNamePair.second;

    shader.globalUniformObject().projection = m_mainCamera->projectionMatrix();
    shader.globalUniformObject().view = m_mainCamera->viewMatrix();
    shader.uploadGlobalState(
        cmd, m_renderer->getDescriptorSet(handle, *m_globalDescriptorSetLayout));

    // FIXME: should be per entity rendererd with the shader not per mesh
    for (const auto &mesh : m_meshes) {
      shader.updateModelState(cmd, glm::mat4(1));
      cmd.buffer().bindVertexBuffers(0, *mesh.second.vertexBuffer().buffer(), {0});
      cmd.buffer().bindIndexBuffer(*mesh.second.indexBuffer().buffer(), 0,
                                   vk::IndexType::eUint32);
      shader.use(cmd);
      cmd.buffer().drawIndexed(mesh.second.indexCount(), 1, 0, 0, 0);
    }
  }
}

SceneEventToken Scene::listen(SceneEvents e, std::function<void(float)> h)
{
  EventHandlerType handler = make_tuple(EVENT_INDEX++, h);
  m_eventHandlers[e].push_back(handler);
  SceneEventToken tok;
  tok.event = e;
  tok.id = EVENT_INDEX;
  return tok;
}

void Scene::unlisten(SceneEventToken tok)
{
  auto i = m_eventHandlers.find(tok.event);
  if (i == m_eventHandlers.end()) return;

  auto end = std::remove_if(i->second.begin(), i->second.end(),
                            [&](const auto &h) { return tok.id == std::get<0>(h); });
  i->second.erase(end, i->second.end());
}

void Scene::fireEventType(SceneEvents event, float delta) const
{
  auto i = m_eventHandlers.find(event);
  if (i == m_eventHandlers.end()) return;
  for (const auto &ev : i->second) std::get<1>(ev)(delta);
}

Scene::~Scene()
{
  // Clear descriptors only if we are not in a moved-from state
  if (*m_globalDescriptorSetLayout != vk::DescriptorSetLayout{}) {
    // Ensure that every operation relative to this scene has been completed
    m_renderer->device().logical().waitIdle();

    // Clear requested resources
    m_renderer->clearDescriptorSet(*m_globalDescriptorSetLayout);
  }
}
