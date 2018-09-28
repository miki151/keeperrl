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

void DrawBuffers::add(const vector<DrawParticle>& particles) {
  if (particles.empty())
    return;

  auto& first = particles.front();
  elements.emplace_back(Element{0, 0, first.texName});

  for (auto& quad : particles) {
    auto* last = &elements.back();
    if (last->texName != quad.texName) {
      Element new_elem{(int)positions.size(), 0, quad.texName};
      elements.emplace_back(new_elem);
      last = &elements.back();
    }
    last->numVertices += 4;

    for (int n = 0; n < 4; n++) {
      positions.emplace_back(quad.positions[n]);
      texCoords.emplace_back(quad.texCoords[n]);
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
      colors.emplace_back(ivalue);
    }
  }
}
}
