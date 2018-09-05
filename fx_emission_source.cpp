#include "fx_emission_source.h"

#include "fx_rect.h"

namespace fx {

EmissionSource::EmissionSource(FVec2 pos) : pos(pos), type(Type::point) {}
EmissionSource::EmissionSource(FRect rect) : pos(rect.center()), param(rect.size() * 0.5f), type(Type::rect) {}
EmissionSource::EmissionSource(FVec2 pos, float radius) : pos(pos), param(radius, 0.0f), type(Type::sphere) {}

FRect EmissionSource::rect() const {
  PASSERT(type == Type::rect);
  return {pos - param, pos + param};
}
float EmissionSource::sphereRadius() const {
  PASSERT(type == Type::sphere);
  return param.x;
}

FVec2 EmissionSource::sample(RandomGen &rand) const {
  // TODO: random get float as well
  switch (type) {
  case Type::point:
    return pos;
  case Type::rect:
    return pos + FVec2(rand.getDouble(-param.x, param.x), rand.getDouble(-param.y, param.y));
  case Type::sphere: {
    FVec2 spoint(rand.getDouble(-1.0f, 1.0f), rand.getDouble(-1.0f, 1.0f));
    while(spoint.x * spoint.x + spoint.y * spoint.y > 1.0f)
      spoint = FVec2(rand.getDouble(-1.0f, 1.0f), rand.getDouble(-1.0f, 1.0f));
    return pos + spoint * param.x;
  }
  }

  return {};
}
}
