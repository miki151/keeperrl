#include "fx_view_manager.h"

#include "fx_base.h"
#include "fx_name.h"
#include "fx_variant_name.h"
#include "variant.h"

void FXViewManager::EntityInfo::clearVisibility() {
  isVisible = false;
  for (int n = 0; n < numEffects; n++)
    effects[n].isVisible = false;
}

void FXViewManager::EntityInfo::addFX(GenericId id, TypeId typeId, const FXDef& def) {
  PASSERT(isVisible); // Effects shouldn't be added if creature itself isn't visible

  for (int n = 0; n < numEffects; n++) {
    if (effects[n].typeId == typeId && !effects[n].isDying) {
      effects[n].isVisible = true;
      effects[n].stackId = def.stackId;
      fx::setPos(effects[n].id, x, y);
      updateParams(def, effects[n].id);
      return;
    }
  }

  if (numEffects < maxEffects) {
    auto& newEffect = effects[numEffects++];
    INFO << "FX view: spawn: " << ENUM_STRING(def.name) << " for: " << id;

    if (justShown)
      newEffect.id = fx::spawnSnapshotEffect(def.name, x, y, def.strength, 0.0f);
    else
      newEffect.id = fx::spawnEffect(def.name, x, y);
    newEffect.typeId = typeId;
    newEffect.isVisible = true;
    newEffect.isDying = false;
    newEffect.stackId = def.stackId;

    updateParams(def, newEffect.id);
  }
}

void FXViewManager::EntityInfo::updateParams(const FXDef& def, FXId id) {
  if (!(def.color == Color::WHITE))
    fx::setColor(id, def.color);
  if (def.strength != 0.0f)
    fx::setScalar(id, def.strength, 0);
}

string FXViewManager::EffectInfo::name() const {
  return typeId.visit([](FXName name) { return ENUM_STRING(name); },
                      [](FXVariantName name) { return ENUM_STRING(name); });
}

void FXViewManager::EntityInfo::updateFX(GenericId gid) {
  bool immediateKill = !isVisible;

  for (int n = 0; n < numEffects; n++) {
    auto& effect = effects[n];
    bool removeDead = effect.isDying && !fx::isAlive(effect.id);

    if (effect.isDying)
      fx::setPos(effects[n].id, x, y);

    if ((!effect.isDying && !effect.isVisible) || (effect.isDying && immediateKill)) {
      INFO << "FX view: kill: " << effect.name() << " for: " << gid;
      fx::kill(effect.id, immediateKill);
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
            fx::setScalar(effects[n].id, float(id++) / float(count - 1), 1);
      }
    }
}

void FXViewManager::beginFrame() {
  for (auto& pair : entities)
    pair.second.clearVisibility();
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

void FXViewManager::addFX(GenericId gid, const FXDef& def) {
  auto it = entities.find(gid);
  PASSERT(it != entities.end());
  it->second.addFX(gid, def.name, def);
}

void FXViewManager::addFX(GenericId gid, FXVariantName var) {
  auto it = entities.find(gid);
  PASSERT(it != entities.end());
  it->second.addFX(gid, var, getDef(var));
}

void FXViewManager::finishFrame() {
  for (auto it = begin(entities); it != end(entities);) {
    auto next = it;
    ++next;

    it->second.justShown = false;
    it->second.updateFX(it->first);
    if (!it->second.isVisible) {
      //INFO << "FX view: clear: " << it->first;
      entities.erase(it);
    }
    it = next;
  }
}
