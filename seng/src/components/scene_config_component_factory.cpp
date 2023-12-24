#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/log.hpp>

#include <memory>
#include <string>
#include <unordered_map>

using namespace std;
using namespace seng::components;

using TConfigCreateFunc = SceneConfigComponentFactory::TConfigCreateFunc;

SceneConfigComponentFactory::FuncStore &SceneConfigComponentFactory::configCreateFuncs()
{
  // IMPORTANT: static variables are allocated only once, so each time the
  // function is called, control will go over the allocation. Yes, we are never
  // deallocating the heap-allocated store, however if we would try to not leak
  // it we open a whole new can of works I do not want to deal with. The OS
  // will clean it up so it is good enough.
  static FuncStore *store = new FuncStore();
  return *store;
}

bool SceneConfigComponentFactory::registerComponent(const ComponentIdType &name,
                                                    TConfigCreateFunc create)
{
  auto &store = configCreateFuncs();
  auto it = store.find(name);
  if (it == store.end()) {
    store.insert({name, create});
    return true;
  }
  return false;
}

std::unique_ptr<BaseComponent> SceneConfigComponentFactory::createFromSceneConfig(
    Application &app,
    Entity &entity,
    const std::string &name,
    const YAML::Node &configNode)
{
  auto &store = configCreateFuncs();
  auto it = store.find(name);
  if (it != store.end()) return it->second(app, entity, configNode);
  seng::log::warning("No registered component matching {}", name);
  return nullptr;
}
