#pragma once

#include "fx_base.h"
#include "fx_curve.h"

namespace fx {

struct ParticleDef {
  // Defines spawned particle life in seconds for given AT
  Curve<float> life;

  // These curves modify particle appearance based on
  // particle time
  Curve<float> alpha;
  Curve<float> size;
  Curve<float> slowdown;
  Curve<FVec3> color;

  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;

  IVec2 textureTiles = IVec2(1, 1);
  string textureName;
  string name;
  BlendMode blendMode = BlendMode::normal;
};
}
