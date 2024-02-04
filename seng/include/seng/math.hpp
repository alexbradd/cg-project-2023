#pragma once

#include <glm/detail/qualifier.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <limits>

namespace seng {

/// Gradually changes a value towards a desired goal over time. Never overshoots
/// target
template <int L, typename T, glm::qualifier Q>
glm::vec<L, T, Q> smoothDamp(glm::vec<L, T, Q> current,
                             glm::vec<L, T, Q> target,
                             glm::vec<L, T, Q> &currentVelocity,
                             float smoothTime,
                             float deltaTime,
                             float maxSpeed = std::numeric_limits<float>::infinity())
{
  using Vec = glm::vec<L, T, Q>;

  // Adapted from the Unity reference source code
  smoothTime = std::max(0.0001f, smoothTime);
  float omega = 2.0f / smoothTime;

  float x = omega * deltaTime;
  float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

  Vec change = current - target;
  Vec originalTo = target;

  // Clamp maximum speed
  float maxChange = maxSpeed * smoothTime;
  float maxChange2 = maxChange * maxChange;
  float len2 = glm::length2(change);
  if (len2 > maxChange2) {
    float len = glm::sqrt(len2);
    change = change / len * maxChange;
  }

  target = current - change;

  Vec temp = (currentVelocity + omega * change) * deltaTime;
  currentVelocity = (currentVelocity - omega * temp) * exp;
  Vec output = target + (change + temp) * exp;

  Vec origCurrent = originalTo - current;
  Vec outOrigin = output - originalTo;

  // Prevent overshooting
  if (glm::dot(origCurrent, outOrigin) > 0.0f) {
    output = originalTo;
    currentVelocity = (output - originalTo) / deltaTime;
  }

  return output;
}

/// Interpolates between `a` and `b` by `t`. `t` is clamped between 0 and 1.
template <int L, typename T, glm::qualifier Q>
glm::vec<L, T, Q> lerp(glm::vec<L, T, Q> a, glm::vec<L, T, Q> b, float t)
{
  return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
}

/// Interpolates between `a` and `b` by `t`.
template <int L, typename T, glm::qualifier Q>
glm::vec<L, T, Q> lerpUnclamped(glm::vec<L, T, Q> a, glm::vec<L, T, Q> b, float t)
{
  return a + (b - a) * t;
}

/// Return +1 if the number is positive, otherwise -1
template <typename T>
T sign(T n)
{
  if (n < 0) return -1;
  return 1;
}

/// Returns the smallest angle in radians between the two vectors
template <int L, typename T, glm::qualifier Q>
T unsignedAngle(glm::vec<L, T, Q> a, glm::vec<L, T, Q> b)
{
  float cos = glm::dot(a, b) / glm::sqrt(glm::length2(a) * glm::length2(b));
  cos = glm::clamp(cos, -1.0f, 1.0f);
  return glm::acos(cos);
}

/**
 * Returns the smallest angle in radians between the two vectors, therefore it
 * is always [-pi; pi]. If one imagines the two vectors on a plane (e.g. a piece
 * of paper), the axis vector would be the vector pointing up out of the paper.
 * A positive angle represents a clockwise direction, while a negative one
 * a counter-clockwise direction
 */
template <int L, typename T, glm::qualifier Q>
T signedAngle(glm::vec<L, T, Q> a, glm::vec<L, T, Q> b, glm::vec<L, T, Q> axis)
{
  float u = unsignedAngle(a, b);
  return sign(glm::dot(glm::cross(a, b), axis)) * u;
}

/// Gradually changes a value towards a desired goal over time. Never overshoots
/// target
float smoothDamp(float current,
                 float target,
                 float &currentVelocity,
                 float smoothTime,
                 float deltaTime,
                 float maxSpeed = std::numeric_limits<float>::infinity());

/// Loops value <t> so that it is never larger than <length> or smaller than 0
float repeat(float t, float length);

/// Calculates the shortest difference between two angles
float deltaAngle(float oldf, float newf);

/// Smoothly interpolates betweeen two given angles
float dampAngle(float oldf, float newf, float delta, float lambda = 15);

}  // namespace seng
