#pragma once

#include "fx_base.h"
#include "fx_curve.h"

namespace fx {

struct ParticleDef {
  // Defines spawned particle life in seconds for given AT
  Curve<float> life = 1.0f;

  // These curves modify particle appearance based on
  // particle time
  Curve<float> alpha = 1.0f;
  Curve<float> size = 1.0f;
  Curve<float> slowdown;
  Curve<FVec3> color = FVec3(1.0f);

  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;

  IVec2 textureTiles = IVec2(1, 1);
  string textureName;
  BlendMode blendMode = BlendMode::normal;
};
}
