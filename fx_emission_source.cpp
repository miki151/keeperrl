#include "fx_emission_source.h"

#include "fx_rect.h"

namespace fx {

EmissionSource::EmissionSource(FVec2 pos) : m_pos(pos), m_type(Type::point) {}
EmissionSource::EmissionSource(FRect rect) : m_pos(rect.center()), m_param(rect.size() * 0.5f), m_type(Type::rect) {}
EmissionSource::EmissionSource(FVec2 pos, float radius) : m_pos(pos), m_param(radius, 0.0f), m_type(Type::sphere) {}

FRect EmissionSource::rect() const {
  PASSERT(m_type == Type::rect);
  return {m_pos - m_param, m_pos + m_param};
}
float EmissionSource::sphereRadius() const {
  PASSERT(m_type == Type::sphere);
  return m_param.x;
}

FVec2 EmissionSource::sample(RandomGen &rand) const {
  // TODO: random get float as well
  switch(m_type) {
  case Type::point:
    return m_pos;
  case Type::rect:
    return m_pos + FVec2(rand.getDouble(-m_param.x, m_param.x), rand.getDouble(-m_param.x, m_param.x));
  case Type::sphere: {
    FVec2 spoint(rand.getDouble(-1.0f, 1.0f), rand.getDouble(-1.0f, 1.0f));
    while(spoint.x * spoint.x + spoint.y * spoint.y > 1.0f)
      spoint = FVec2(rand.getDouble(-1.0f, 1.0f), rand.getDouble(-1.0f, 1.0f));
    return m_pos + spoint * m_param.x;
  }
  }

  return {};
}
}
