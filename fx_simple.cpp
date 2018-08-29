#include "fx_simple.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

FXId spawnEffect(FXName name, double x, double y) {
  return spawnEffect(name, x, y, Vec2(0, 0));
}

FXId spawnEffect(FXName name, double x, double y, Vec2 dir) {
  if(auto *inst = FXManager::getInstance()) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
    auto instId = inst->addSystem(name, {fpos, ftargetOff});
    INFO << "FX spawn: " << EnumInfo<FXName>::getString(name) << " at:" << x << ", " << y << " dir:" << dir;
    return {instId.index(), instId.spawnTime()};
  }

  return {-1, -1};
}

void setPos(FXId tid, double x, double y) {
  ParticleSystemId id(tid.first, tid.second);
  if (auto *inst = FXManager::getInstance()) {
    if (inst->valid(id)) {
      auto &system = inst->get(id);
      system.pos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    }
  }
}

bool isAlive(FXId tid) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance())
    return inst->alive(id);
  return false;
}

optional<FXName> name(FXId tid) {
  ParticleSystemId id(tid.first, tid.second);
  if (auto* inst = FXManager::getInstance())
    if (inst->valid(id)) {
      auto& system = inst->get(id);
      return system.defId;
    }

  return none;
}

void kill(FXId tid, bool immediate) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance())
    inst->kill(id, immediate);
}

void setColor(FXId tid, Color col, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < ParticleSystem::Params::maxColors);
      system.params.color[paramIndex] = FColor(col).rgb();
    }
  }
}
void setScalar(FXId tid, float value, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < ParticleSystem::Params::maxScalars);
      system.params.scalar[paramIndex] = value;
    }
  }
}

void setDir(FXId tid, Dir dir, int paramIndex) {
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
