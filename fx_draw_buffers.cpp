#include "fx_draw_buffers.h"

#include "fx_vec.h"
#include "fx_particle_system.h"

namespace fx {

void DrawBuffers::clear() {
  positions.clear();
  texCoords.clear();
  colors.clear();
  elements.clear();
}

void DrawBuffers::add(const DrawParticle* particles, int count) {
  if (count == 0 || !particles)
    return;

  PROFILE;
  auto& first = particles[0];
  if (elements.empty())
    elements.emplace_back(Element{0, 0, first.texName});

  // TODO: sort particles by texture ?
  for (int n = 0; n < count; n++) {
    auto& quad = particles[n];
    auto* last = &elements.back();
    if (last->texName != quad.texName) {
      Element new_elem{(int)positions.size(), 0, quad.texName};
      elements.emplace_back(new_elem);
      last = &elements.back();
    }
    last->numVertices += 4;

    positions.insert(positions.end(), begin(quad.positions), end(quad.positions));
    texCoords.insert(texCoords.end(), begin(quad.texCoords), end(quad.texCoords));

    union {
      struct {
        unsigned char r, g, b, a;
      } channels;
      unsigned int ivalue;
      };

      channels.r = quad.color.r;
      channels.g = quad.color.g;
      channels.b = quad.color.b;
      channels.a = quad.color.a;
      colors.resize(colors.size() + 4, ivalue);
  }
}
}
