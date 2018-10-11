#include "fx_view_manager.h"

#include "fx_base.h"
#include "fx_name.h"
#include "fx_variant_name.h"
#include "fx_info.h"
#include "fx_interface.h"
#include "fx_renderer.h"
#include "variant.h"
#include "fx_manager.h"
#include "renderer.h"

FXViewManager::~FXViewManager() = default;
FXViewManager::FXViewManager(fx::FXManager* manager, fx::FXRenderer* renderer)
    : fxManager(manager), fxRenderer(renderer) {
}

void FXViewManager::EntityInfo::clearVisibility() {
  isVisible = false;
  for (int n = 0; n < numEffects; n++)
    effects[n].isVisible = false;
}

void FXViewManager::EntityInfo::addFX(fx::FXManager* manager, GenericId id, TypeId typeId, const FXInfo& info) {
  PASSERT(isVisible); // Effects shouldn't be added if creature itself isn't visible

  for (int n = 0; n < numEffects; n++) {
    if (effects[n].typeId == typeId && !effects[n].isDying) {
      effects[n].isVisible = true;
      effects[n].stackId = info.stackId;
      updateParams(manager, info, effects[n].id);
      return;
    }
  }

  if (numEffects < maxEffects) {
    auto& newEffect = effects[numEffects++];
    INFO << "FX view: spawn: " << ENUM_STRING(info.name) << " for: " << id;

    if (justShown)
      newEffect.id = fx::spawnSnapshotEffect(manager, info.name, 0.0f, 0.0f, info.strength, 0.0f);
    else
      newEffect.id = fx::spawnEffect(manager, info.name, 0.0f, 0.0f);
    newEffect.typeId = typeId;
    newEffect.isVisible = true;
    newEffect.isDying = false;
    newEffect.stackId = info.stackId;

    updateParams(manager, info, newEffect.id);
  }
}

void FXViewManager::updateParams(fx::FXManager* manager, const FXInfo& info, FXId id) {
  if (!(info.color == Color::WHITE))
    fx::setColor(manager, id, info.color);
  if (info.strength != 0.0f)
    fx::setScalar(manager, id, info.strength, 0);
}

string FXViewManager::EffectInfo::name() const {
  return typeId.visit([](FXName name) { return ENUM_STRING(name); },
                      [](FXVariantName name) { return ENUM_STRING(name); });
}

void FXViewManager::EntityInfo::updateFX(fx::FXManager* manager, GenericId gid) {
  bool immediateKill = !isVisible;

  for (int n = 0; n < numEffects; n++) {
    auto& effect = effects[n];
    bool removeDead = effect.isDying && !manager->alive(fx::ParticleSystemId(effect.id.first, effect.id.second));

    if ((!effect.isDying && !effect.isVisible) || (effect.isDying && immediateKill)) {
      INFO << "FX view: kill: " << effect.name() << " for: " << gid;
      manager->kill(fx::ParticleSystemId(effect.id.first, effect.id.second), immediateKill);
      effect.isDying = true;
    }

    if (immediateKill || removeDead) {
      if (removeDead)
        INFO << "FX view: remove: " << effect.name() << " for: " << gid;
      effects[n--] = effects[--numEffects];
    }
  }

  // Updating stack position of stacked effects:
  if (numEffects >= 2)
    for (auto stackId : ENUM_ALL(FXStackId)) {
      if (stackId == FXStackId::generic)
        continue;

      int count = 0;
      for (int n = 0; n < numEffects; n++)
        if (effects[n].stackId == stackId)
          count++;
      if (count > 1) {
        int id = 0;
        for (int n = 0; n < numEffects; n++)
          if (effects[n].stackId == stackId)
            fx::setScalar(manager, effects[n].id, float(id++) / float(count), 1);
      }
    }
}

void FXViewManager::beginFrame(Renderer& renderer, float zoom, float ox, float oy) {
  for (auto& pair : entities)
    pair.second.clearVisibility();

  auto size = renderer.getSize();
  m_zoom = zoom;
  fxRenderer->setView(zoom, ox, oy, size.x, size.y);
  fxRenderer->prepareOrdered();
}

void FXViewManager::addEntity(GenericId gid, float x, float y) {
  auto it = entities.find(gid);
  if (it == entities.end()) {
    EntityInfo newInfo;
    newInfo.x = x;
    newInfo.y = y;
    entities[gid] = newInfo;
  } else {
    it->second.isVisible = true;
    it->second.x = x;
    it->second.y = y;
  }
}

void FXViewManager::addFX(GenericId gid, const FXInfo& info) {
  auto it = entities.find(gid);
  PASSERT(it != entities.end());
  it->second.addFX(fxManager, gid, info.name, info);
}

void FXViewManager::addFX(GenericId gid, FXVariantName var) {
  auto it = entities.find(gid);
  PASSERT(it != entities.end());
  it->second.addFX(fxManager, gid, var, getFXInfo(var));
}

void FXViewManager::finishFrame() {
  for (auto it = begin(entities); it != end(entities);) {
    auto next = it;
    ++next;

    it->second.justShown = false;
    it->second.updateFX(fxManager, it->first);
    if (!it->second.isVisible)
      entities.erase(it);
    it = next;
  }
}

void FXViewManager::drawFX(Renderer& renderer, GenericId id) {
  auto it = entities.find(id);
  PASSERT(it != entities.end());
  auto& entity = it->second;

  int ids[EntityInfo::maxEffects];
  int num = 0;

  for (int n = 0; n < entity.numEffects; n++) {
    auto& effect = entity.effects[n];
    if (effect.isVisible)
      ids[num++] = effect.id.first;
  }

  if (num > 0) {
    renderer.flushSprites();
    float px = (entity.x) * Renderer::nominalSize * m_zoom;
    float py = (entity.y) * Renderer::nominalSize * m_zoom;
    fxRenderer->drawOrdered(ids, num, px, py);
  }
}

void FXViewManager::drawUnorderedBackFX(Renderer& renderer) {
  renderer.flushSprites();
  fxRenderer->drawUnordered(fx::Layer::back);
}

void FXViewManager::drawUnorderedFrontFX(Renderer& renderer) {
  renderer.flushSprites();
  fxRenderer->drawUnordered(fx::Layer::front);
}

// TODO: unmanaged are automatically unordered; make it the same ?
void FXViewManager::addUnmanagedFX(const FXSpawnInfo& spawnInfo) {
  auto coord = spawnInfo.position;
  float x = coord.x, y = coord.y;
  if (spawnInfo.genericId) {
    // TODO: delay until end of frame? Increases chance that entity info will be available
    auto it = entities.find(spawnInfo.genericId);
    if (it != entities.end()) {
      x = it->second.x;
      y = it->second.y;
    }
  }
  auto id = fx::spawnUnorderedEffect(fxManager, spawnInfo.info.name, x, y, spawnInfo.targetOffset);
  updateParams(fxManager, spawnInfo.info, id);
}
