#include "fx_spawner.h"

#include "fx_manager.h"

namespace fx {

Spawner::Spawner(Type type, IVec2 tile_pos, ParticleSystemDefId def_id)
    :  tile_pos(tile_pos), def_id(def_id), type(type) {}

void Spawner::update(FXManager &manager) {
  if(is_dead)
    return;

  if(manager.dead(instance_id)) {
    if(spawn_count > 0 && type == Type::single) {
      is_dead = true;
      return;
    }

    spawn_count++;
    auto spawn_pos = (FVec2(tile_pos) + FVec2(0.5f)) * default_tile_size;
    instance_id = manager.addSystem(def_id, {spawn_pos.x, spawn_pos.y});
  }

  if(!manager.dead(instance_id))
    manager.get(instance_id).params = params;
}

void Spawner::kill(FXManager &manager) {
  manager.kill(instance_id);
  is_dead = true;
}
}
