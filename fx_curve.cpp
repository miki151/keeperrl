#include "fx_curve.h"

namespace fx {

// TODO: there is a lot of potential for optimization

template <class T> Curve<T>::Curve() = default;
template <class T> Curve<T>::~Curve() = default;

template <class T> Curve<T>::Curve(T value) : keys({0.0f}), values({value}), interp(InterpType::linear) {}

template <class T>
Curve<T>::Curve(vector<T> tvalues, InterpType interp) : values(std::move(tvalues)), interp(interp) {
  keys.resize(values.size());
  for (int n = 0; n < (int)keys.size(); n++)
    keys[n] = float(n) / float(keys.size() - 1);
}

template <class T>
Curve<T>::Curve(vector<float> tkeys, vector<T> tvalues, InterpType interp)
    : keys(std::move(tkeys)), values(std::move(tvalues)), interp(interp) {
  DASSERT(keys.size() == values.size());
  for (int n = 0; n < (int)keys.size(); n++) {
    DASSERT(keys[n] >= 0.0f && keys[n] <= 1.0f);
    if (n > 0)
      DASSERT(keys[n] >= keys[n - 1]);
  }
}

// TODO: add sampleLooped
template <class T> T Curve<T>::sample(float position) const {
  PASSERT(position >= 0.0f && position <= 1.0f);
  if (values.size() <= 1)
    return values.empty() ? T() : values.front();

  int id = 0;
  while (id < (int)keys.size() && keys[id] < position)
    id++;
  id = id == 0 ? 0 : id - 1;

  int ids[4] = {id == 0 ? 0 : id - 1, id, id + 1, id + 2};

  if (ids[2] == (int)keys.size())
    ids[2] = ids[3] = id;
  else if (ids[3] == (int)keys.size())
    ids[3] = id + 1;

  float key1 = keys[ids[1]], key2 = keys[ids[2]];
  float t = key1 == key2 ? 0.0 : (position - key1) / (key2 - key1);

  switch (interp) {
  case InterpType::linear:
    return lerp(values[ids[1]], values[ids[2]], t);
  case InterpType::cosine:
    return interpCosine(values[ids[1]], values[ids[2]], t);
  case InterpType::quadratic:
    return interpQuadratic(values[ids[0]], values[ids[1]], values[ids[2]], values[ids[3]], t);
  case InterpType::cubic:
    return interpCubic(values[ids[0]], values[ids[1]], values[ids[2]], values[ids[3]], t);
  }

  return T();
}

template <class T> void Curve<T>::print(float step) const {
  /*if constexpr(std::is_same<T, float>())
    for(float t = 0.0f; t <= 1.0f; t += step)
      printf("[%f -> %f] ", t, sample(t));
  printf("\n");*/
}

template struct Curve<float>;
template struct Curve<FVec2>;
template struct Curve<FVec3>;
}
