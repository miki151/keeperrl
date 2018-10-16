#include "fx_curve.h"

namespace fx {

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
  // auto a0 = y3 - y2 - y0 + y1;
  // auto a1 = y0 - y1 - a0;
  // auto a2 = y2 - y0;
  // auto a3 = y1;

  auto a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
  auto a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
  auto a2 = -0.5f * y0 + 0.5f * y2;
  auto a3 = y1;

  return a0 * mu * mu_sq + a1 * mu_sq + a2 * mu + a3;
}

template <class T> Curve<T>::~Curve() = default;
template <class T> Curve<T>::Curve(T value) : Curve({value}, InterpType::linear) {}

template <class T> Curve<T>::Curve(vector<T> values, InterpType interp) : interp(interp) {
  vector<float> keys(values.size());
  for (int n = 0; n < (int)keys.size(); n++)
    keys[n] = float(n) / float(keys.size() - 1);
  initialize(keys, values);
}

template <class T> Curve<T>::Curve(vector<float> keys, vector<T> values, InterpType interp) : interp(interp) {
  DASSERT(keys.size() == values.size());
  for (int n = 0; n < (int)keys.size(); n++) {
    DASSERT(keys[n] >= 0.0f && keys[n] <= 1.0f);
    if (n > 0)
      DASSERT(keys[n] > keys[n - 1]);
  }
  if (keys[0] != 0.0f) {
    keys.insert(keys.begin(), 0.0f);
    values.insert(values.begin(), values[0]);
  }
  if (keys.back() != 1.0f) {
    keys.emplace_back(1.0f);
    values.emplace_back(values.back());
  }
  initialize(keys, values);
}

template <class T> void Curve<T>::initialize(vector<float>& tkeys, vector<T>& tvalues) {
  DASSERT(tkeys.size() <= maxKeys - 2);
  DASSERT(tkeys.size() >= 1);

  num_keys = tkeys.size();
  for (int n = 0; n < num_keys; n++) {
    keys[n] = tkeys[n];
    values[n] = tvalues[n];
  }
  values[num_keys] = values[num_keys + 1] = values[num_keys - 1];
  for (int n = 0; n < num_keys - 1; n++)
    scale[n] = 1.0f / (keys[n + 1] - keys[n]);
  scale[num_keys - 1] = 0.0f;
}

template <class T> T Curve<T>::sample(float position) const {
  PASSERT(position >= 0.0f && position <= 1.0f);
  if (num_keys <= 1)
    return values[0];

  int id = 0;
  while (keys[id] < position)
    id++;
  id = id - 1; // keys[0] is always 0.0f
  float t = (position - keys[id]) * scale[id];

  switch (interp) {
  case InterpType::linear:
    return lerp(values[id], values[id + 1], t);
  case InterpType::cosine:
    return interpCosine(values[id], values[id + 1], t);
  case InterpType::quadratic: {
    int prevId = id == 0 ? 0 : id - 1;
    return interpQuadratic(values[prevId], values[id], values[id + 1], values[id + 2], t);
  }
  case InterpType::cubic: {
    int prevId = id == 0 ? 0 : id - 1;
    return interpCubic(values[prevId], values[id], values[id + 1], values[id + 2], t);
  }
  }
  return T();
}

template <class T> void Curve<T>::print(int num_steps) const {
  /*if constexpr (std::is_same<T, float>()) {
    printf("Values: ");
    for (int n = 0; n < num_keys; n++)
      printf("%f ", values[n]);
    printf("\nKeys: ");
    for (int n = 0; n < num_keys; n++)
      printf("%f ", keys[n]);
    printf("\n");
    for (int n = 0; n < num_steps; n++) {
      float t = float(n) / float(num_steps - 1);
      printf("[%f -> %f] ", t, sample(t));
    }
  }
  printf("\n");*/
}

template struct Curve<float>;
template struct Curve<FVec2>;
template struct Curve<FVec3>;
}
