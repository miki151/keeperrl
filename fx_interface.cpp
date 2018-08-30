#include "fx_interface.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

FXId spawnEffect(FXName name, float x, float y) {
  return spawnEffect(name, x, y, Vec2(0, 0));
}

FXId spawnEffect(FXName name, float x, float y, Vec2 dir) {
  if(auto *inst = FXManager::getInstance()) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
    auto instId = inst->addSystem(name, {fpos, ftargetOff});
    INFO << "FX spawn: " << EnumInfo<FXName>::getString(name) << " at:" << x << ", " << y << " dir:" << dir;
    return {instId.index(), instId.spawnTime()};
  }

  return {-1, -1};
}

FXId spawnSnapshotEffect(FXName name, float x, float y, float scalar0, float scalar1) {
  if (auto* inst = FXManager::getInstance()) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    auto instId = inst->addSystem(name, {fpos, SnapshotKey{scalar0, scalar1}});
    INFO << "FX spawn: " << EnumInfo<FXName>::getString(name) << " at:" << x << ", " << y;
    return {instId.index(), instId.spawnTime()};
  }

  return {-1, -1};
}

void setPos(FXId tid, float x, float y) {
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
      PASSERT(paramIndex >= 0 && paramIndex < SystemParams::maxColors);
      system.params.color[paramIndex] = FColor(col).rgb();
    }
  }
}
void setScalar(FXId tid, float value, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < SystemParams::maxScalars);
      system.params.scalar[paramIndex] = value;
    }
  }
}

void setDir(FXId tid, Dir dir, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if(auto *inst = FXManager::getInstance()) {
    if(inst->valid(id)) {
      auto &system = inst->get(id);
      PASSERT(paramIndex >= 0 && paramIndex < SystemParams::maxDirs);
      system.params.dir[paramIndex] = dir;
    }
  }
}
}
