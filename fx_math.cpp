#include "fx_math.h"
#include <algorithm>

namespace fx {

std::pair<float, float> sincos(float radians) {
  std::pair<float, float> out;
  ::sincosf(radians, &out.first, &out.second);
  return out;
}

float angleDistance(float a, float b) {
  float diff = fabsf(a - b);
  return std::min(diff, fconstant::pi * 2.0f - diff);
}

float blendAngles(float initial, float target, float step) {
  if(initial != target) {
    float new_ang1 = initial + step, new_ang2 = initial - step;
    if(new_ang1 < 0.0f)
      new_ang1 += fconstant::pi * 2.0f;
    if(new_ang2 < 0.0f)
      new_ang2 += fconstant::pi * 2.0f;
    float new_angle = angleDistance(new_ang1, target) < angleDistance(new_ang2, target) ? new_ang1 : new_ang2;
    if(angleDistance(initial, target) < step)
      new_angle = target;
    return new_angle;
  }

  return initial;
}

float normalizeAngle(float angle) {
  angle = fmodf(angle, 2.0f * fconstant::pi);
  if(angle < 0.0f)
    angle += 2.0f * fconstant::pi;
  return angle;
}
}
