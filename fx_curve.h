#pragma once

#include "fx_vec.h"
#include "util.h"

// TODO: fix RICH_ENUM so that it will work with namespaces ?
RICH_ENUM(InterpType, linear, cosine, quadratic, cubic);

namespace fx {


template <class T> struct Curve {
  public:
  static constexpr int maxKeys = 16;

  Curve(vector<float> keys, vector<T> values, InterpType = InterpType::linear);
  Curve(vector<T> values, InterpType = InterpType::linear); // regular keys
  Curve(T value = T());
  ~Curve();

  // Position is always within range: <0, 1>
  T sample(float position) const;

  void print(int num_steps = 20) const;

  private:
  void initialize(vector<float>&, vector<T>&);

  // TODO: keys can be optional, then we're treating it as a regular curve
  float keys[maxKeys];
  float scale[maxKeys];
  T values[maxKeys];
  int num_keys;
  InterpType interp; // TODO: this doesn't have to be here
};

extern template struct Curve<float>;
extern template struct Curve<FVec2>;
extern template struct Curve<FVec3>;
}
