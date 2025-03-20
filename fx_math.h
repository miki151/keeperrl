#pragma once

#include <cmath>
#include <limits>
#include <utility>

#ifdef OSX
void sincosf(float a, float* sin, float* cos);
#endif


namespace fx {

struct NoAssertsTag {};

// TODO: rename
namespace fconstant {
  static constexpr const float pi = 3.14159265358979323846f;
  static constexpr const float e = 2.7182818284590452354f;
  static constexpr const float epsilon = 0.0001f;
  static constexpr const float isect_epsilon = 0.000000001f;
  static const float inf = std::numeric_limits<float>::infinity();
}

inline double inv(double s) { return 1.0 / s; }
inline float inv(float s) { return 1.0f / s; }

inline float degToRad(float v) { return v * (2.0f * fconstant::pi / 360.0f); }
inline float radToDeg(float v) { return v * (360.0 / (2.0 * fconstant::pi)); }

inline std::pair<float, float> sincos(float radians) {
#ifndef WINDOWS
  std::pair<float, float> out;
  ::sincosf(radians, &out.first, &out.second);
  return out;
#else
  std::pair<float, float> out;
  out.first = std::sin(radians);
  out.second = std::cos(radians);
  return out;
#endif
}

// Return angle in range (0; 2 * PI)
float normalizeAngle(float radians);

template <class Obj, class Scalar> inline Obj lerp(const Obj &a, const Obj &b, const Scalar &x) {
  return (b - a) * x + a;
}

template <class T> T clamp(const T &obj, const T &tmin, const T &tmax) { return min(tmax, max(tmin, obj)); }

//template <class T1, class T2> bool operator!=(const T1 &a, const T2 &b) { return !(a == b); }

float angleDistance(float a, float b);
float blendAngles(float initial, float target, float step);

// Returns angle in range <0, 2 * PI)
float normalizeAngle(float angle);
}
