#pragma once

#include <seng/components/base_component.hpp>
#include <seng/components/definitions.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_manager.hpp>

#include <yaml-cpp/yaml.h>

#include <memory>

class TestController : public seng::ScriptComponent,
                       public seng::ConfigParsableComponent<TestController> {
 public:
  TestController(seng::Entity &entity, bool enabled = true);
  TestController(const TestController &) = delete;
  TestController(TestController &&) = delete;

  TestController &operator=(const TestController &) = delete;
  TestController &operator=(TestController &&) = delete;

  DECLARE_COMPONENT_ID("TestController");
  static std::unique_ptr<seng::BaseComponent> createFromConfig(seng::Entity &entity,
                                                               const YAML::Node &node);

  void onUpdate(float deltaTime) override;

 private:
  float m_delta;

  glm::vec3 initial_pos;
  glm::quat initial_rot;

  float m_speed = 7.0f;
  float m_rot = 70.0f;

  seng::Transform *m_transform;
  seng::InputManager *m_input;

  std::array<std::string, 2> m_scenes{"default", "other"};
  static size_t s_sceneIndex;  // Static so that it can persist between scenes

  void handleMovement();
  void handleRotation();
  void recenter();
};
