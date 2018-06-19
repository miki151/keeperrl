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
#include "experience_type.h"

SERIALIZE_DEF(CollectiveWarnings, warnings, warningTimes, lastWarningTime)

void CollectiveWarnings::setWarning(Warning w, bool state) {
  warnings.set(w, state);
}

void CollectiveWarnings::disable() {
  lastWarningTime = LocalTime(100000000);
}

void CollectiveWarnings::considerWarnings(WCollective col) {
  setWarning(Warning::MANA, col->numResource(CollectiveResourceId::MANA) < 100);
  setWarning(Warning::DIGGING, col->getTerritory().isEmpty());
  considerMoraleWarning(col);
  considerWeaponWarning(col);
  considerTorchesWarning(col);
}

bool CollectiveWarnings::isWarning(Warning w) const {
  return warnings.contains(w);
}

void CollectiveWarnings::considerWeaponWarning(WCollective col) {
  int numWeapons = col->getNumItems(ItemIndex::WEAPON);
  PItem genWeapon = ItemType(ItemType::Sword{}).get();
  int numNeededWeapons = 0;
  for (WCreature c : col->getCreatures(MinionTrait::FIGHTER))
    if (col->usesEquipment(c) && col->getMinionEquipment().needsItem(c, genWeapon.get(), true))
      ++numNeededWeapons;
  setWarning(Warning::NO_WEAPONS, numNeededWeapons > numWeapons);
}

void CollectiveWarnings::considerMoraleWarning(WCollective col) {
  vector<WCreature> minions = col->getCreatures(MinionTrait::FIGHTER);
  setWarning(Warning::LOW_MORALE,
      minions.filter([] (WConstCreature c) { return c->getMorale() < -0.2; }).size() > minions.size() / 2);
}

void CollectiveWarnings::considerTorchesWarning(WCollective col) {
  double numLit = 0;
  const double unlitPen = 4;
  for (auto type : col->getConfig().getRoomsNeedingLight())
    for (auto pos : col->getConstructions().getBuiltPositions(type))
      if (pos.getLight() < 0.8)
        numLit -= unlitPen;
      else
        numLit += 1;
  setWarning(Warning::MORE_LIGHTS, numLit < 0);
}

const char* CollectiveWarnings::getText(Warning w) {
  switch (w) {
    case Warning::DIGGING: return "Dig into the mountain and start building a dungeon.";
    case Warning::RESOURCE_STORAGE: return "Storage room for resources is needed.";
    case Warning::EQUIPMENT_STORAGE: return "Storage room for equipment is needed.";
    case Warning::LIBRARY: return "Build a library to start research.";
    case Warning::BEDS: return "You need to build beds for your minions.";
    case Warning::TRAINING: return "Build a training room for your minions.";
    case Warning::TRAINING_UPGRADE: return "Training room upgrade needed.";
    case Warning::NO_HATCHERY: return "You need to build a pigsty.";
    case Warning::WORKSHOP: return "Build a workshop to produce equipment and traps.";
    case Warning::NO_WEAPONS: return "You need weapons for your minions.";
    case Warning::LOW_MORALE: return "Kill some enemies or summon a succubus to increase morale of your minions.";
    case Warning::GRAVES: return "You need a graveyard to collect corpses";
    case Warning::CHESTS: return "You need to build treasure chests.";
    case Warning::NO_PRISON: return "You need to build a prison.";
    case Warning::LARGER_PRISON: return "You need a larger prison.";
    case Warning::TORTURE_ROOM: return "You need to build a torture room.";
    case Warning::MORE_CHESTS: return "You need more treasure chests.";
    case Warning::MANA: return "Conquer an enemy tribe for more mana.";
    case Warning::MORE_LIGHTS: return "Place some torches to light up your dungeon.";
  }
  return "";
}

const auto anyWarningFrequency = 100_visible;
const auto warningFrequency = 500_visible;

optional<const char*> CollectiveWarnings::getNextWarning(LocalTime time) {
  if (time > lastWarningTime + anyWarningFrequency)
    for (Warning w : ENUM_ALL(Warning))
      if (isWarning(w) && (!warningTimes[w] || time > *warningTimes[w] + warningFrequency)) {
        warningTimes[w] = lastWarningTime = time;
        return getText(w);
      }
  return none;
}

