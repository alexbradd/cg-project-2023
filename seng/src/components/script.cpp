#include <seng/application.hpp>
#include <seng/components/base_component.hpp>
#include <seng/components/script.hpp>
#include <seng/scene/scene.hpp>

#include <functional>

using namespace seng;
using namespace seng::components;

using namespace std::placeholders;

ScriptComponent::ScriptComponent(scene::Entity &entity) : BaseComponent(entity) {}

void ScriptComponent::initialize()
{
  auto &s = entity->getScene();

  earlyUpdateToken = s.listen(scene::SceneEvents::EARLY_UPDATE,
                              std::bind(&ScriptComponent::onEarlyUpdate, this, _1));
  updateToken = s.listen(scene::SceneEvents::UPDATE,
                         std::bind(&ScriptComponent::onUpdate, this, _1));
  lateUpdateToken = s.listen(scene::SceneEvents::UPDATE,
                             std::bind(&ScriptComponent::onLateUpdate, this, _1));
  scriptInitialize();
}

ScriptComponent::~ScriptComponent()
{
  if (!entity) return;  // We are in a moved-from state

  auto &s = entity->getScene();

  s.unlisten(earlyUpdateToken);
  s.unlisten(updateToken);
  s.unlisten(lateUpdateToken);
}
