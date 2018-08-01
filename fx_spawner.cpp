#include "fx_spawner.h"

#include "fx_manager.h"

namespace fx {

Spawner::Spawner(Type type, IVec2 tile_pos, FVec2 target_offset, ParticleSystemDefId def_id)
    : tile_pos(tile_pos), pos(tileCenter(tile_pos)), target_offset(target_offset), def_id(def_id), type(type) {}

void Spawner::update(FXManager &manager) {
  if(is_dead)
    return;

  if(manager.dead(instance_id)) {
    if(spawn_count > 0 && type == Type::single) {
      is_dead = true;
      return;
    }

    spawn_count++;
    float scale(default_tile_size);
    instance_id = manager.addSystem(def_id, pos * scale, target_offset * scale);
  }

  if(!manager.dead(instance_id))
    manager.get(instance_id).params = params;
}

void Spawner::kill(FXManager &manager) {
  manager.kill(instance_id);
  is_dead = true;
}
}
