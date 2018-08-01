#pragma once

#include "fx_emission_source.h"

namespace fx {

// Emiterem mogą też być cząsteczki innego subsystemu ?
struct EmitterDef {
  void serialize(IArchive &, unsigned int);
  void serialize(OArchive &, unsigned int) const;

  EmissionSource source;
  Curve<float> frequency; // particles per second
  Curve<float> strength_min, strength_max;
  Curve<float> angle = 0.0f, angle_spread = fconstant::pi; // in radians
  Curve<float> rotation_speed_min, rotation_speed_max;

  vector<Curve<float>> scalar_curves;
  vector<Curve<FVec3>> color_curves;

  float initial_spawn_count = 0.0f;

  // TODO: zamiast częstotliwości możemy mieć docelową ilość cząsteczek
  // (danego rodzaju?) w danym momencie
  // TODO: całkowanie niektórych krzywych?

  string name;
};
}
