#pragma once

#include "fx_base.h"
#include "util.h"
#include "fx_particle_system.h"
#include "fx_defs.h"
#include "fx_name.h"
#include "fx_texture_name.h"

namespace fx {

class FXManager {
public:
  FXManager();
  ~FXManager();

  FXManager(const FXManager &) = delete;
  void operator=(const FXManager &) = delete;

  // TODO: better way to communicate with FXManager ?
  static FXManager *getInstance();

  // TODO: make sure that it works correctly with animation slowdown or pause
  void simulateStableTime(double time, int visibleFps = 60, int simulateFps = 60);

  // Animations will look correct even when FPS is low
  // The downside is that more simulation steps are required
  void simulateStable(double timeDelta, int visibleFps = 60, int simulateFps = 60);
  void simulate(float timeDelta);

  const auto& getTextureDefs() const { return textureDefs; }
  const auto& getSystemDefs() const { return systemDefs; }

  const ParticleSystemDef& operator[](FXName) const;
  const TextureDef& operator[](TextureName) const;

  bool valid(ParticleSystemId) const;
  bool alive(ParticleSystemId) const;
  bool dead(ParticleSystemId) const; // invalid ids will be dead
  void kill(ParticleSystemId, bool immediate);

  void clearUnorderedEffects();

  // TODO: this interface may be too fancy...
  // id cannot be invalid
  ParticleSystem &get(ParticleSystemId);
  const ParticleSystem &get(ParticleSystemId) const;

  ParticleSystemId addSystem(FXName, InitConfig);

  const auto& getSystems() const { return systems; }
  auto& getSystems() { return systems; }
  void genQuads(vector<DrawParticle>&, int id, int ssid);

  using Snapshot = vector<ParticleSystem::SubSystem>;
  struct SnapshotGroup {
    SnapshotKey key;
    vector<Snapshot> snapshots;
  };

  void addSnapshot(float animTime, const ParticleSystem&);
  const SnapshotGroup* findSnapshotGroup(FXName, SnapshotKey) const;
  void genSnapshots(FXName, vector<float>, vector<float> params = {}, int randomVariants = 1);

  void addDef(FXName, ParticleSystemDef);

  private:
  ParticleSystem makeSystem(FXName, uint spawnTime, InitConfig);

  // Implemented in fx_factory.cpp:
  void initializeDefs();
  void initializeTextureDefs();
  void initializeTextureDef(TextureName, TextureDef&);

  void simulate(ParticleSystem &, float timeDelta);
  SubSystemContext ssctx(ParticleSystem &, int);

  EnumMap<FXName, ParticleSystemDef> systemDefs;
  EnumMap<FXName, vector<SnapshotGroup>> snapshotGroups;
  EnumMap<TextureName, TextureDef> textureDefs;

  // TODO: add simple statistics: num particles, instances, etc.
  vector<ParticleSystem> systems;
  unique_ptr<RandomGen> randomGen;
  uint spawnClock = 1;
  double accumFrameTime = 0.0f;
  double oldTime = -1.0;
  double globalSimTime = 0.0;
};
}
