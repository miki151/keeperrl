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

  EmissionSource() : m_type(Type::point) {}
  EmissionSource(FVec2 pos);
  EmissionSource(FRect rect);
  EmissionSource(FVec2 pos, float radius);

  Type type() const { return m_type; }
  FVec2 pos() const { return m_pos; }
  FRect rect() const;
  float sphereRadius() const;

  // TODO(opt): sample multiple points at once
  FVec2 sample(RandomGen &) const;

private:
  FVec2 m_pos, m_param;
  Type m_type;
};
}
