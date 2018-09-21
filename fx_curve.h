#pragma once

#include "fx_vec.h"
#include "util.h"

// TODO: fix RICH_ENUM so that it will work with namespaces ?
RICH_ENUM(InterpType, linear, cosine, quadratic, cubic);

namespace fx {

// TODO: move these functions inside curve.cpp ?

template <class T> T interpCosine(const T &a, const T &b, float x) {
  return lerp(a, b, (1.0f - std::cos(x * fconstant::pi)) * 0.5f);
}

template <class T> T interpQuadratic(const T &v0, const T &v1, const T &v2, const T &v3, float x) {
  // TODO: to chyba nie jest poprawne
  return v1 * (1 - x) + v2 * x + (v2 - v3 + v1 - v0) * (1 - x) * x;
}

template <class T> T interpCubic(const T &y0, const T &y1, const T &y2, const T &y3, float mu) {
  // Source: http://paulbourke.net/miscellaneous/
  auto mu_sq = mu * mu;
  //	auto a0 = y3 - y2 - y0 + y1;
  //	auto a1 = y0 - y1 - a0;
  //	auto a2 = y2 - y0;
  //	auto a3 = y1;

  auto a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
  auto a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
  auto a2 = -0.5f * y0 + 0.5f * y2;
  auto a3 = y1;

  return a0 * mu * mu_sq + a1 * mu_sq + a2 * mu + a3;
}

template <class T> struct Curve {
public:
  Curve(vector<float> keys, vector<T> values, InterpType = InterpType::linear);
  Curve(vector<T> values, InterpType = InterpType::linear); // regular keys
  Curve(T value);
  Curve();
  ~Curve();

  // Position is always within range: <0, 1>
  T sample(float position) const;

  void print(float step = 0.05f) const;

  // TODO: keys can be optional, then we're treating it as a regular curve
  vector<float> keys;
  vector<T> values;
  InterpType interp; // TODO: this doesn't have to be here
};

extern template struct Curve<float>;
extern template struct Curve<FVec2>;
extern template struct Curve<FVec3>;
}
