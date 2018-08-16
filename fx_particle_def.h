#pragma once

#include "fx_base.h"
#include "fx_curve.h"

namespace fx {

struct TextureDef {
  TextureDef(const char* name = "circular.png") : name(name), tiles(1, 1) { }
  TextureDef(string name) : name(name), tiles(1, 1) { }
  TextureDef(string name, int xTiles, int yTiles) : name(name), tiles(xTiles, yTiles) { }

  // TODO: option to select subRect for randomization
  string name;
  IVec2 tiles;
};

// Defines behavious and looks of a single particle.
struct ParticleDef {
  // Defines spawned particle life in seconds for given AT
  Curve<float> life = 1.0f;

  // These curves modify particle appearance based on
  // particle time
  Curve<float> alpha = 1.0f;
  Curve<float> size = 1.0f;
  Curve<float> slowdown;
  Curve<FVec3> color = FVec3(1.0f);

  TextureDef texture;
  BlendMode blendMode = BlendMode::normal;

  // Additional curves, which may be used for complex particles
  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;
};
}
