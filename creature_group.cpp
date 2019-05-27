#include "stdafx.h"
#include "creature_group.h"
#include "monster_ai.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item_type.h"
#include "tribe.h"

SERIALIZE_DEF(CreatureGroup, tribe, creatures, weights, unique, tribeOverrides, baseLevelIncrease, inventory)
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

PCreature CreatureGroup::random(CreatureFactory* f) {
  return random(f, MonsterAIFactory::monster());
}

PCreature CreatureGroup::random(CreatureFactory* creatureFactory, const MonsterAIFactory& actorFactory) {
  CreatureId id;
  if (unique.size() > 0) {
    id = unique.back();
    unique.pop_back();
  } else
    id = Random.choose(creatures, weights);
  PCreature ret = creatureFactory->fromId(id, getTribeFor(id), actorFactory, inventory);
  for (auto exp : ENUM_ALL(ExperienceType))
    ret->getAttributes().increaseBaseExpLevel(exp, baseLevelIncrease[exp]);
  return ret;
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

CreatureGroup::CreatureGroup(TribeId tribe, const vector<pair<CreatureId, double>>& creatures,
                             const vector<CreatureId>& unique, map<CreatureId, optional<TribeId>> overrides)
    : CreatureGroup(tribe, creatures.transform([](const auto& c) { return c.first; }),
      creatures.transform([](const auto& c) { return c.second; }), unique, overrides) {
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
  return CreatureGroup(enemy, {
      make_pair("BANDIT", 100.),
      make_pair("GREEN_DRAGON", 5.),
      make_pair("RED_DRAGON", 5.),
      make_pair("SOFT_MONSTER", 5.),
      make_pair("CYCLOPS", 15.),
      make_pair("WITCH", 15.),
      make_pair("CLAY_GOLEM", 20.),
      make_pair("STONE_GOLEM", 20.),
      make_pair("IRON_GOLEM", 20.),
      make_pair("LAVA_GOLEM", 20.),
      make_pair("FIRE_ELEMENTAL", 10.),
      make_pair("WATER_ELEMENTAL", 10.),
      make_pair("EARTH_ELEMENTAL", 10.),
      make_pair("AIR_ELEMENTAL", 10.),
      make_pair("GNOME", 100.),
      make_pair("GNOME_CHIEF", 20.),
      make_pair("DWARF", 100.),
      make_pair("DWARF_FEMALE", 40.),
      make_pair("JACKAL", 200.),
      make_pair("BAT", 200.),
      make_pair("SNAKE", 150.),
      make_pair("SPIDER", 200.),
      make_pair("FLY", 100.),
      make_pair("RAT", 100.)},
      {}, {{"GNOME", peaceful}, {"GNOME_CHIEF", peaceful} });
}

#include "pretty_archive.h"
template
void CreatureGroup::serialize(PrettyInputArchive& ar1, unsigned);
