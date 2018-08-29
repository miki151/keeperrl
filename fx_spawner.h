#pragma once

#include "fx_particle_system.h"

RICH_ENUM(SpawnerType, single, repeated);

namespace fx {

class FXManager;

struct Spawner {
  using Type = SpawnerType;
  using Params = ParticleSystem::Params;

  static FVec2 tileCenter(IVec2 tilePos) { return FVec2(tilePos) + FVec2(0.5f); }

  // TODO: use InitConfig here as well
  Spawner(Type type, IVec2 tilePos, FXName id, SnapshotKey key = {}) : Spawner(type, tilePos, FVec2(), id, key) {}
  Spawner(Type, IVec2 tilePos, FVec2 targetOff, FXName, SnapshotKey = {});

  void update(FXManager &);
  void kill(FXManager &);

  IVec2 tilePos;
  FVec2 pos, targetOff;

  FXName defId;
  ParticleSystemId instanceId;
  int spawnCount = 0;
  bool isDead = false;
  Type type;

  SnapshotKey snapshotKey;
  Params params;
};
}
