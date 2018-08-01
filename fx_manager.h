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

  void saveDefs() const;

  // Animations will look correct even when FPS is low
  // The downside is that more simulation steps are required
  //
  // TODO: make sure that it works correctly with animation slowdown or pause
  void simulateStableTime(double time, int desired_fps = 60);
  void simulateStable(double time_delta, int desired_fps = 60);
  void simulate(float time_delta);

  const auto &particleDefs() const { return m_particle_defs; }
  const auto &emitterDefs() const { return m_emitter_defs; }
  const auto &systemDefs() const { return m_system_defs; }

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
  ParticleSystemId addSystem(ParticleSystemDefId, FVec2 pos, FVec2 target_off);

  ParticleDefId addDef(ParticleDef);
  EmitterDefId addDef(EmitterDef);
  ParticleSystemDefId addDef(ParticleSystemDef);

  vector<ParticleSystemId> aliveSystems() const;
  const auto &systems() const { return m_systems; }
  auto &systems() { return m_systems; }

  vector<DrawParticle> genQuads();

private:
  void addDefaultDefs();
  void simulate(ParticleSystem &, float time_delta);
  void initialize(const ParticleSystemDef &, ParticleSystem &);
  SubSystemContext ssctx(ParticleSystem &, int);

  vector<ParticleDef> m_particle_defs;
  vector<EmitterDef> m_emitter_defs;
  vector<ParticleSystemDef> m_system_defs;

  // TODO: add simple statistics: num particles, instances, etc.
  vector<ParticleSystem> m_systems;
  int m_spawn_clock = 0, m_random_seed = 0;
  double m_accum_frame_time = 0.0f;
  double m_old_time = -1.0;
};
}
