#include "fx_interface.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

FXId spawnEffect(FXName name, float x, float y) {
  if (auto* inst = FXManager::getInstance()) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    InitConfig conf{fpos};
    //conf.orderedDraw = true; // TODO: enable
    auto instId = inst->addSystem(name, conf);
    INFO << "FX spawn: " << ENUM_STRING(name) << " at:" << x << ", " << y;
    return {instId.getIndex(), instId.getSpawnTime()};
  }

  return {-1, -1};
}

FXId spawnUnorderedEffect(FXName name, float x, float y, Vec2 dir) {
  if(auto *inst = FXManager::getInstance()) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
    auto instId = inst->addSystem(name, {fpos, ftargetOff});
    INFO << "FX spawn unordered: " << ENUM_STRING(name) << " at:" << x << ", " << y << " dir:" << dir;
    return {instId.getIndex(), instId.getSpawnTime()};
  }

  return {-1, -1};
}

FXId spawnSnapshotEffect(FXName name, float x, float y, float scalar0, float scalar1) {
  if (auto* inst = FXManager::getInstance()) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    InitConfig config{fpos, SnapshotKey{scalar0, scalar1}};
    //config.orderedDraw = true; // TODO: enable
    auto instId = inst->addSystem(name, config);
    INFO << "FX spawn: " << ENUM_STRING(name) << " at:" << x << ", " << y;
    return {instId.getIndex(), instId.getSpawnTime()};
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
}

bool fxesAvailable() {
  return fx::FXManager::getInstance() != nullptr;
}
