#include "fx_curve.h"
#include "fx_emitter_def.h"
#include "fx_manager.h"
#include "fx_particle_def.h"
#include "fx_particle_system.h"

#include "cereal/archives/json.hpp"
#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include <iostream>

namespace fx {

namespace {
  template <class T, int vec_size = T::vec_size> vector<float> linearize(const vector<T> &vecs) {
    vector<float> out;
    out.reserve(vecs.size() * vec_size);
    for(auto &v : vecs)
      for(int n = 0; n < vec_size; n++)
        out.emplace_back(v[n]);
    return out;
  }

  template <class T, int vec_size = T::vec_size> vector<T> delinearize(const vector<float> &values) {
    vector<T> out;
    ASSERT(values.size() % vec_size == 0);
    int num_vecs = values.size() / vec_size;
    out.reserve(num_vecs);
    for(int n = 0; n < num_vecs; n++) {
      T vec;
      for(int i = 0; i < vec_size; i++)
        vec[i] = values[n * vec_size + i];
      out.emplace_back(vec);
    }
    return out;
  }

  string stringize(const vector<float> &values) {
    std::stringstream ss;
    for(int n = 0; n < (int)values.size(); n++) {
      ss << values[n];
      if(n + 1 < (int)values.size())
        ss << ' ';
    }
    return ss.str();
  }

  vector<float> destringize(const string &str) {
    vector<float> out;
    std::stringstream ss(str);
    while(true) {
      float v;
      ss >> v;
      if(!ss)
        break;
      out.emplace_back(v);
    }
    return out;
  }
}

template <class T> void Curve<T>::serialize(IArchive &ar, unsigned int) {
  //TODO: writeme
}

template <class T> void Curve<T>::serialize(OArchive &ar, unsigned int) const {
  vector<float> values;

  // TODO: fixme
  /*
  if constexpr(!std::is_same<T, float>::value)
    values = linearize(m_values);
  else
    values = {begin(m_values), end(m_values)};
  ar << cereal::make_nvp("keys", stringize(m_keys));
  ar << cereal::make_nvp("values", stringize(values));
  //	if(m_interp != InterpType::linear)
  //		ar << cereal::make_nvp("interpolation", EnumInfo<InterpType>::getString(m_interp));
  */
}

#define INSTANTIATE(type)                                                                                              \
  template void Curve<type>::serialize(IArchive &, unsigned int);                                                      \
  template void Curve<type>::serialize(OArchive &, unsigned int) const;

INSTANTIATE(float)
INSTANTIATE(FVec2)
INSTANTIATE(FVec3)

#undef INSTANTIATE

void EmitterDef::serialize(IArchive &ar, unsigned int) {}

void EmitterDef::serialize(OArchive &ar, unsigned int) const {
  ar(CEREAL_NVP(frequency), CEREAL_NVP(strength_min), CEREAL_NVP(strength_max));
}

void ParticleDef::serialize(IArchive &ar, unsigned int) {
  ar(CEREAL_NVP(life), CEREAL_NVP(alpha), CEREAL_NVP(size), CEREAL_NVP(slowdown), CEREAL_NVP(color));
}

void ParticleDef::serialize(OArchive &ar, unsigned int) const {
  ar(CEREAL_NVP(life), CEREAL_NVP(alpha), CEREAL_NVP(size), CEREAL_NVP(slowdown), CEREAL_NVP(color));
}

void ParticleSystemDef::serialize(IArchive &ar, unsigned int) {}

void ParticleSystemDef::serialize(OArchive &ar, unsigned int) const {}

void FXManager::saveDefs() const {
  std::ofstream stream("effects.json");
  OArchive ar(stream, OArchive::Options::Default());

  for(int n = 0; n < (int)m_particle_defs.size(); n++) {
    char name[256];
    snprintf(name, sizeof(name), "particle_%d", n);
    ar << cereal::make_nvp(name, m_particle_defs[n]);
  }
  for(int n = 0; n < (int)m_emitter_defs.size(); n++) {
    char name[256];
    snprintf(name, sizeof(name), "emitter_%d", n);
    ar << cereal::make_nvp(name, m_emitter_defs[n]);
  }
}
}
