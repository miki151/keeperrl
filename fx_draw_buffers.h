#pragma once

#include "fx_base.h"

namespace fx {

struct DrawBuffers {
  struct Element {
    int firstVertex;
    int numVertices;
    TextureName texName;
  };

  void fill(const vector<DrawParticle>&);
  bool empty() const {
    return elements.empty();
  }

  vector<FVec2> positions;
  vector<FVec2> texCoords;
  vector<unsigned int> colors;
  vector<Element> elements;
};
}
