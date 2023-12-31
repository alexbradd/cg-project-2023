#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_manager.hpp>

#include <yaml-cpp/yaml.h>

#include <memory>

class TestController : public seng::components::ScriptComponent,
                       public seng::components::ConfigParsableComponent<TestController> {
 public:
  TestController(seng::scene::Entity &entity, bool enabled = true) :
      seng::components::ScriptComponent(entity, enabled)
  {
  }
  TestController(const TestController &) = delete;
  TestController(TestController &&) = delete;

  TestController &operator=(const TestController &) = delete;
  TestController &operator=(TestController &&) = delete;

  DECLARE_COMPONENT_ID("TestController");
  static std::unique_ptr<seng::components::BaseComponent> createFromConfig(
      seng::scene::Entity &entity, const YAML::Node &node);

  void scriptInitialize() override;
  void onUpdate(float deltaTime) override;

 private:
  float m_delta;

  glm::vec3 initial_pos;
  glm::quat initial_rot;

  float m_speed = 7.0f;
  float m_rot = 70.0f;

  seng::components::Transform *m_transform;
  seng::InputManager *m_input;

  void handleMovement();
  void handleRotation();
  void recenter();
};
