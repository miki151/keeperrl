#include "stdafx.h"
#include "collective_warning.h"
#include "inventory.h"
#include "minion_trait.h"
#include "collective.h"
#include "item_factory.h"
#include "item_type.h"
#include "minion_equipment.h"
#include "creature.h"
#include "construction_map.h"
#include "collective_config.h"
#include "territory.h"
#include "item.h"
#include "minion_activity.h"
#include "game.h"
#include "content_factory.h"
#include "item_types.h"

SERIALIZE_DEF(CollectiveWarnings, warnings, warningTimes, lastWarningTime)

void CollectiveWarnings::setWarning(Warning w, bool state) {
  warnings.set(w, state);
}

void CollectiveWarnings::disable() {
  lastWarningTime = LocalTime(100000000);
}

void CollectiveWarnings::considerWarnings(Collective* col) {
  PROFILE;
  setWarning(Warning::DUNGEON_LEVEL, col->getDungeonLevel().level == 0 && col->getGame()->getGlobalTime() > 3000_global);
  setWarning(Warning::DIGGING, col->getTerritory().isEmpty());
  considerWeaponWarning(col);
  considerTorchesWarning(col);
}

bool CollectiveWarnings::isWarning(Warning w) const {
  return warnings.contains(w);
}

void CollectiveWarnings::considerWeaponWarning(Collective* col) {
  int numWeapons = col->getNumItems(ItemIndex::WEAPON);
  PItem genWeapon = ItemType(CustomItemId("Sword")).get(col->getGame()->getContentFactory());
  int numNeededWeapons = 0;
  for (Creature* c : col->getCreatures(MinionTrait::FIGHTER))
    if (col->usesEquipment(c) && col->getMinionEquipment().needsItem(c, genWeapon.get(), true))
      ++numNeededWeapons;
  setWarning(Warning::NO_WEAPONS, numNeededWeapons > numWeapons);
}

void CollectiveWarnings::considerTorchesWarning(Collective* col) {
  double numLit = 0;
  const double unlitPen = 4;
  for (auto type : col->getGame()->getContentFactory()->furniture.getFurnitureNeedingLight())
    for (auto pos : col->getConstructions().getBuiltPositions(type))
      if (pos.getLight() < 0.8)
        numLit -= unlitPen;
      else
        numLit += 1;
  setWarning(Warning::MORE_LIGHTS, numLit < 0);
}

TString CollectiveWarnings::getText(Warning w) {
  switch (w) {
    case Warning::DIGGING: return TStringId("DIGGING_WARNING");
    case Warning::RESOURCE_STORAGE: return TStringId("RESOURCE_STORAGE_WARNING");
    case Warning::EQUIPMENT_STORAGE: return TStringId("EQUIPMENT_STORAGE_WARNING");
    case Warning::LIBRARY: return TStringId("LIBRARY_WARNING");
    case Warning::BEDS: return TStringId("BEDS_WARNING");
    case Warning::TRAINING: return TStringId("TRAINING_WARNING");
    case Warning::TRAINING_UPGRADE: return TStringId("TRAINING_UPGRADE_WARNING");
    case Warning::NO_HATCHERY: return TStringId("NO_HATCHERY_WARNING");
    case Warning::WORKSHOP: return TStringId("WORKSHOP_WARNING");
    case Warning::NO_WEAPONS: return TStringId("NO_WEAPONS_WARNING");
    case Warning::GRAVES: return TStringId("GRAVES_WARNING");
    case Warning::CHESTS: return TStringId("CHESTS_WARNING");
    case Warning::NO_PRISON: return TStringId("NO_PRISON_WARNING");
    case Warning::LARGER_PRISON: return TStringId("LARGER_PRISON_WARNING");
    case Warning::TORTURE_ROOM: return TStringId("TORTURE_ROOM_WARNING");
    case Warning::MORE_CHESTS: return TStringId("MORE_CHESTS_WARNING");
    case Warning::DUNGEON_LEVEL: return TStringId("DUNGEON_LEVEL_WARNING");
    case Warning::MORE_LIGHTS: return TStringId("MORE_LIGHTS_WARNING");
    case Warning::GUARD_POSTS: return TStringId("GUARD_POSTS_WARNING");
  }
}

const auto anyWarningFrequency = 100_visible;
const auto warningFrequency = 500_visible;

optional<TString> CollectiveWarnings::getNextWarning(LocalTime time) {
  if (time > lastWarningTime + anyWarningFrequency)
    for (Warning w : ENUM_ALL(Warning))
      if (isWarning(w) && (!warningTimes[w] || time > *warningTimes[w] + warningFrequency)) {
        warningTimes[w] = lastWarningTime = time;
        return getText(w);
      }
  return none;
}

