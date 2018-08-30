#pragma once

#include "fx_base.h"
#include "util.h"
#include "fx_simple.h"
#include "fx_particle_system.h"
#include "fx_defs.h"
#include "fx_name.h"

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

  bool valid(ParticleDefId) const;
  bool valid(EmitterDefId) const;

  const ParticleDef &operator[](ParticleDefId) const;
  const EmitterDef &operator[](EmitterDefId) const;
  const ParticleSystemDef& operator[](FXName) const;

  bool valid(ParticleSystemId) const;
  bool alive(ParticleSystemId) const;
  bool dead(ParticleSystemId) const; // invalid ids will be dead
  void kill(ParticleSystemId, bool immediate);

  // id cannot be invalid
  ParticleSystem &get(ParticleSystemId);
  const ParticleSystem &get(ParticleSystemId) const;

  ParticleSystemId addSystem(FXName, InitConfig);

  vector<ParticleSystemId> aliveSystems() const;
  const auto &systems() const { return m_systems; }
  auto &systems() { return m_systems; }

  vector<DrawParticle> genQuads();

  // These can be used only during initialization
  ParticleDefId addDef(ParticleDef);
  EmitterDefId addDef(EmitterDef);
  void addDef(FXName, ParticleSystemDef);

  using Snapshot = vector<ParticleSystem::SubSystem>;
  struct SnapshotGroup {
    SnapshotKey key;
    vector<Snapshot> snapshots;
  };

  void addSnapshot(float animTime, const ParticleSystem&);
  const SnapshotGroup* findSnapshotGroup(FXName, SnapshotKey) const;
  void genSnapshots(FXName, vector<float>, vector<float> params = {}, int randomVariants = 1);

  private:
  ParticleSystem makeSystem(FXName, uint spawnTime, InitConfig);
  void initializeDefs();

  void simulate(ParticleSystem &, float timeDelta);
  SubSystemContext ssctx(ParticleSystem &, int);

  vector<ParticleDef> m_particleDefs;
  vector<EmitterDef> m_emitterDefs;
  EnumMap<FXName, ParticleSystemDef> m_systemDefs;
  EnumMap<FXName, vector<SnapshotGroup>> m_snapshotGroups;

  // TODO: add simple statistics: num particles, instances, etc.
  vector<ParticleSystem> m_systems;
  unique_ptr<RandomGen> m_randomGen;
  uint m_spawnClock = 1;
  double m_accumFrameTime = 0.0f;
  double m_oldTime = -1.0;
};
}
