#pragma once

#include "fx_base.h"
#include "fx_curve.h"
#include "fx_emission_source.h"

namespace fx {

struct TextureDef {
  void validate() const {
    CHECK(fileName != nullptr);
  }

  const char* fileName = nullptr;
  IVec2 tiles = {1, 1};
  BlendMode blendMode = BlendMode::normal;
};

// Defines behaviour and looks of a single particle.
struct ParticleDef {
  // Defines spawned particle life in seconds for given AT
  Curve<float> life = 1.0f;

  // These curves modify particle appearance based on
  // particle time
  Curve<float> alpha = 1.0f;
  Curve<float> size = 1.0f;
  Curve<float> slowdown;
  Curve<FVec3> color = FVec3(1.0f);

  // Additional curves, which may be used for complex particles
  vector<Curve<float>> scalarCurves;
  vector<Curve<FVec3>> colorCurves;

  // TODO: option to select subRect for randomization
  TextureName textureName = TextureName(0);
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
using DrawParticleFunc = bool (*)(DrawContext&, const Particle&, DrawParticle&);
using DrawParticlesFunc = void (*)(DrawContext&, const Particle&, vector<DrawParticle>&, Color);

// Returns number of particles to emit
// Fractionals will be accumulated over time
using PrepareEmissionFunc = float (*)(AnimationContext&, EmissionState&);
using EmitParticleFunc = function<void(AnimationContext&, EmissionState&, Particle&)>;

void defaultAnimateParticle(AnimationContext&, Particle&);
float defaultPrepareEmission(AnimationContext&, EmissionState&);
void defaultEmitParticle(AnimationContext&, EmissionState&, Particle&);
bool defaultDrawParticle(DrawContext&, const Particle&, DrawParticle&);

struct SubSystemDef {
  SubSystemDef(ParticleDef pdef, EmitterDef edef, float estart, float eend)
      : particle(pdef), emitter(edef), emissionStart(estart), emissionEnd(eend) {}

  ParticleDef particle;
  EmitterDef emitter;

  float emissionStart, emissionEnd;

  AnimateParticleFunc animateFunc = defaultAnimateParticle;
  PrepareEmissionFunc prepareFunc = defaultPrepareEmission;
  EmitParticleFunc emitFunc = defaultEmitParticle;
  DrawParticleFunc drawFunc = defaultDrawParticle;
  DrawParticlesFunc multiDrawFunc = nullptr;

  int maxActiveParticles = INT_MAX;
  int maxTotalParticles = INT_MAX; // TODO: how should we treat it in looped animations?
  Layer layer = Layer::front;
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
  bool randomOffset = false;
  bool isLooped = false;
};
}
