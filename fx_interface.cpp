#include "fx_interface.h"

#include "fx_manager.h"
#include "fx_particle_system.h"
#include "renderer.h"

namespace fx {

/*<<<<<<< HEAD
FXId spawnEffect(FXManager* manager, FXName name, float x, float y) {
  return spawnEffect(manager, name, x, y, Vec2(0, 0));
}

FXId spawnEffect(FXManager* manager, FXName name, float x, float y, Vec2 dir) {
  auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
  auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
  auto instId = manager->addSystem(name, {fpos, ftargetOff});
  INFO << "FX spawn: " << ENUM_STRING(name) << " at:" << x << ", " << y << " dir:" << dir;
  return {instId.getIndex(), instId.getSpawnTime()};
}

FXId spawnSnapshotEffect(FXManager* manager, FXName name, float x, float y, float scalar0, float scalar1) {
  auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
  auto instId = manager->addSystem(name, {fpos, SnapshotKey{scalar0, scalar1}});
  INFO << "FX spawn: " << ENUM_STRING(name) << " at:" << x << ", " << y;
  return {instId.getIndex(), instId.getSpawnTime()};
=======*/
FXId spawnEffect(FXManager* manager, FXName name, float x, float y) {
  auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
  InitConfig conf{fpos};
  conf.orderedDraw = true;
  auto instId = manager->addSystem(name, conf);
  INFO << "FX spawn: " << ENUM_STRING(name) << " at:" << x << ", " << y;
  return {instId.getIndex(), instId.getSpawnTime()};
}

FXId spawnUnorderedEffect(FXManager* manager, FXName name, float x, float y, Vec2 dir) {
  auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
  auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
  auto instId = manager->addSystem(name, {fpos, ftargetOff});
  INFO << "FX spawn unordered: " << ENUM_STRING(name) << " at:" << x << ", " << y << " dir:" << dir;
  return {instId.getIndex(), instId.getSpawnTime()};
}

FXId spawnSnapshotEffect(FXManager* manager, FXName name, float x, float y, float scalar0, float scalar1) {
    auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
    InitConfig config{fpos, SnapshotKey{scalar0, scalar1}};
    config.orderedDraw = true;
    auto instId = manager->addSystem(name, config);
    INFO << "FX spawn: " << ENUM_STRING(name) << " at:" << x << ", " << y;
    return {instId.getIndex(), instId.getSpawnTime()};
}

void setPos(FXManager* manager, FXId tid, float x, float y) {
  ParticleSystemId id(tid.first, tid.second);
  if (manager->valid(id)) {
    auto &system = manager->get(id);
    system.pos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
  }
}

optional<FXName> name(FXManager* manager, FXId tid) {
  ParticleSystemId id(tid.first, tid.second);
  if (manager->valid(id)) {
    auto& system = manager->get(id);
    return system.defId;
  }
  return none;
}

void setColor(FXManager* manager, FXId tid, Color col, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if (manager->valid(id)) {
    auto &system = manager->get(id);
    PASSERT(paramIndex >= 0 && paramIndex < SystemParams::maxColors);
    system.params.color[paramIndex] = FColor(col).rgb();
  }
}

void setScalar(FXManager* manager, FXId tid, float value, int paramIndex) {
  ParticleSystemId id(tid.first, tid.second);
  if (manager->valid(id)) {
    auto &system = manager->get(id);
    PASSERT(paramIndex >= 0 && paramIndex < SystemParams::maxScalars);
    system.params.scalar[paramIndex] = value;
  }
}
}
