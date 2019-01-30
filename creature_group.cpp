#include "stdafx.h"
#include "creature_group.h"
#include "monster_ai.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item_type.h"
#include "tribe.h"

SERIALIZE_DEF(CreatureGroup, tribe, creatures, weights, unique, tribeOverrides, levelIncrease, baseLevelIncrease, inventory)
SERIALIZATION_CONSTRUCTOR_IMPL(CreatureGroup)

CreatureGroup CreatureGroup::singleCreature(TribeId tribe, CreatureId id) {
  return CreatureGroup(tribe, {id}, {1}, {});
}


TribeId CreatureGroup::getTribeFor(CreatureId id) {
  if (auto t = tribeOverrides[id])
    return *t;
  else
    return *tribe;
}

PCreature CreatureGroup::random(const CreatureFactory* f) {
  return random(f, MonsterAIFactory::monster());
}

PCreature CreatureGroup::random(const CreatureFactory* creatureFactory, const MonsterAIFactory& actorFactory) {
  CreatureId id;
  if (unique.size() > 0) {
    id = unique.back();
    unique.pop_back();
  } else
    id = Random.choose(creatures, weights);
  PCreature ret = creatureFactory->fromId(id, getTribeFor(id), actorFactory, inventory);
  for (auto exp : ENUM_ALL(ExperienceType)) {
    ret->getAttributes().increaseBaseExpLevel(exp, baseLevelIncrease[exp]);
    ret->increaseExpLevel(exp, levelIncrease[exp]);
  }
  return ret;
}

CreatureGroup& CreatureGroup::increaseLevel(EnumMap<ExperienceType, int> l) {
  levelIncrease = l;
  return *this;
}

CreatureGroup& CreatureGroup::increaseLevel(ExperienceType t, int l) {
  levelIncrease[t] = l;
  return *this;
}

CreatureGroup& CreatureGroup::increaseBaseLevel(ExperienceType t, int l) {
  baseLevelIncrease[t] = l;
  return *this;
}

CreatureGroup& CreatureGroup::addInventory(vector<ItemType> items) {
  inventory = items;
  return *this;
}

CreatureGroup::CreatureGroup(TribeId t, const vector<CreatureId>& c, const vector<double>& w,
    const vector<CreatureId>& u, map<CreatureId, optional<TribeId>> overrides)
    : tribe(t), creatures(c), weights(w), unique(u), tribeOverrides(overrides) {
}

CreatureGroup::CreatureGroup(const vector<tuple<CreatureId, double, TribeId>>& c, const vector<CreatureId>& u)
    : unique(u) {
  for (auto& elem : c) {
    creatures.push_back(std::get<0>(elem));
    weights.push_back(std::get<1>(elem));
    tribeOverrides[std::get<0>(elem)] = std::get<2>(elem);
  }
}

// These have to be defined here to be able to forward declare some ItemType and other classes
CreatureGroup::~CreatureGroup() {
}

CreatureGroup::CreatureGroup(const CreatureGroup&) = default;

CreatureGroup& CreatureGroup::operator =(const CreatureGroup&) = default;

static optional<pair<CreatureGroup, CreatureGroup>> splashFactories;

void CreatureGroup::initSplash(TribeId tribe) {
  splashFactories = Random.choose(
      make_pair(CreatureGroup(tribe, { "KNIGHT", "ARCHER"}, { 1, 1}, {}),
        CreatureGroup::singleType(tribe, "DUKE")),
      make_pair(CreatureGroup(tribe, { "WARRIOR"}, { 1}, {}),
        CreatureGroup::singleType(tribe, "SHAMAN")),
      make_pair(CreatureGroup(tribe, { "ELF_ARCHER"}, { 1}, {}),
        CreatureGroup::singleType(tribe, "ELF_LORD")),
      make_pair(CreatureGroup(tribe, { "DWARF"}, { 1}, {}),
        CreatureGroup::singleType(tribe, "DWARF_BARON")),
      make_pair(CreatureGroup(tribe, { "LIZARDMAN"}, { 1}, {}),
        CreatureGroup::singleType(tribe, "LIZARDLORD"))
      );
}

CreatureGroup CreatureGroup::splashHeroes(TribeId tribe) {
  if (!splashFactories)
    initSplash(tribe);
  return splashFactories->first;
}

CreatureGroup CreatureGroup::splashLeader(TribeId tribe) {
  if (!splashFactories)
    initSplash(tribe);
  return splashFactories->second;
}

CreatureGroup CreatureGroup::splashMonsters(TribeId tribe) {
  return CreatureGroup(tribe, { "GNOME", "GOBLIN", "OGRE",
      "SPECIAL_HLBN", "SPECIAL_BLBW", "WOLF", "CAVE_BEAR",
      "BAT", "WEREWOLF", "ZOMBIE", "VAMPIRE", "DOPPLEGANGER",
      "SUCCUBUS"},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {}, {}).increaseBaseLevel(ExperienceType::MELEE, 25);
}

CreatureGroup CreatureGroup::forrest(TribeId tribe) {
  return CreatureGroup(tribe,
      { "DEER", "FOX", "BOAR" },
      { 4, 2, 2}, {});
}

CreatureGroup CreatureGroup::waterCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { "WATER_ELEMENTAL", "KRAKEN" }, {20, 1}, {"KRAKEN"});
}

CreatureGroup CreatureGroup::elementals(TribeId tribe) {
  return CreatureGroup(tribe, {"AIR_ELEMENTAL", "FIRE_ELEMENTAL", "WATER_ELEMENTAL",
      "EARTH_ELEMENTAL"}, {1, 1, 1, 1}, {});
}

CreatureGroup CreatureGroup::lavaCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { "FIRE_ELEMENTAL" }, {1}, { });
}

CreatureGroup CreatureGroup::singleType(TribeId tribe, CreatureId id) {
  return CreatureGroup(tribe, { id}, {1}, {});
}

CreatureGroup CreatureGroup::gnomishMines(TribeId peaceful, TribeId enemy, int level) {
  return CreatureGroup({
      make_tuple("BANDIT", 100., enemy),
      make_tuple("GREEN_DRAGON", 5., enemy),
      make_tuple("RED_DRAGON", 5., enemy),
      make_tuple("SOFT_MONSTER", 5., enemy),
      make_tuple("CYCLOPS", 15., enemy),
      make_tuple("WITCH", 15., enemy),
      make_tuple("CLAY_GOLEM", 20., enemy),
      make_tuple("STONE_GOLEM", 20., enemy),
      make_tuple("IRON_GOLEM", 20., enemy),
      make_tuple("LAVA_GOLEM", 20., enemy),
      make_tuple("FIRE_ELEMENTAL", 10., enemy),
      make_tuple("WATER_ELEMENTAL", 10., enemy),
      make_tuple("EARTH_ELEMENTAL", 10., enemy),
      make_tuple("AIR_ELEMENTAL", 10., enemy),
      make_tuple("GNOME", 100., peaceful),
      make_tuple("GNOME_CHIEF", 20., peaceful),
      make_tuple("DWARF", 100., enemy),
      make_tuple("DWARF_FEMALE", 40., enemy),
      make_tuple("JACKAL", 200., enemy),
      make_tuple("BAT", 200., enemy),
      make_tuple("SNAKE", 150., enemy),
      make_tuple("SPIDER", 200., enemy),
      make_tuple("FLY", 100., enemy),
      make_tuple("RAT", 100., enemy)});
}
