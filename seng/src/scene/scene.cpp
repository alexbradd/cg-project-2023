#include <seng/application.hpp>
#include <seng/components/camera.hpp>
#include <seng/components/mesh_renderer.hpp>
#include <seng/components/transform.hpp>
#include <seng/log.hpp>
#include <seng/rendering/primitive_types.hpp>
#include <seng/rendering/renderer.hpp>
#include <seng/scene/entity.hpp>
#include <seng/scene/scene.hpp>
#include <seng/yaml_utils.hpp>

#include <yaml-cpp/yaml.h>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <memory>
#include <string>

using namespace seng;
using namespace seng::rendering;
using namespace std;

Scene::Scene(Application &app) :
    m_app(std::addressof(app)), m_renderer(app.renderer().get()), m_mainCamera(nullptr)
{
  seng::log::dbg("Created new scene");
}

std::unique_ptr<Scene> Scene::loadFromDisk(Application &app, std::string sceneName)
{
  std::unique_ptr<Scene> s = std::make_unique<Scene>(app);
  auto &config = s->m_app->config();

  // Parse config file
  filesystem::path scene{filesystem::path{config.scenePath} /
                         filesystem::path{sceneName + ".yml"}};

  YAML::Node sceneConfig;

  try {
    sceneConfig = YAML::LoadFile(scene.string());
  } catch (exception &e) {
    seng::log::error("Unable to load scene: {}", e.what());
    return nullptr;
  }

  // Parse light
  if (sceneConfig["Light"] && sceneConfig["Light"].IsMap()) {
    auto light = sceneConfig["Light"];
    if (light["ambient"] && light["ambient"].IsSequence())
      s->m_ambient = light["ambient"].as<glm::vec4>();
    if (light["color"] && light["color"].IsSequence())
      s->m_directLight.color(light["color"].as<glm::vec4>());
    if (light["direction"] && light["direction"].IsSequence())
      s->m_directLight.direction(light["direction"].as<glm::vec3>());
  }

  // Load entities
  if (sceneConfig["Entities"] && sceneConfig["Entities"].IsSequence()) {
    auto e = sceneConfig["Entities"];
    for (YAML::const_iterator i = e.begin(); i != e.end(); ++i) s->parseEntity(*i);
  }

  return s;
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
    ret->transform(std::move(ptr));
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
      auto ptr = SceneConfigComponentFactory::create(*ret, id, comp);
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

HookRegistrar<const rendering::CommandBuffer &> &Scene::onShaderInstanceDraw(
    const std::string &instance)
{
  auto it = m_renderer->shaders().objectShaderInstances().find(instance);
  if (it == m_renderer->shaders().objectShaderInstances().end())
    seng::log::warning("Attempting to register for unknown instance");
  return m_renderers[instance].registrar();
}

void Scene::draw(const FrameHandle &handle)
{
  const auto &cmd = m_renderer->getCommandBuffer(handle);

  if (m_mainCamera == nullptr) return;

  // Update projection binding
  m_renderer->globalUniform().projection().projection = m_mainCamera->projectionMatrix();
  m_renderer->globalUniform().projection().view = m_mainCamera->viewMatrix();

  // Update lighting binding
  m_renderer->globalUniform().lighting().ambientColor = m_ambient;
  m_renderer->globalUniform().lighting().lightColor = m_directLight.color();
  m_renderer->globalUniform().lighting().lightDir = m_directLight.direction();
  m_renderer->globalUniform().lighting().cameraPosition =
      m_mainCamera->attachedTo().transform()->position();

  // Push to device
  m_renderer->globalUniform().update(handle);

  // For each pipeline
  for (auto &shader : m_renderer->shaders().objectShaders()) {
    // Bind said pipeline
    shader.second.use(cmd);

    // For each instance of that pipeline
    for (auto instancePtr : shader.second.instances()) {
      // Check if any MeshRenderers are using it
      auto renderers = m_renderers.find(instancePtr->name());
      if (renderers == m_renderers.end()) continue;
      if (renderers->second.empty()) continue;

      // If there are any, bind the descriptors
      instancePtr->bindDescriptorSets(handle, cmd);

      // For each registered MeshRenderer, render it
      renderers->second(cmd);
    }
  }
}

void Scene::update(Timestamp lastFrame, const FrameHandle &handle)
{
  float deltaTime = inSeconds(Clock::now() - lastFrame);
  m_earlyUpdate(deltaTime);

  // Update
  m_update(deltaTime);
  draw(handle);

  // Late update
  m_lateUpdate(deltaTime);
}

Scene::~Scene()
{
  // Ensure that every operation relative to this scene has been completed
  m_renderer->device().logical().waitIdle();
  seng::log::dbg("Deallocated scene");
}
