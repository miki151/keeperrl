#pragma once

#include "fx_base.h"
#include "fx_curve.h"

namespace fx {

struct ParticleDef {
  void serialize(IArchive &, unsigned int);
  void serialize(OArchive &, unsigned int) const;

  // Defines spawned particle life in seconds for given AT
  Curve<float> life;

  // These curves modify particle appearance based on
  // particle time
  Curve<float> alpha;
  Curve<float> size;
  Curve<float> slowdown;
  Curve<FVec3> color;

  vector<Curve<float>> scalar_curves;
  vector<Curve<FVec3>> color_curves;

  IVec2 texture_tiles = IVec2(1, 1);
  string texture_name;
  string name;
};
}
