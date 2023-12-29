#include <seng/application.hpp>
#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_enums.hpp>
#include <seng/input_manager.hpp>
#include <seng/log.hpp>
#include <seng/rendering/glfw_window.hpp>

#include <yaml-cpp/yaml.h>

#include <memory>

using namespace seng;

class TestController : public components::ScriptComponent,
                       public components::ConfigParsableComponent<TestController> {
 public:
  TestController(scene::Entity &entity) : components::ScriptComponent(entity) {}
  TestController(const TestController &) = delete;
  TestController(TestController &&) = delete;

  TestController &operator=(const TestController &) = delete;
  TestController &operator=(TestController &&) = delete;

  DECLARE_COMPONENT_ID("TestController");
  static std::unique_ptr<components::BaseComponent> createFromConfig(
      scene::Entity &entity, const YAML::Node &node)
  {
    return std::make_unique<TestController>(entity);
  }

  void scriptInitialize() override
  {
    t = entity->transform().get();
    inputManager = entity->application().input().get();
  }

  void onUpdate(float deltaTime) override
  {
    if (inputManager->keyHold(seng::KeyCode::eKeyA))
      t->translate(-t->right() * speed * deltaTime);
    if (inputManager->keyHold(seng::KeyCode::eKeyD))
      t->translate(t->right() * speed * deltaTime);
    if (inputManager->keyHold(seng::KeyCode::eKeyW))
      t->translate(-t->forward() * speed * deltaTime);
    if (inputManager->keyHold(seng::KeyCode::eKeyS))
      t->translate(t->forward() * speed * deltaTime);
    if (inputManager->keyHold(seng::KeyCode::eSpace)) {
      if (inputManager->keyHold(seng::KeyCode::eModLeftShift))
        t->translate(-t->up() * speed * deltaTime);
      else
        t->translate(t->up() * speed * deltaTime);
    }

    if (inputManager->keyHold(seng::KeyCode::eUp))
      t->rotate(glm::radians(1.0f), 0.0f, 0.0f);
    else if (inputManager->keyHold(seng::KeyCode::eDown))
      t->rotate(glm::radians(-1.0f), 0.0f, 0.0f);
    else if (inputManager->keyHold(seng::KeyCode::eLeft))
      t->rotate(0.0f, glm::radians(1.0f), 0.0f);
    else if (inputManager->keyHold(seng::KeyCode::eRight))
      t->rotate(0.0f, glm::radians(-1.0f), 0.0f);

    if (inputManager->keyDown(seng::KeyCode::eEsc)) entity->application().stop();
  }

 private:
  float speed = 5.0f;
  components::Transform *t;
  InputManager *inputManager;
};
