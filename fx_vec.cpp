#include "fx_vec.h"

namespace fx {

float vectorToAngle(const FVec2 &normalized_vec) {
  float ang = std::acos(normalized_vec.x);
  return normalized_vec.y < 0 ? 2.0f * fconstant::pi - ang : ang;
}

FVec2 angleToVector(float radians) {
  auto sc = sincos(radians);
  return FVec2(sc.second, sc.first);
}

FVec2 rotateVector(const FVec2 &vec, float radians) {
  auto sc = sincos(radians);
  return FVec2(sc.second * vec.x - sc.first * vec.y, sc.second * vec.y + sc.first * vec.x);
}
}
