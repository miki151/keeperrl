#pragma once

#include "fx_emission_source.h"

namespace fx {

// Emiterem mogą też być cząsteczki innego subsystemu ?
struct EmitterDef {
  EmissionSource source;
  Curve<float> frequency; // particles per second
  Curve<float> strengthMin, strengthMax;
  Curve<float> angle = 0.0f, angleSpread = fconstant::pi; // in radians
  Curve<float> rotationSpeedMin, rotationSpeedMax;

  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;

  float initialSpawnCount = 0.0f;

  // TODO: zamiast częstotliwości możemy mieć docelową ilość cząsteczek
  // (danego rodzaju?) w danym momencie
  // TODO: całkowanie niektórych krzywych?

  string name;
};
}
