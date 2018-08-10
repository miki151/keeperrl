#pragma once

#include "fx_base.h"
#include "fx_color.h"
#include "fx_tag_id.h"
#include "util.h"
#include <limits.h>

namespace fx {

// Identifies a particluar particle system instance
class ParticleSystemId {
public:
  ParticleSystemId() : m_index(-1) {}
  ParticleSystemId(int index, int spawnTime) : m_index(index), m_spawnTime(spawnTime) {}

  bool validIndex() const { return m_index >= 0; }
  explicit operator bool() const { return validIndex(); }

  operator int() const { return m_index; }
  int index() const { return m_index; }
  int spawnTime() const { return m_spawnTime; }

private:
  int m_index;
  int m_spawnTime;
};

using AnimateParticleFunc = void (*)(AnimationContext &, Particle &);
using DrawParticleFunc = void (*)(DrawContext &, const Particle &, DrawParticle &);

// Returns number of particles to emit
// Fractionals will be accumulated over time
using PrepareEmissionFunc = float (*)(AnimationContext &, EmissionState &);
using EmitParticleFunc = void (*)(AnimationContext &, EmissionState &, Particle &);

void defaultAnimateParticle(AnimationContext &, Particle &);
float defaultPrepareEmission(AnimationContext &, EmissionState &);
void defaultEmitParticle(AnimationContext &, EmissionState &, Particle &);
void defaultDrawParticle(DrawContext &, const Particle &, DrawParticle &);

// Defines behaviour of a whole particle system
struct ParticleSystemDef {
  // TODO: is it really useful to separate particle, emitter & system definitions?
  // maybe let's just merge it into single structure? We're not reusing anything anyways...

  struct SubSystem {
    SubSystem(ParticleDefId pdef, EmitterDefId edef, float estart, float eend)
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

  const SubSystem &operator[](int ssid) const { return subSystems[ssid]; }
  SubSystem &operator[](int ssid) { return subSystems[ssid]; }

  vector<SubSystem> subSystems;
  optional<float> animLength;
  bool isLooped = false;
  string name;
};

struct Particle {
  float particleTime() const { return life / maxLife; }

  // TODO: quantize members ? It may give the particles a pixelated feel
  FVec2 pos, movement, size = FVec2(1.0);
  float life = 0.0f, maxLife = 1.0f;
  float rot = 0.0f, rotSpeed = 0.0f;
  SVec2 texTile;
  int randomSeed;
};

struct DrawParticle {
  // TODO: compress it somehow?
  std::array<FVec2, 4> positions;
  std::array<FVec2, 4> texCoords;
  IColor color;
  int particleDefId;
  BlendMode blendMode;
};

struct ParticleSystem {
  struct SubSystem {
    vector<Particle> particles;
    float emissionFract = 0.0f;
    int randomSeed = 123;
    int totalParticles = 0;
  };

  struct Params {
    static constexpr int maxScalars = 2, maxColors = 2, maxDirs = 2;

    float scalar[maxScalars] = {0.0f, 0.0f};
    FVec3 color[maxColors] = {FVec3(1.0), FVec3(1.0)};
    Dir dir[maxDirs] = {Dir::N, Dir::N};
  };

  ParticleSystem(FVec2 pos, FVec2 targetOff, ParticleSystemDefId, int spawnTime, int numSubSystems);

  int numActiveParticles() const;
  int numTotalParticles() const;

  void kill(bool immediate);

  const SubSystem &operator[](int ssid) const { return subSystems[ssid]; }
  SubSystem &operator[](int ssid) { return subSystems[ssid]; }

  vector<SubSystem> subSystems;
  Params params;
  FVec2 pos, targetOff;

  ParticleSystemDefId defId;
  int spawnTime;

  float animTime = 0.0f;
  bool isDead = false;
  bool isDying = false;
};

struct SubSystemContext {
  SubSystemContext(const ParticleSystem &ps, const ParticleSystemDef &, const ParticleDef &, const EmitterDef &,
                   int ssid);

  const ParticleSystem &ps;
  const ParticleSystem::SubSystem &ss;

  const ParticleSystemDef &psdef;
  const ParticleSystemDef::SubSystem &ssdef;

  const ParticleDef &pdef;
  const EmitterDef &edef;

  const int ssid;
};

struct AnimationContext : public SubSystemContext {
  AnimationContext(const SubSystemContext &, float animTime, float timeDelta);

  float uniformSpread(float spread);
  float uniform(float min, float max);
  int randomSeed();
  SVec2 randomTexTile();

  RandomGen rand;
  const float animTime;
  const float timeDelta, invTimeDelta;
};

struct DrawContext : public SubSystemContext {
  DrawContext(const SubSystemContext &, FVec2);

  FVec2 invTexTile;

  array<FVec2, 4> quadCorners(FVec2 pos, FVec2 size, float rotation) const;
  array<FVec2, 4> texQuadCorners(SVec2 texTile) const;
};

struct EmissionState {
  EmissionState(float time) : time(time) {}
  const float time;

  float maxLife;
  float strength, strengthSpread;
  float direction, directionSpread;
  float rotSpeed, rotSpeedSpread;

  float var[128];
};
}
