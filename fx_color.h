#pragma once

#include "fx_vec.h"

struct Color;

namespace fx {

struct FColor {
  FColor() : r(0.0), g(0.0), b(0.0), a(1.0) {}
  FColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
  FColor(const FColor &col, float a) : r(col.r), g(col.g), b(col.b), a(a) {}
  FColor(const FVec3 &rgb, float a = 1.0) : r(rgb[0]), g(rgb[1]), b(rgb[2]), a(a) {}

  FColor(Color);
  explicit operator Color() const;

  FVec3 rgb() const { return {r, g, b}; }

  FColor operator*(float s) const { return FColor(r * s, g * s, b * s, a * s); }
  FColor operator*(const FColor &rhs) const { return FColor(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a); }
  FColor operator-(const FColor &rhs) const { return FColor(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a); }
  FColor operator+(const FColor &rhs) const { return FColor(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a); }

  union {
    struct {
      float r, g, b, a;
    };
    float v[4];
  };
};

FColor mulAlpha(FColor color, float alpha);
FColor desaturate(FColor col, float value);
}
