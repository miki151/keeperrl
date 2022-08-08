#include "fx_view_manager.h"

#include "fx_base.h"
#include "fx_name.h"
#include "fx_variant_name.h"
#include "fx_info.h"
#include "fx_renderer.h"
#include "variant.h"
#include "fx_manager.h"
#include "renderer.h"

using fx::FVec2;
using fx::InitConfig;
using FXId = FXViewManager::FXId;

struct FXViewManager::EffectInfo {
  string name() const;

  FXId id;
  TypeId typeId;
  FXStackId stackId;
  bool isVisible;
  bool isDying;
};

struct FXViewManager::EntityInfo {
  static constexpr int maxEffects = 8;

  void clearVisibility();
  void addFX(FXViewManager*, GenericId, TypeId, const FXInfo&);
  void updateFX(FXViewManager*, GenericId);

  float x, y;
  EffectInfo effects[maxEffects];
  int numEffects = 0;
  bool isVisible = true;
  bool justShown = true;
};

FXViewManager::~FXViewManager() = default;
FXViewManager::FXViewManager(fx::FXManager* manager, fx::FXRenderer* renderer)
    : fxManager(manager), fxRenderer(renderer) {
  entities = make_unique<EntityMap>();
}

FXId FXViewManager::spawnOrderedEffect(const FXInfo& info, bool snapshot) {
  InitConfig config{FVec2(Renderer::nominalSize / 2)}; // position is passed when drawing
  config.orderedDraw = true;
  if (snapshot)
    config.snapshotKey = fx::SnapshotKey{info.strength, 0.0f};
  return fxManager->addSystem(info.name, config);
}

FXId FXViewManager::spawnUnorderedEffect(FXName name, float x, float y, Vec2 dir, Color color) {
  auto fpos = (FVec2(x, y) + FVec2(0.5f)) * Renderer::nominalSize;
  auto ftargetOff = FVec2(dir.x, dir.y) * Renderer::nominalSize;
  INFO << "FX spawn unordered: " << ENUM_STRING(name) << " at:" << x << ", " << y << " dir:" << dir;
  return fxManager->addSystem(name, InitConfig(fpos, ftargetOff, color));
}

void FXViewManager::EntityInfo::clearVisibility() {
  isVisible = false;
  for (int n = 0; n < numEffects; n++)
    effects[n].isVisible = false;
}

void FXViewManager::updateParams(const FXInfo& info, FXId id) {
  if (fxManager->valid(id)) {
    auto& system = fxManager->get(id);
    system.params.color[0] = fx::FColor(info.color).rgb();
    system.params.scalar[0] = info.strength;
  }
}

string FXViewManager::EffectInfo::name() const {
  return typeId.visit([](FXName name) { return ENUM_STRING(name); },
                      [](FXVariantName name) { return ENUM_STRING(name); });
}

void FXViewManager::EntityInfo::addFX(FXViewManager* manager, GenericId id, TypeId typeId, const FXInfo& info) {
  PASSERT(isVisible); // Effects shouldn't be added if creature itself isn't visible

  for (int n = 0; n < numEffects; n++) {
    if (effects[n].typeId == typeId && !effects[n].isDying) {
      effects[n].isVisible = true;
      effects[n].stackId = info.stackId;
      manager->updateParams(info, effects[n].id);
      return;
    }
  }

  if (numEffects < maxEffects) {
    auto& newEffect = effects[numEffects++];
    INFO << "FX spawn ordered: " << ENUM_STRING(info.name) << " for: " << id << "at: " << x << ", " << y;

    newEffect.id = manager->spawnOrderedEffect(info, justShown);
    newEffect.typeId = typeId;
    newEffect.isVisible = true;
    newEffect.isDying = false;
    newEffect.stackId = info.stackId;

    manager->updateParams(info, newEffect.id);
  }
}

void FXViewManager::EntityInfo::updateFX(FXViewManager* manager, GenericId gid) {
  bool immediateKill = !isVisible;
  auto* fxManager = manager->fxManager;

  for (int n = 0; n < numEffects; n++) {
    auto& effect = effects[n];
    bool removeDead = effect.isDying && !fxManager->alive(effect.id);

    if ((!effect.isDying && !effect.isVisible) || (effect.isDying && immediateKill)) {
      INFO << "FX kill: " << effect.name() << " for: " << gid;
      fxManager->kill(effect.id, immediateKill);
      effect.isDying = true;
    }

    if (immediateKill || removeDead) {
      if (removeDead)
        INFO << "FX remove: " << effect.name() << " for: " << gid;
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
        int stackPos = 0;
        for (int n = 0; n < numEffects; n++)
          if (effects[n].stackId == stackId && fxManager->valid(effects[n].id))
            fxManager->get(effects[n].id).params.scalar[1] = float(stackPos++) / float(count);
      }
    }
}

void FXViewManager::beginFrame(Renderer& renderer, float zoom, float ox, float oy) {
  for (auto& pair : *entities)
    pair.second.clearVisibility();

  auto size = renderer.getSize();
  m_zoom = zoom;
  fxRenderer->setView(zoom, ox, oy, size.x, size.y);
  fxRenderer->prepareOrdered();
}

void FXViewManager::addEntity(GenericId gid, float x, float y) {
  auto it = entities->find(gid);
  if (it == entities->end()) {
    EntityInfo newInfo;
    newInfo.x = x;
    newInfo.y = y;
    (*entities)[gid] = newInfo;
  } else {
    it->second.isVisible = true;
    it->second.x = x;
    it->second.y = y;
  }
}

void FXViewManager::addFX(GenericId gid, const FXInfo& info) {
  auto it = entities->find(gid);
  PASSERT(it != entities->end());
  it->second.addFX(this, gid, info.name, info);
}

void FXViewManager::addFX(GenericId gid, FXVariantName var, bool bigSprite) {
  auto it = entities->find(gid);
  PASSERT(it != entities->end());
  auto info = getFXInfo(var);
  if (bigSprite && isOneOf(info.name, FXName::BUFF, FXName::DEBUFF))
    info.strength = 1.0f;
  it->second.addFX(this, gid, var, info);
}

void FXViewManager::finishFrame() {
  for (auto it = begin(*entities); it != end(*entities);) {
    auto next = it;
    ++next;

    it->second.justShown = false;
    it->second.updateFX(this, it->first);
    if (!it->second.isVisible)
      entities->erase(it);
    it = next;
  }
}

void FXViewManager::drawFX(Renderer& renderer, GenericId id, Color color) {
  PROFILE;
  auto it = entities->find(id);
  PASSERT(it != entities->end());
  auto& entity = it->second;

  int ids[EntityInfo::maxEffects];
  int num = 0;

  for (int n = 0; n < entity.numEffects; n++) {
    auto& effect = entity.effects[n];
    if (effect.isVisible)
      ids[num++] = effect.id.getIndex();
  }

  if (num > 0) {
    renderer.renderDeferredSprites();
    float px = (entity.x) * Renderer::nominalSize * m_zoom;
    float py = (entity.y) * Renderer::nominalSize * m_zoom;
    fxRenderer->drawOrdered(ids, num, px, py, color);
  }
}

void FXViewManager::drawUnorderedBackFX(Renderer& renderer) {
  renderer.renderDeferredSprites();
  fxRenderer->drawUnordered(fx::Layer::back);
}

void FXViewManager::drawUnorderedFrontFX(Renderer& renderer) {
  renderer.renderDeferredSprites();
  fxRenderer->drawUnordered(fx::Layer::front);
}

void FXViewManager::addUnmanagedFX(const FXSpawnInfo& spawnInfo, Color color) {
  auto coord = spawnInfo.position;
  auto offset = spawnInfo.targetOffset;
  if (spawnInfo.info.reversed) {
    coord += offset;
    offset = -offset;
  }
  float x = coord.x, y = coord.y;
  auto id = spawnUnorderedEffect(spawnInfo.info.name, x, y, offset, color);
  updateParams(spawnInfo.info, id);
}
