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
  switch (type) {
  case Type::point:
    return pos;
  case Type::rect: {
    auto f2 = rand.getFloat2Fast();
    return pos + FVec2((f2.first * 2.0f - 1.0f) * param.x, (f2.second * 2.0f - 1.0f) * param.y);
  }
  case Type::sphere: {
    FVec2 spoint(rand.getFloatFast(-1.0f, 1.0f), rand.getFloatFast(-1.0f, 1.0f));
    while(spoint.x * spoint.x + spoint.y * spoint.y > 1.0f)
      spoint = FVec2(rand.getFloatFast(-1.0f, 1.0f), rand.getFloatFast(-1.0f, 1.0f));
    return pos + spoint * param.x;
  }
  }

  return {};
}
}
