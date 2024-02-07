#include "./car_controller.hpp"

#include <seng/application.hpp>
#include <seng/components/scene_config_component_factory.hpp>
#include <seng/components/script.hpp>
#include <seng/components/transform.hpp>
#include <seng/input_manager.hpp>
#include <seng/math.hpp>
#include <seng/scene/entity.hpp>

#include <yaml-cpp/yaml.h>
#include <glm/exponential.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <string>

using seng::smoothDamp;

// ===== CarController
DEFINE_CREATE_FROM_CONFIG(CarController, entity, node)
{
  std::string model;
  bool enabled = true;
  float accel = DEFAULT_ACCEL;
  float decel = DEFAULT_DECEL;
  float turn = DEFAULT_TURN_RATE;
  float maxSpeed = DEFAULT_MAX_SPEED;

  model = node["model_entity"].as<std::string>();
  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  if (node["acceleration"]) accel = node["acceleration"].as<float>();
  if (node["deceleration"]) decel = node["deceleration"].as<float>();
  if (node["turn_rate"]) turn = node["turn_rate"].as<float>();
  if (node["max_speed"]) maxSpeed = node["max_speed"].as<float>();

  return std::make_unique<CarController>(entity, model, accel, decel, turn, maxSpeed,
                                         enabled);
}

CarController::CarController(seng::Entity &entity,
                             const std::string &model,
                             float acceleration,
                             float deceleration,
                             float turnRate,
                             float maxSpeed,
                             bool enabled) :
    ScriptComponent(entity, enabled)
{
  m_modelName = model;
  m_accel = acceleration;
  m_decel = deceleration;
  m_turnRate = turnRate;
  m_maxSpeed = maxSpeed;
  m_maxSpeed2 = m_maxSpeed * m_maxSpeed;
}

void CarController::lateInit()
{
  auto it = entity->scene().findByName(m_modelName);
  if (it == entity->scene().entities().end())
    throw std::runtime_error("No entity named " + m_modelName + " can be found");
  m_model = it->transform();
}

float CarController::speed() const
{
  return glm::length(m_velocity);
}

void CarController::onUpdate(float delta)
{
  accelerate(delta);
  steer(delta);

  float speed2 = glm::length2(m_velocity);
  if (speed2 > m_maxSpeed2) {
    float speed = glm::sqrt(speed2);
    m_velocity = m_velocity / speed * m_maxSpeed;
  }

  // prevent some leftover inaccuracies
  if (glm::length(m_velocity) < 2.0f) {
    m_velocity = smoothDamp(m_velocity, glm::vec3(0.0f), m_dampVelocities, 1.f, delta);
  }

  if (glm::length(m_velocity) > 0.01f) entity->transform()->translate(m_velocity * delta);
}

void CarController::accelerate(float delta)
{
  auto &input = entity->application().input();

  float target;
  float dampTime;

  bool key = false;

  if (input->keyHold(seng::KeyCode::eKeyW)) {
    key = true;
    target = m_accel;
    dampTime = 0.1f;
  } else if (input->keyHold(seng::KeyCode::eKeyS)) {
    key = true;
    target = -m_accel;
    dampTime = 0.1f;
  } else {
    target = 0;
    dampTime = 0.1f;
  }
  m_currentAccel = smoothDamp(m_currentAccel, target, m_dampAccel, dampTime, delta);

  if (key) {
    m_velocity += m_model->forward() * target * delta;
  } else {
    float len2 = glm::length2(m_velocity);
    if (len2 > 0) {
      float scale = 1 - glm::exp(-len2);
      scale = glm::clamp(scale, 0.0f, 1.0f);
      m_velocity -= glm::normalize(m_velocity) * scale * m_decel * delta;
    }
  }
}

void CarController::steer(float delta)
{
  auto &input = entity->application().input();

  bool key = false;

  if (input->keyHold(seng::KeyCode::eKeyA)) {
    key = true;
    m_angularVelocity =
        smoothDamp(m_angularVelocity, -m_turnRate, m_dampAngular, 0.01, delta);
  } else if (input->keyHold(seng::KeyCode::eKeyD)) {
    key = true;
    m_angularVelocity =
        smoothDamp(m_angularVelocity, m_turnRate, m_dampAngular, 0.01, delta);
  } else {
    m_angularVelocity = smoothDamp(m_angularVelocity, 0.0f, m_dampAngular, 0.01, delta);
  }

  float speed2 = glm::length2(m_velocity);
  if (glm::abs(m_angularVelocity) < .1f) return;
  if (speed2 < 5.0f) return;

  float turnRateScale = 1.0f - glm::exp(-0.01 * speed2);
  glm::vec3 accel =
      glm::cross(glm::vec3(0.0f, turnRateScale * m_angularVelocity, 0.0f), m_velocity);
  m_velocity += accel * delta;

  if (key) {
    // Rotate the model in new direction of motion
    bool reverse = false;

    // Check if we are going in reverse
    float unsignedAngle = seng::unsignedAngle(m_velocity, m_model->forward());
    if (unsignedAngle > glm::pi<float>() / 2) reverse = true;

    // Depending on the outcome, pick the axis for angle calulation
    glm::vec3 axis =
        reverse ? -seng::Transform::worldForward() : seng::Transform::worldForward();

    // Then rotate accordingly
    float signedAngle = seng::signedAngle(m_velocity, axis, -seng::Transform::worldUp());
    m_model->rotation(glm::vec3(0, signedAngle, 0));
  }
}

// ==== Gizmo
class Gizmo : public seng::ScriptComponent, public seng::ConfigParsableComponent<Gizmo> {
 private:
  CarController *car;
  seng::Transform *carTransform;

 public:
  DECLARE_COMPONENT_ID("Gizmo")
  DECLARE_CREATE_FROM_CONFIG();

  Gizmo(seng::Entity &e, const std::string &name) : ScriptComponent(e)
  {
    auto c = entity->scene().findByName(name);
    carTransform = c->transform();
    car = c->componentsOfType<CarController>()[0].sureGet<CarController>();
  }

  Gizmo(const Gizmo &) = delete;
  Gizmo(Gizmo &&) = delete;

  Gizmo &operator=(const Gizmo &) = delete;
  Gizmo &operator=(Gizmo &&) = delete;

  void onUpdate([[maybe_unused]] float delta) override
  {
    entity->transform()->position(carTransform->position() +
                                  glm::vec3(0.0, entity->transform()->position().y, 0.0));

    glm::vec3 dir;
    if (glm::length(car->m_velocity) > 0)
      dir = glm::normalize(car->m_velocity);
    else
      return;

    float cos =
        glm::dot(dir, seng::Transform::worldForward()) /
        glm::sqrt(glm::length2(dir) * glm::length2(seng::Transform::worldForward()));
    float angle = glm::acos(cos);
    float signedAngle =
        seng::sign(glm::dot(glm::cross(dir, seng::Transform::worldForward()),
                            -seng::Transform::worldUp())) *
        angle;

    entity->transform()->rotation(glm::vec3(0, signedAngle, 0));
  }
};

REGISTER_TO_CONFIG_FACTORY(Gizmo);

DEFINE_CREATE_FROM_CONFIG(Gizmo, e, node)
{
  std::string c = node["car_entity"].as<std::string>();
  return std::make_unique<Gizmo>(e, c);
}
