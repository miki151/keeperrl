#include "fx_simple.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

pair<int, int> spawnEffect(const char *name, double x, double y) { return spawnEffect(name, x, y, Vec2(0, 0)); }

pair<int, int> spawnEffect(const char *name, double x, double y, Vec2 dir) {
  if(auto *inst = FXManager::getInstance()) {
    if(auto id = inst->findSystem(name)) {
      auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
      auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
      auto instId = inst->addSystem(*id, fpos, ftargetOff);
      INFO << "FX spawn: " << name << " at:" << x << ", " << y << " dir:" << dir;
      return {instId.index(), instId.spawnTime()};
    }
    INFO << "FX spawn: couldn't find fx: " << name;
  }

  return {-1, -1};
}

void setPos(pair<int, int> tid, double x, double y) {
  ParticleSystemId id(tid.first, tid.second);
  if (auto *inst = FXManager::getInstance()) {
    if (inst->valid(id)) {
      auto &system = inst->get(id);
      system.pos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    }
  }
}

bool isAlive(pair<int, int> tid) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance())
    return inst->alive(id);
  return false;
}

void kill(pair<int, int> tid, bool immediate) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance())
    inst->kill(id, immediate);
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
