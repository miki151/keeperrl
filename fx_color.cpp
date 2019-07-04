#include "fx_color.h"

#include "renderer.h"

namespace fx {

FColor::FColor(Color col) { *this = FColor(col.r, col.g, col.b, col.a) * (1.0f / 255.0f); }

FColor::operator Color() const {
  using u8 = SDL::Uint8;
  return {(u8)clamp(r * 255.0f, 0.0f, 255.0f), (u8)clamp(g * 255.0f, 0.0f, 255.0f), (u8)clamp(b * 255.0f, 0.0f, 255.0f),
          (u8)clamp(a * 255.0f, 0.0f, 255.0f)};
}

}
