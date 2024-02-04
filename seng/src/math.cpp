#include <seng/math.hpp>

#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

float seng::smoothDamp(float current,
                       float target,
                       float &currentVelocity,
                       float smoothTime,
                       float deltaTime,
                       float maxSpeed)
{
  glm::vec1 ref(currentVelocity);
  auto x = seng::smoothDamp(glm::vec1(current), glm::vec1(target), ref, smoothTime,
                            deltaTime, maxSpeed);
  currentVelocity = ref.x;
  return x.x;
}

float seng::repeat(float t, float length)
{
  // Taken directly from the Unity reference source code
  return std::clamp(t - std::floor(t / length) * length, 0.0f, length);
}

float seng::deltaAngle(float oldf, float newf)
{
  // Taken directly from the Unity reference source code
  float delta = seng::repeat(newf - oldf, 2 * glm::pi<float>());
  if (delta > glm::pi<float>()) delta -= 2 * glm::pi<float>();
  return delta;
}

float seng::dampAngle(float oldf, float newf, float delta, float lambda)
{
  newf = oldf + deltaAngle(oldf, newf);
  return oldf * exp(-lambda * delta) + newf * (1 - exp(-lambda * delta));
}
