#pragma once

#include "fx_base.h"
#include "fx_curve.h"
#include "fx_emission_source.h"
#include "fx_tag_id.h"

namespace fx {

struct TextureDef {
  TextureDef(const char* name = "circular.png") : name(name), tiles(1, 1) { }
  TextureDef(string name) : name(name), tiles(1, 1) { }
  TextureDef(string name, int xTiles, int yTiles) : name(name), tiles(xTiles, yTiles) { }

  // TODO: option to select subRect for randomization
  string name;
  IVec2 tiles;
};

// Defines behavious and looks of a single particle.
struct ParticleDef {
  // Defines spawned particle life in seconds for given AT
  Curve<float> life = 1.0f;

  // These curves modify particle appearance based on
  // particle time
  Curve<float> alpha = 1.0f;
  Curve<float> size = 1.0f;
  Curve<float> slowdown;
  Curve<FVec3> color = FVec3(1.0f);

  TextureDef texture;
  BlendMode blendMode = BlendMode::normal;

  // Additional curves, which may be used for complex particles
  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;
};

// Defines behaviour of a particle system emitter.
//
//         frequency: how many particles are spawned per second
//         direction: defines angle (in radians) or direction in which particles are emitted;
//                    if angleSpread is big enough, base angle makes no difference
//          strength: emission strength; affects particle movement and rotation speed
//          rotSpeed: initial rotation speed; 50% of particles will rotate the other way
// initialSpawnCount: how many particles are spawned at time 0
//                    fractional value will speed up following emissions
//
//           *Spread: adds random(-spread, spread) to base value
//                    should be >= 0
//           *Curves: additional curves which may be used for complex FXes
//
// TODO: use vectors instead of angles ?
struct EmitterDef {
  EmissionSource source;
  Curve<float> frequency;

  Curve<float> strength, strengthSpread;
  Curve<float> direction, directionSpread = fconstant::pi;
  Curve<float> rotSpeed, rotSpeedSpread;
  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;
  float initialSpawnCount = 0.0f;

  void setStrengthSpread(Curve<float> base, Curve<float> spread) {
    strength = std::move(base);
    strengthSpread = std::move(spread);
  }
  void setRotSpeedSpread(Curve<float> base, Curve<float> spread) {
    rotSpeed = std::move(base);
    rotSpeedSpread = std::move(spread);
  }
  void setDirectionSpread(Curve<float> base, Curve<float> spread) {
    direction = std::move(base);
    directionSpread = std::move(spread);
  }
};

using AnimateParticleFunc = void (*)(AnimationContext&, Particle&);
using DrawParticleFunc = void (*)(DrawContext&, const Particle&, DrawParticle&);

// Returns number of particles to emit
// Fractionals will be accumulated over time
using PrepareEmissionFunc = float (*)(AnimationContext&, EmissionState&);
using EmitParticleFunc = void (*)(AnimationContext&, EmissionState&, Particle&);

void defaultAnimateParticle(AnimationContext&, Particle&);
float defaultPrepareEmission(AnimationContext&, EmissionState&);
void defaultEmitParticle(AnimationContext&, EmissionState&, Particle&);
void defaultDrawParticle(DrawContext&, const Particle&, DrawParticle&);

struct SubSystemDef {
  SubSystemDef(ParticleDefId pdef, EmitterDefId edef, float estart, float eend)
      : particleId(pdef), emitterId(edef), emissionStart(estart), emissionEnd(eend) {}

  ParticleDefId particleId;
  EmitterDefId emitterId;

  float emissionStart, emissionEnd;

  AnimateParticleFunc animateFunc = defaultAnimateParticle;
  PrepareEmissionFunc prepareFunc = defaultPrepareEmission;
  EmitParticleFunc emitFunc = defaultEmitParticle;
  DrawParticleFunc drawFunc = defaultDrawParticle;

  int maxActiveParticles = INT_MAX;
  int maxTotalParticles = INT_MAX; // TODO: how should we treat it in looped animations?
};

// Defines behaviour of a whole particle system
struct ParticleSystemDef {
  // TODO: is it really useful to separate particle, emitter & system definitions?
  // maybe let's just merge it into single structure? We're not reusing anything anyways...

  explicit operator bool () const { return !subSystems.empty(); }
  const SubSystemDef &operator[](int ssid) const { return subSystems[ssid]; }
  SubSystemDef &operator[](int ssid) { return subSystems[ssid]; }

  vector<SubSystemDef> subSystems;
  optional<float> animLength;
  bool isLooped = false;
};
}
