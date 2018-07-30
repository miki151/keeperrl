#pragma once

#include "fx_particle_system.h"

RICH_ENUM(SpawnerType, single, repeated);

namespace fx {

class FXManager;

struct Spawner {
  using Type = SpawnerType;
  using Params = ParticleSystem::Params;

  // TODO: dodatkowe parametry
  Spawner(Type type, IVec2 tile_pos, ParticleSystemDefId);

  void update(FXManager &);
  void kill(FXManager &);

  IVec2 tile_pos;
  ParticleSystemDefId def_id;
  ParticleSystemId instance_id;
  int spawn_count = 0;
  bool is_dead = false;
  Type type;

  Params params;
};
}
