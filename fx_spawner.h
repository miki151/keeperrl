#pragma once

#include "fx_particle_system.h"

RICH_ENUM(SpawnerType, single, repeated);

namespace fx {

class FXManager;

struct Spawner {
  using Type = SpawnerType;
  using Params = ParticleSystem::Params;

  static FVec2 tileCenter(IVec2 tile_pos) { return FVec2(tile_pos) + FVec2(0.5f); }

  Spawner(Type type, IVec2 tile_pos, ParticleSystemDefId id) : Spawner(type, tile_pos, FVec2(), id) {}
  Spawner(Type, IVec2 tile_pos, FVec2 target_offset, ParticleSystemDefId);

  void update(FXManager &);
  void kill(FXManager &);

  IVec2 tile_pos;
  FVec2 pos, target_offset;

  ParticleSystemDefId def_id;
  ParticleSystemId instance_id;
  int spawn_count = 0;
  bool is_dead = false;
  Type type;

  Params params;
};
}
