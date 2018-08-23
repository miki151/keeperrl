#pragma once

#include "fx_particle_system.h"

RICH_ENUM(SpawnerType, single, repeated);

namespace fx {

class FXManager;

struct Spawner {
  using Type = SpawnerType;
  using Params = ParticleSystem::Params;

  static FVec2 tileCenter(IVec2 tilePos) { return FVec2(tilePos) + FVec2(0.5f); }

  Spawner(Type type, IVec2 tilePos, FXName id) : Spawner(type, tilePos, FVec2(), id) {}
  Spawner(Type, IVec2 tilePos, FVec2 targetOff, FXName);

  void update(FXManager &);
  void kill(FXManager &);

  IVec2 tilePos;
  FVec2 pos, targetOff;

  FXName defId;
  ParticleSystemId instanceId;
  int spawnCount = 0;
  bool isDead = false;
  Type type;

  Params params;
};
}
