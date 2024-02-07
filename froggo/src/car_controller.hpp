#pragma once

#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>

class CarController : public seng::ScriptComponent,
                      public seng::ConfigParsableComponent<CarController> {
 public:
  static constexpr float DEFAULT_ACCEL = 7.0f;
  static constexpr float DEFAULT_DECEL = 0.0f;
  static constexpr float DEFAULT_TURN_RATE = 0.2f;
  static constexpr float DEFAULT_MAX_SPEED = 50.0f;

  CarController(seng::Entity &entity,
                const std::string &model,
                float acceleration = DEFAULT_ACCEL,
                float deceleration = DEFAULT_DECEL,
                float turnRate = DEFAULT_TURN_RATE,
                float maxSpeed = DEFAULT_MAX_SPEED,
                bool enabled = true);
  CarController(const CarController &) = delete;
  CarController(CarController &&) = delete;

  CarController &operator=(const CarController &) = delete;
  CarController &operator=(CarController &&) = delete;

  DECLARE_COMPONENT_ID("CarController");
  DECLARE_CREATE_FROM_CONFIG();

  void lateInit() override;
  void onUpdate(float deltaTime) override;

  float speed() const;
  float maxSpeed() const { return m_maxSpeed; }

 private:
  std::string m_modelName;
  seng::Transform *m_model;

  float m_accel;
  float m_decel;
  float m_turnRate;
  float m_maxSpeed;
  float m_maxSpeed2;

  glm::vec3 m_velocity = glm::vec3(0.0f);
  glm::vec3 m_dampVelocities = glm::vec3{0.0f};

  float m_currentAccel = 0;
  float m_dampAccel = 0;

  float m_angularVelocity = 0.0f;
  float m_dampAngular = 0.0f;

  void accelerate(float delta);
  void steer(float delta);

  friend class Gizmo;
};

REGISTER_TO_CONFIG_FACTORY(CarController);
