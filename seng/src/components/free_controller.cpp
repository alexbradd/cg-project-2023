#include <seng/application.hpp>
#include <seng/components/free_controller.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_manager.hpp>
#include <seng/scene/entity.hpp>

#include <yaml-cpp/yaml.h>
#include <glm/gtx/norm.hpp>

using namespace seng;

DEFINE_CREATE_FROM_CONFIG(FreeController, entity, node)
{
  bool enabled = true;
  float move = FreeController::DEFAULT_MOVE_SPEED;
  float rot = FreeController::DEFAULT_ROTATION_SPEED;

  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  if (node["moveSpeed"]) move = node["moveSpeed"].as<float>();
  if (node["rotationSpeed"]) rot = node["rotationSpeed"].as<float>();
  return std::make_unique<FreeController>(entity, move, rot, enabled);
}

FreeController::FreeController(seng::Entity &entity,
                               float moveSpeed,
                               float rotSpeed,
                               bool enabled) :
    ScriptComponent(entity, enabled)
{
  m_initialPos = entity.transform()->position();
  m_initialRot = entity.transform()->quaternion();

  m_speed = moveSpeed;
  m_rot = rotSpeed;
}

void FreeController::onUpdate(float delta)
{
  handleRotation(delta);
  handleMovement(delta);
}

void FreeController::handleMovement(float delta)
{
  glm::vec3 moveDirection(0.0f);
  auto transform = entity->transform();
  auto &input = entity->application().input();

  if (input->keyHold(seng::KeyCode::eKeyA)) moveDirection += -transform->right();
  if (input->keyHold(seng::KeyCode::eKeyD)) moveDirection += transform->right();
  if (input->keyHold(seng::KeyCode::eKeyW)) moveDirection += transform->forward();
  if (input->keyHold(seng::KeyCode::eKeyS)) moveDirection += -transform->forward();
  if (input->keyHold(seng::KeyCode::eSpace)) {
    if (input->keyHold(seng::KeyCode::eModLeftShift))
      moveDirection += -transform->up();
    else
      moveDirection += transform->up();
  }

  if (glm::length2(moveDirection) > 0.0) moveDirection = glm::normalize(moveDirection);

  transform->translate(moveDirection * m_speed * delta);
}

void FreeController::handleRotation(float delta)
{
  float rot_amount = glm::radians(m_rot * delta);
  auto &input = entity->application().input();
  auto transform = entity->transform();

  if (input->keyHold(seng::KeyCode::eDown))
    transform->rotate(rot_amount, transform->right());
  if (input->keyHold(seng::KeyCode::eUp))
    transform->rotate(-rot_amount, transform->right());
  if (input->keyHold(seng::KeyCode::eRight))
    transform->rotate(rot_amount, Transform::worldUp());
  if (input->keyHold(seng::KeyCode::eLeft))
    transform->rotate(-rot_amount, Transform::worldUp());
}
