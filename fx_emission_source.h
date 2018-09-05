#pragma once

#include "fx_base.h"
#include "fx_curve.h"
#include "fx_vec.h"
#include "util.h"

RICH_ENUM(EmissionSourceType, point, rect, sphere);

namespace fx {

class EmissionSource {
  public:
  using Type = EmissionSourceType;

  EmissionSource() : type(Type::point) {}
  EmissionSource(FVec2 pos);
  EmissionSource(FRect rect);
  EmissionSource(FVec2 pos, float radius);

  Type getType() const { return type; }
  FVec2 getPos() const { return pos; }
  FRect rect() const;
  float sphereRadius() const;

  // TODO(opt): sample multiple points at once
  FVec2 sample(RandomGen &) const;

  private:
  FVec2 pos, param;
  Type type;
};
}
