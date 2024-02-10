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
  std::string model, body, wheelL, wheelR;
  bool enabled = true;
  float accel = DEFAULT_ACCEL;
  float breaking = DEFAULT_BREAK;
  float decel = DEFAULT_DECEL;
  float turn = DEFAULT_TURN_RATE;
  float maxSpeed = DEFAULT_MAX_SPEED;
  float maxPitch = DEFAULT_BODY_PITCH;
  float maxRoll = DEFAULT_BODY_ROLL;
  float maxYaw = DEFAULT_WHEEL_YAW;

  model = node["model_entity"].as<std::string>();
  body = node["body_entity"].as<std::string>();
  wheelL = node["wheel_left_entity"].as<std::string>();
  wheelR = node["wheel_right_entity"].as<std::string>();
  if (node["enabled"]) enabled = node["enabled"].as<bool>();
  if (node["acceleration"]) accel = node["acceleration"].as<float>();
  if (node["breaking"]) breaking = node["breaking"].as<float>();
  if (node["deceleration"]) decel = node["deceleration"].as<float>();
  if (node["turn_rate"]) turn = node["turn_rate"].as<float>();
  if (node["max_speed"]) maxSpeed = node["max_speed"].as<float>();

  if (node["max_body_pitch_deg"])
    maxPitch = glm::radians(node["max_body_pitch_deg"].as<float>());
  if (node["max_body_roll_deg"])
    maxRoll = glm::radians(node["max_body_roll_deg"].as<float>());
  if (node["max_wheel_yaw_deg"])
    maxYaw = glm::radians(node["max_wheel_yaw_deg"].as<float>());

  return std::make_unique<CarController>(entity, model, body, wheelL, wheelR, accel,
                                         breaking, decel, turn, maxSpeed, maxPitch,
                                         maxRoll, maxYaw, enabled);
}

CarController::CarController(seng::Entity &entity,
                             const std::string &model,
                             const std::string &body,
                             const std::string &wheelLeft,
                             const std::string &wheelRight,
                             float acceleration,
                             float breaking,
                             float deceleration,
                             float turnRate,
                             float maxSpeed,
                             float maxPitch,
                             float maxRoll,
                             float maxYaw,
                             bool enabled) :
    ScriptComponent(entity, enabled)
{
  m_modelName = model;
  m_bodyName = body;
  m_wheelLeftName = wheelLeft;
  m_wheelRightName = wheelRight;

  m_accel = acceleration;
  m_breaking = breaking;
  m_decel = deceleration;
  m_turnRate = turnRate;
  m_maxSpeed = maxSpeed;
  m_maxSpeed2 = m_maxSpeed * m_maxSpeed;
  m_maxBodyPitch = maxPitch;
  m_maxBodyRoll = maxRoll;
  m_maxWheelYaw = maxYaw;
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

  it = entity->scene().findByName(m_wheelLeftName);
  if (it == entity->scene().entities().end())
    throw std::runtime_error("No entity named " + m_wheelLeftName + " can be found");
  m_wheelLeft = it->transform();

  it = entity->scene().findByName(m_wheelRightName);
  if (it == entity->scene().entities().end())
    throw std::runtime_error("No entity named " + m_wheelRightName + " can be found");
  m_wheelRight = it->transform();
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
  float dot = glm::dot(m_velocity, m_body->forward());
  if (input->keyHold(seng::KeyCode::eKeyW)) {
    if (dot < 0.0f) {
      // We are breaking while going in reverse
      targetAccel = m_breaking;
    } else {
      // We are accelerating forwards
      targetAccel = m_accel;
    }
    targetPitch = -m_maxBodyPitch;
  } else if (input->keyHold(seng::KeyCode::eKeyS)) {
    if (dot > 0.0f) {
      // We are braking while going forwaards
      targetAccel = -m_breaking;
    } else {
      // We are accelerating in reverse
      targetAccel = -m_accel;
    }
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
  float targetYaw;

  // First calculate the murrent angular velocity and set the target roll
  if (input->keyHold(seng::KeyCode::eKeyA)) {
    targetAngular = -m_turnRate;
    targetRoll = m_maxBodyRoll;
    targetYaw = -m_maxWheelYaw;
  } else if (input->keyHold(seng::KeyCode::eKeyD)) {
    targetAngular = m_turnRate;
    targetRoll = -m_maxBodyRoll;
    targetYaw = m_maxWheelYaw;
  } else {
    targetAngular = 0.0f;
    targetRoll = 0.0f;
    targetYaw = 0.0f;
  }

  m_wheelYaw = smoothDamp(m_wheelYaw, targetYaw, m_yawVelocity, 0.1f, delta);
  m_wheelLeft->rotation(glm::vec3(m_wheelLeft->pitch(), m_wheelYaw, m_wheelLeft->roll()));
  m_wheelRight->rotation(
      glm::vec3(m_wheelRight->pitch(), m_wheelYaw, m_wheelRight->roll()));

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

  // Check if we are going in reverse
  bool reverse = false;
  float unsignedAngle = seng::unsignedAngle(m_velocity, m_model->forward());
  if (unsignedAngle > glm::pi<float>() / 2) reverse = true;

  // Calculate the lateral acceleration
  float angVel = reverse ? -m_angularVelocity : m_angularVelocity;
  glm::vec3 accel = glm::cross(glm::vec3(0.0f, angVel, 0.0f), m_velocity);
  m_velocity += accel * delta;

  // If a non-zero angular speed was requested, rotate the model in new direction
  // of motion
  if (targetAngular != 0.0f) {
    // Depending on the direction of motion, pick the axis for angle calulation
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
