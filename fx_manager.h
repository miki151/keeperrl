#pragma once

#include "fx_base.h"

// TODO: this is only for optional
#include "util.h"

namespace fx {

class FXManager {
public:
  FXManager();
  ~FXManager();

  FXManager(const FXManager &) = delete;
  void operator=(const FXManager &) = delete;

  // TODO: better way to communicate with FXManager ?
  static FXManager *getInstance();

  // Animations will look correct even when FPS is low
  // The downside is that more simulation steps are required
  //
  // TODO: make sure that it works correctly with animation slowdown or pause
  void simulateStableTime(double time, int desiredFps = 60);
  void simulateStable(double timeDelta, int desiredFps = 60);
  void simulate(float timeDelta);

  const auto &particleDefs() const { return m_particleDefs; }
  const auto &emitterDefs() const { return m_emitterDefs; }
  const auto &systemDefs() const { return m_systemDefs; }

  optional<ParticleSystemDefId> findSystem(const char *) const;

  bool valid(ParticleDefId) const;
  bool valid(EmitterDefId) const;
  bool valid(ParticleSystemDefId) const;

  const ParticleDef &operator[](ParticleDefId) const;
  const EmitterDef &operator[](EmitterDefId) const;
  const ParticleSystemDef &operator[](ParticleSystemDefId) const;

  bool valid(ParticleSystemId) const;
  bool alive(ParticleSystemId) const;
  bool dead(ParticleSystemId) const; // invalid ids will be dead
  void kill(ParticleSystemId);

  // id cannot be invalid
  ParticleSystem &get(ParticleSystemId);
  const ParticleSystem &get(ParticleSystemId) const;

  ParticleSystemId addSystem(ParticleSystemDefId, FVec2 pos);
  ParticleSystemId addSystem(ParticleSystemDefId, FVec2 pos, FVec2 targetOff);

  ParticleDefId addDef(ParticleDef);
  EmitterDefId addDef(EmitterDef);
  ParticleSystemDefId addDef(ParticleSystemDef);

  vector<ParticleSystemId> aliveSystems() const;
  const auto &systems() const { return m_systems; }
  auto &systems() { return m_systems; }

  vector<DrawParticle> genQuads();

private:
  void addDefaultDefs();
  void simulate(ParticleSystem &, float timeDelta);
  void initialize(const ParticleSystemDef &, ParticleSystem &);
  SubSystemContext ssctx(ParticleSystem &, int);

  vector<ParticleDef> m_particleDefs;
  vector<EmitterDef> m_emitterDefs;
  vector<ParticleSystemDef> m_systemDefs;

  // TODO: add simple statistics: num particles, instances, etc.
  vector<ParticleSystem> m_systems;
  int m_spawnClock = 0, m_randomSeed = 0;
  double m_accumFrameTime = 0.0f;
  double m_oldTime = -1.0;
};
}
