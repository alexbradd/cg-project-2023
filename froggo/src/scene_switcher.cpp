#include <seng/application.hpp>
#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_manager.hpp>

#include <yaml-cpp/yaml.h>

class SceneSwitcher : public seng::ScriptComponent,
                      public seng::ConfigParsableComponent<SceneSwitcher> {
 public:
  SceneSwitcher(seng::Entity &entity, bool enabled = true);
  SceneSwitcher(const SceneSwitcher &) = delete;
  SceneSwitcher(SceneSwitcher &&) = delete;

  SceneSwitcher &operator=(const SceneSwitcher &) = delete;
  SceneSwitcher &operator=(SceneSwitcher &&) = delete;

  DECLARE_COMPONENT_ID("SceneSwitcher");
  DECLARE_CREATE_FROM_CONFIG();

  void onUpdate(float deltaTime) override;

 private:
  static std::array<std::string, 2> SCENE_NAMES;
  static size_t SCENE_INDEX;
};

REGISTER_TO_CONFIG_FACTORY(SceneSwitcher);

std::array<std::string, 2> SceneSwitcher::SCENE_NAMES = {"default", "ggx"};
size_t SceneSwitcher::SCENE_INDEX = 0;

DEFINE_CREATE_FROM_CONFIG(SceneSwitcher, entity, node)
{
  bool enabled = true;

  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  return std::make_unique<SceneSwitcher>(entity, enabled);
}

SceneSwitcher::SceneSwitcher(seng::Entity &e, bool enabled) : ScriptComponent(e, enabled)
{
}

void SceneSwitcher::onUpdate([[maybe_unused]] float deltaTime)
{
  auto &input = entity->application().input();

  if (input->keyDown(seng::KeyCode::eRightBracket)) {
    SCENE_INDEX = (SCENE_INDEX + 1) % SCENE_NAMES.size();
    entity->application().switchScene(SCENE_NAMES[SCENE_INDEX]);
  }
  if (input->keyDown(seng::KeyCode::eLeftBracket)) {
    SCENE_INDEX = (SCENE_INDEX - 1) % SCENE_NAMES.size();
    entity->application().switchScene(SCENE_NAMES[SCENE_INDEX]);
  }
  if (input->keyDown(seng::KeyCode::eEsc)) entity->application().stop();
}
