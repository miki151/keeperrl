#include "fx_simple.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

pair<int, int> spawnEffect(const char *name, Vec2 pos) { return spawnEffect(name, pos, Vec2(0, 0)); }

pair<int, int> spawnEffect(const char *name, Vec2 pos, Vec2 dir) {
  if(auto *inst = FXManager::getInstance()) {
    if(auto id = inst->findSystem(name)) {
      auto fpos = (FVec2(pos.x, pos.y) + fx::FVec2(0.5)) * 24.0f;
      auto ftargetOff = FVec2(dir.x, dir.y) * 24.0f;
      auto instId = inst->addSystem(*id, fpos, ftargetOff);
      INFO << "FX spawn: " << name << " at:" << pos << " dir:" << dir;
      return {instId.index(), instId.spawnTime()};
    }
    INFO << "FX spawn: couldn't find fx: " << name;
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

void setColor(pair<int, int> tid, Color col, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < ParticleSystem::Params::maxColors);
      system.params.color[paramIndex] = FColor(col).rgb();
    }
  }
}
void setScalar(pair<int, int> tid, float value, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < ParticleSystem::Params::maxScalars);
      system.params.scalar[paramIndex] = value;
    }
  }
}

void setDir(pair<int, int> tid, Dir dir, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < ParticleSystem::Params::maxDirs);
      system.params.dir[paramIndex] = dir;
    }
  }
}
}
