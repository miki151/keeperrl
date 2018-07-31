#include "fx_simple.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

pair<int, int> spawnEffect(const char *name, Vec2 pos) {
  printf("FX: Spawning %s at: %d %d\n", name, pos.x, pos.y);
  if(auto *inst = FXManager::getInstance())
    if(auto id = inst->findSystem(name)) {
      auto fpos = (FVec2(pos.x, pos.y) + fx::FVec2(0.5)) * 24.0f;
      auto inst_id = inst->addSystem(*id, fpos);
      return {inst_id.index(), inst_id.spawnTime()};
    }
  return {-1, -1};
}

bool isAlive(pair<int, int> tid) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance())
    return inst->alive(id);
  return false;
}

void kill(pair<int, int> tid) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance())
    inst->kill(id);
}

void setColor(pair<int, int> tid, Color col, int param_index) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(param_index >= 0 && param_index < ParticleSystem::Params::max_colors);
      system.params.color[param_index] = FColor(col).rgb();
    }
  }
}
void setScalar(pair<int, int> tid, float value, int param_index) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(param_index >= 0 && param_index < ParticleSystem::Params::max_scalars);
      system.params.scalar[param_index] = value;
    }
  }
}

void setDir(pair<int, int> tid, Dir dir, int param_index) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(param_index >= 0 && param_index < ParticleSystem::Params::max_dirs);
      system.params.dir[param_index] = dir;
    }
  }
}
}
