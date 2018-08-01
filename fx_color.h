#pragma once

#include "fx_vec.h"

struct Color;

namespace fx {

struct FColor {
  FColor() : r(0.0), g(0.0), b(0.0), a(1.0) {}
  FColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
  FColor(const FColor &col, float a) : r(col.r), g(col.g), b(col.b), a(a) {}
  FColor(const FVec3 &rgb, float a = 1.0) : r(rgb[0]), g(rgb[1]), b(rgb[2]), a(a) {}

  FColor(IColor);
  FColor(Color);
  explicit operator Color() const;
  explicit operator IColor() const;

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

// TODO: IColor is very similar to ::Color, but it doesn't require including renderer.h and SDL
struct IColor {
  IColor() : IColor(0, 0, 0, 0) {}
  IColor(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {} // TODO: clamp or not ?
  IColor(Color);
  explicit operator Color() const;

  unsigned char r, g, b, a;
};

FColor mulAlpha(FColor color, float alpha);
FColor desaturate(FColor col, float value);
}
