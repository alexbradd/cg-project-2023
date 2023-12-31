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

#include <memory>

using namespace seng;

std::unique_ptr<components::BaseComponent> TestController::createFromConfig(
    scene::Entity &entity, const YAML::Node &node)
{
  bool enabled = true;

  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  return std::make_unique<TestController>(entity, enabled);
}

void TestController::scriptInitialize()
{
  m_transform = entity->transform().get();
  m_input = entity->application().input().get();
}

void TestController::onUpdate(float deltaTime)
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
    m_transform->rotate(glm::radians(1.0f), m_transform->right());
  else if (m_input->keyHold(seng::KeyCode::eDown))
    m_transform->rotate(glm::radians(-1.0f), m_transform->right());
  else if (m_input->keyHold(seng::KeyCode::eLeft))
    m_transform->rotate(glm::radians(1.0f), m_transform->up());
  else if (m_input->keyHold(seng::KeyCode::eRight))
    m_transform->rotate(glm::radians(-1.0f), m_transform->up());

  if (m_input->keyDown(seng::KeyCode::eEsc)) entity->application().stop();
}
