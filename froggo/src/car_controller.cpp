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
  std::string model, body;
  bool enabled = true;
  float accel = DEFAULT_ACCEL;
  float decel = DEFAULT_DECEL;
  float turn = DEFAULT_TURN_RATE;
  float maxSpeed = DEFAULT_MAX_SPEED;
  float maxPitch = DEFAULT_BODY_PITCH;
  float maxRoll = DEFAULT_BODY_ROLL;

  model = node["model_entity"].as<std::string>();
  body = node["body_entity"].as<std::string>();
  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  if (node["acceleration"]) accel = node["acceleration"].as<float>();
  if (node["deceleration"]) decel = node["deceleration"].as<float>();
  if (node["turn_rate"]) turn = node["turn_rate"].as<float>();
  if (node["max_speed"]) maxSpeed = node["max_speed"].as<float>();
  if (node["max_pitch_deg"]) maxPitch = glm::radians(node["max_pitch_deg"].as<float>());
  if (node["max_roll_deg"]) maxRoll = glm::radians(node["max_roll_deg"].as<float>());

  return std::make_unique<CarController>(entity, model, body, accel, decel, turn,
                                         maxSpeed, maxPitch, maxRoll, enabled);
}

CarController::CarController(seng::Entity &entity,
                             const std::string &model,
                             const std::string &body,
                             float acceleration,
                             float deceleration,
                             float turnRate,
                             float maxSpeed,
                             float maxPitch,
                             float maxRoll,
                             bool enabled) :
    ScriptComponent(entity, enabled)
{
  m_modelName = model;
  m_bodyName = body;
  m_accel = acceleration;
  m_decel = deceleration;
  m_turnRate = turnRate;
  m_maxSpeed = maxSpeed;
  m_maxSpeed2 = m_maxSpeed * m_maxSpeed;
  m_maxBodyPitch = maxPitch;
  m_maxBodyRoll = maxRoll;
}

void CarController::lateInit()
{
  auto it = entity->scene().findByName(m_modelName);
  if (it == entity->scene().entities().end())
    throw std::runtime_error("No entity named " + m_modelName + " can be found");
  m_model = it->transform();

  it = entity->scene().findByName(m_bodyName);
  if (it == entity->scene().entities().end())
    throw std::runtime_error("No entity named " + m_bodyName + " can be found");
  m_body = it->transform();
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

  float targetAccel;
  float targetPitch;

  // Depending on input, calculate the target acceleration and model pitch
  if (input->keyHold(seng::KeyCode::eKeyW)) {
    targetAccel = m_accel;
    targetPitch = -m_maxBodyPitch;
  } else if (input->keyHold(seng::KeyCode::eKeyS)) {
    targetAccel = -m_accel;
    targetPitch = m_maxBodyPitch;
  } else {
    targetAccel = 0;
    targetPitch = 0.0f;
  }

  // Update the current acceleration moving it towards the target
  m_currentAccel = smoothDamp(m_currentAccel, targetAccel, m_dampAccel, 0.1f, delta);

  // If a non-zero acceleration is wanted, change the velocity based on that,
  // otherwise apply an "intertial" deceleration
  if (targetAccel != 0.0f) {
    m_velocity += m_model->forward() * targetAccel * delta;
  } else {
    float len2 = glm::length2(m_velocity);
    if (len2 > 0) {
      float scale = 1 - glm::exp(-len2);
      scale = glm::clamp(scale, 0.0f, 1.0f);
      m_velocity -= glm::normalize(m_velocity) * scale * m_decel * delta;
    }
  }

  m_bodyPitch = smoothDamp(m_bodyPitch, targetPitch, m_pitchVelocity, 0.1f, delta);
  m_body->rotation(glm::vec3(m_bodyPitch, m_body->yaw(), m_body->roll()));
}

void CarController::steer(float delta)
{
  auto &input = entity->application().input();

  float targetAngular;
  float targetRoll;

  // First calculate the murrent angular velocity and set the target roll
  if (input->keyHold(seng::KeyCode::eKeyA)) {
    targetAngular = -m_turnRate;
    targetRoll = m_maxBodyRoll;
  } else if (input->keyHold(seng::KeyCode::eKeyD)) {
    targetAngular = m_turnRate;
    targetRoll = -m_maxBodyRoll;
  } else {
    targetAngular = 0.0f;
    targetRoll = 0.0f;
  }

  // Scaling w.r.t. speed for regulating turning intensity
  // (higher speed -> higher turning)
  float speed2 = glm::length2(m_velocity);
  float turnScaling = 1.0f - glm::exp(-0.01 * speed2);

  // Update the current angular velocity
  m_angularVelocity = smoothDamp(m_angularVelocity, turnScaling * targetAngular,
                                 m_dampAngular, 0.01, delta);

  // Update the model roll
  m_bodyRoll =
      smoothDamp(m_bodyRoll, turnScaling * targetRoll, m_rollVelocity, 0.1f, delta);
  m_body->rotation(glm::vec3(m_body->pitch(), m_body->yaw(), m_bodyRoll));

  // If going too slow, simply abort acceleration calcualtions to avoid dealing
  // with infinitesimal values
  if (speed2 < 5.0f) return;
  if (glm::abs(m_angularVelocity) < .1f) return;

  // Calculate the lateral acceleration
  glm::vec3 accel = glm::cross(glm::vec3(0.0f, m_angularVelocity, 0.0f), m_velocity);
  m_velocity += accel * delta;

  // If a non-zero angular speed was requested, rotate the model in new direction
  // of motion
  if (targetAngular != 0.0f) {
    // Check if we are going in reverse
    bool reverse = false;
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
