#pragma once

#include "fx_base.h"

namespace fx {

struct DrawBuffers {
  struct Element {
    int firstVertex;
    int numVertices;
    int particleDefId;
    BlendMode blendMode;
  };

  void fill(const vector<DrawParticle>&);

  vector<FVec2> positions;
  vector<FVec2> texCoords;
  vector<unsigned int> colors;
  vector<Element> elements;
};
}
