#pragma once

#include "fx_emission_source.h"

namespace fx {

// Defines behaviour of a particle system emitter.
//
//         frequency: how many particles are spawned per second
//         direction: defines angle (in radians) or direction in which particles are emitted;
//                    if angleSpread is big enough, base angle makes no difference
//          strength: emission strength; affects particle movement and rotation speed
//          rotSpeed: initial rotation speed; 50% of particles will rotate the other way
// initialSpawnCount: how many particles are spawned at time 0
//                    fractional value will speed up following emissions
//
//           *Spread: adds random(-spread, spread) to base value
//                    should be >= 0
//           *Curves: additional curves which may be used for complex FXes
//
// TODO: use vectors instead of angles ?
struct EmitterDef {
  EmissionSource source;
  Curve<float> frequency;

  Curve<float> strength, strengthSpread;
  Curve<float> direction, directionSpread = fconstant::pi;
  Curve<float> rotSpeed, rotSpeedSpread;
  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;
  float initialSpawnCount = 0.0f;

  void setStrengthSpread(Curve<float> base, Curve<float> spread) {
    strength = std::move(base);
    strengthSpread = std::move(spread);
  }
  void setRotSpeedSpread(Curve<float> base, Curve<float> spread) {
    rotSpeed = std::move(base);
    rotSpeedSpread = std::move(spread);
  }
  void setDirectionSpread(Curve<float> base, Curve<float> spread) {
    direction = std::move(base);
    directionSpread = std::move(spread);
  }
};
}
