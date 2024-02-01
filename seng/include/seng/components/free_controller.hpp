#pragma once

#include <seng/components/definitions.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>

#include <glm/ext/quaternion_float.hpp>

namespace seng {

/**
 * Simple ScriptComponent implementing a free-flying controller. Ideal for a
 * free-flying camera.
 */
class FreeController : public seng::ScriptComponent,
                       public seng::ConfigParsableComponent<FreeController> {
 public:
  static constexpr float DEFAULT_MOVE_SPEED = 7.0f;
  static constexpr float DEFAULT_ROTATION_SPEED = 70.0f;

  FreeController(seng::Entity &entity,
                 float moveSpeed = DEFAULT_MOVE_SPEED,
                 float rotSpeed = DEFAULT_ROTATION_SPEED,
                 bool enabled = true);
  FreeController(const FreeController &) = delete;
  FreeController(FreeController &&) = delete;

  FreeController &operator=(const FreeController &) = delete;
  FreeController &operator=(FreeController &&) = delete;

  DECLARE_COMPONENT_ID("FreeController");
  DECLARE_CREATE_FROM_CONFIG();

  void onUpdate(float deltaTime) override;

 private:
  glm::vec3 m_initialPos;
  glm::quat m_initialRot;

  float m_speed;
  float m_rot;

  void handleMovement(float delta);
  void handleRotation(float delta);
};

REGISTER_TO_CONFIG_FACTORY(FreeController);

}  // namespace seng
