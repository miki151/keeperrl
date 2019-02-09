#pragma once

#include "fx_base.h"

namespace fx {

struct DrawBuffers {
  struct Element {
    int firstVertex;
    int numVertices;
    TextureName texName;
  };

  void clear();
  // TODO: span would be useful
  void add(const DrawParticle*, int count);
  bool empty() const {
    return elements.empty();
  }

  vector<FVec2> positions;
  vector<FVec2> texCoords;
  vector<unsigned int> colors;

  // TODO: group elements by TextureName
  vector<Element> elements;
};
}
