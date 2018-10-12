#include "fx_math.h"
#include <algorithm>

namespace fx {

float angleDistance(float a, float b) {
  float diff = fabsf(a - b);
  return std::min(diff, fconstant::pi * 2.0f - diff);
}

float blendAngles(float initial, float target, float step) {
  if (initial != target) {
    float newAng1 = initial + step, newAng2 = initial - step;
    if (newAng1 < 0.0f)
      newAng1 += fconstant::pi * 2.0f;
    if (newAng2 < 0.0f)
      newAng2 += fconstant::pi * 2.0f;
    float newAngle = angleDistance(newAng1, target) < angleDistance(newAng2, target) ? newAng1 : newAng2;
    if (angleDistance(initial, target) < step)
      newAngle = target;
    return newAngle;
  }

  return initial;
}

float normalizeAngle(float angle) {
  angle = fmodf(angle, 2.0f * fconstant::pi);
  if (angle < 0.0f)
    angle += 2.0f * fconstant::pi;
  return angle;
}
}
