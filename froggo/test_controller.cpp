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
      scene::Entity &entity, [[maybe_unused]] const YAML::Node &node)
  {
    return std::make_unique<TestController>(entity);
  }

  void scriptInitialize() override
  {
    m_transform = entity->transform().get();
    m_input = entity->application().input().get();
  }

  void onUpdate(float deltaTime) override
  {
    if (m_input->keyHold(seng::KeyCode::eKeyA))
      m_transform->translate(-m_transform->right() * m_speed * deltaTime);
    if (m_input->keyHold(seng::KeyCode::eKeyD))
      m_transform->translate(m_transform->right() * m_speed * deltaTime);
    if (m_input->keyHold(seng::KeyCode::eKeyW))
      m_transform->translate(-m_transform->forward() * m_speed * deltaTime);
    if (m_input->keyHold(seng::KeyCode::eKeyS))
      m_transform->translate(m_transform->forward() * m_speed * deltaTime);
    if (m_input->keyHold(seng::KeyCode::eSpace)) {
      if (m_input->keyHold(seng::KeyCode::eModLeftShift))
        m_transform->translate(-m_transform->up() * m_speed * deltaTime);
      else
        m_transform->translate(m_transform->up() * m_speed * deltaTime);
    }

    if (m_input->keyHold(seng::KeyCode::eUp))
      m_transform->rotate(glm::radians(1.0f), 0.0f, 0.0f);
    else if (m_input->keyHold(seng::KeyCode::eDown))
      m_transform->rotate(glm::radians(-1.0f), 0.0f, 0.0f);
    else if (m_input->keyHold(seng::KeyCode::eLeft))
      m_transform->rotate(0.0f, glm::radians(1.0f), 0.0f);
    else if (m_input->keyHold(seng::KeyCode::eRight))
      m_transform->rotate(0.0f, glm::radians(-1.0f), 0.0f);

    if (m_input->keyDown(seng::KeyCode::eEsc)) entity->application().stop();
  }

 private:
  float m_speed = 5.0f;
  components::Transform *m_transform;
  InputManager *m_input;
};
