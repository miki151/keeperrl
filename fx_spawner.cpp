#include "fx_spawner.h"

#include "fx_manager.h"
#include "renderer.h"

namespace fx {

Spawner::Spawner(Type type, IVec2 tilePos, FVec2 targetOff, ParticleSystemDefId defId)
    : tilePos(tilePos), pos(tileCenter(tilePos)), targetOff(targetOff), defId(defId), type(type) {}

void Spawner::update(FXManager &manager) {
  if (isDead)
    return;

  if (manager.dead(instanceId)) {
    if (spawnCount > 0 && type == Type::single) {
      isDead = true;
      return;
    }

    spawnCount++;
    float scale(Renderer::nominalSize);
    instanceId = manager.addSystem(defId, pos * scale, targetOff * scale);
  }

  if (!manager.dead(instanceId))
    manager.get(instanceId).params = params;
}

void Spawner::kill(FXManager &manager) {
  manager.kill(instanceId, false);
  isDead = true;
}
}
