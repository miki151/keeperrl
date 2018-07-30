#pragma once

#ifdef FWK_TESTING_MODE

#include <fwk/format.h>
#include <fwk_vector.h>

#undef CHECK

namespace fx {
template <class T> using vector = fwk::vector<T>; // for additional checks
using fwk::print;
}

#else

#include "debug.h"
#include <vector>

namespace fx {
template <class T> using vector = std::vector<T>;
}

#ifndef DASSERT // It can only be defined in fx_tester
#define DASSERT CHECK
#define ASSERT CHECK
#define PASSERT CHECK
#endif

#endif

namespace cereal {
template <class T> class NameValuePair;
class JSONOutputArchive;
class JSONInputArchive;
}

namespace fx {

template <class T> using NVP = cereal::NameValuePair<T>;
using IArchive = cereal::JSONInputArchive;
using OArchive = cereal::JSONOutputArchive;

static constexpr int default_tile_size = 24;

class FXManager;

template <class T> class Rect;
template <class T> struct vec2;
template <class T> struct vec3;

using SVec2 = vec2<short>;
using SVec3 = vec3<short>;
using IVec2 = vec2<int>;
using IVec3 = vec3<int>;
using FVec2 = vec2<float>;
using FVec3 = vec3<float>;

using IRect = Rect<IVec2>;
using FRect = Rect<FVec2>;

struct FColor;
struct IColor;

template <class T> struct Curve;

struct DrawParticle;
struct Particle;
struct ParticleSystem;

struct ParticleDef;
struct EmitterDef;
struct ParticleSystemDef;

struct SubSystemContext;
struct DrawContext;
struct AnimationContext;
struct EmissionState;

template <class T, class Base = unsigned> class TagId;

using ParticleDefId = TagId<ParticleDef>;
using EmitterDefId = TagId<EmitterDef>;
using ParticleSystemDefId = TagId<ParticleSystemDef>;
class ParticleSystemId;
}
