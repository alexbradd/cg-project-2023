#include "test_controller.hpp"

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
#include <glm/gtx/norm.hpp>

#include <memory>

using namespace seng;

size_t TestController::s_sceneIndex = 0;

std::unique_ptr<BaseComponent> TestController::createFromConfig(Entity &entity,
                                                                const YAML::Node &node)
{
  bool enabled = true;

  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  return std::make_unique<TestController>(entity, enabled);
}

void TestController::scriptInitialize()
{
  m_transform = entity->transform().get();
  m_input = entity->application().input().get();

  initial_pos = m_transform->position();
  initial_rot = m_transform->quaternion();
}

void TestController::onUpdate(float deltaTime)
{
  m_delta = deltaTime;

  handleRotation();
  handleMovement();

  // Misc
  if (m_input->keyDown(seng::KeyCode::eEnter)) recenter();
  if (m_input->keyDown(seng::KeyCode::eKeyM)) {
    s_sceneIndex = (s_sceneIndex + 1) % m_scenes.size();
    entity->application().switchScene(m_scenes[s_sceneIndex]);
  }
  if (m_input->keyDown(seng::KeyCode::eEsc)) entity->application().stop();
}

void TestController::handleMovement()
{
  glm::vec3 moveDirection(0.0f);

  if (m_input->keyHold(seng::KeyCode::eKeyA)) moveDirection += -m_transform->right();
  if (m_input->keyHold(seng::KeyCode::eKeyD)) moveDirection += m_transform->right();
  if (m_input->keyHold(seng::KeyCode::eKeyW)) moveDirection += -m_transform->forward();
  if (m_input->keyHold(seng::KeyCode::eKeyS)) moveDirection += m_transform->forward();
  if (m_input->keyHold(seng::KeyCode::eSpace)) {
    if (m_input->keyHold(seng::KeyCode::eModLeftShift))
      moveDirection += -m_transform->up();
    else
      moveDirection += m_transform->up();
  }

  if (glm::length2(moveDirection) > 0.0) moveDirection = glm::normalize(moveDirection);

  m_transform->translate(moveDirection * m_speed * m_delta);
}

void TestController::handleRotation()
{
  float rot_amount = glm::radians(m_rot * m_delta);
  if (m_input->keyHold(seng::KeyCode::eUp))
    m_transform->rotate(rot_amount, m_transform->right());
  if (m_input->keyHold(seng::KeyCode::eDown))
    m_transform->rotate(-rot_amount, m_transform->right());
  if (m_input->keyHold(seng::KeyCode::eLeft))
    m_transform->rotate(rot_amount, Transform::worldUp());
  if (m_input->keyHold(seng::KeyCode::eRight))
    m_transform->rotate(-rot_amount, Transform::worldUp());
}

void TestController::recenter()
{
  m_transform->position(initial_pos);
  m_transform->rotation(initial_rot);
}
