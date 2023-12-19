#pragma once

#include <chrono>

namespace seng {

/**
 * Type alias for the clock.
 */
using Clock = std::chrono::steady_clock;

/**
 * Type alias for a Clock timestamp (e.g. as returned by Clock::now()).
 */
using Timestamp =
    std::chrono::time_point<Clock, std::chrono::duration<Clock::rep, Clock::period>>;

/**
 * Type alias for a duration taken based calculated using Clock timestamps.
 */
using Duration = std::chrono::duration<Clock::rep, Clock::period>;

/**
 * Convert the given Duration into seconds.
 */
inline float inSeconds(const Duration &d)
{
  return d.count() / static_cast<float>(Clock::period::den);
}

};  // namespace seng
