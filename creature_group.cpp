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
      make_pair(CreatureGroup(tribe, { CreatureId("KNIGHT"), CreatureId("ARCHER")}, { 1, 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId("DUKE"))),
      make_pair(CreatureGroup(tribe, { CreatureId("WARRIOR")}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId("SHAMAN"))),
      make_pair(CreatureGroup(tribe, { CreatureId("ELF_ARCHER")}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId("ELF_LORD"))),
      make_pair(CreatureGroup(tribe, { CreatureId("DWARF")}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId("DWARF_BARON"))),
      make_pair(CreatureGroup(tribe, { CreatureId("LIZARDMAN")}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId("LIZARDLORD")))
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
  return CreatureGroup(tribe, { CreatureId("GNOME"), CreatureId("GOBLIN"), CreatureId("OGRE"),
      CreatureId("SPECIAL_HLBN"), CreatureId("SPECIAL_BLBW"), CreatureId("WOLF"), CreatureId("CAVE_BEAR"),
      CreatureId("BAT"), CreatureId("WEREWOLF"), CreatureId("ZOMBIE"), CreatureId("VAMPIRE"), CreatureId("DOPPLEGANGER"),
      CreatureId("SUCCUBUS")},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {}, {}).increaseBaseLevel(ExperienceType::MELEE, 25);
}

CreatureGroup CreatureGroup::forrest(TribeId tribe) {
  return CreatureGroup(tribe,
      { CreatureId("DEER"), CreatureId("FOX"), CreatureId("BOAR") },
      { 4, 2, 2}, {});
}

CreatureGroup CreatureGroup::waterCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId("WATER_ELEMENTAL"), CreatureId("KRAKEN") }, {20, 1}, {CreatureId("KRAKEN")});
}

CreatureGroup CreatureGroup::elementals(TribeId tribe) {
  return CreatureGroup(tribe, {CreatureId("AIR_ELEMENTAL"), CreatureId("FIRE_ELEMENTAL"), CreatureId("WATER_ELEMENTAL"),
      CreatureId("EARTH_ELEMENTAL")}, {1, 1, 1, 1}, {});
}

CreatureGroup CreatureGroup::lavaCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId("FIRE_ELEMENTAL") }, {1}, { });
}

CreatureGroup CreatureGroup::singleType(TribeId tribe, CreatureId id) {
  return CreatureGroup(tribe, { id}, {1}, {});
}

CreatureGroup CreatureGroup::gnomishMines(TribeId peaceful, TribeId enemy, int level) {
  return CreatureGroup(enemy, {
      make_pair(CreatureId("BANDIT"), 100.),
      make_pair(CreatureId("GREEN_DRAGON"), 5.),
      make_pair(CreatureId("RED_DRAGON"), 5.),
      make_pair(CreatureId("BLACK_DRAGON"), 5.),
      make_pair(CreatureId("SOFT_MONSTER"), 5.),
      make_pair(CreatureId("CYCLOPS"), 15.),
      make_pair(CreatureId("WITCH"), 15.),
      make_pair(CreatureId("CLAY_GOLEM"), 20.),
      make_pair(CreatureId("STONE_GOLEM"), 20.),
      make_pair(CreatureId("IRON_GOLEM"), 20.),
      make_pair(CreatureId("LAVA_GOLEM"), 20.),
      make_pair(CreatureId("FIRE_ELEMENTAL"), 10.),
      make_pair(CreatureId("WATER_ELEMENTAL"), 10.),
      make_pair(CreatureId("EARTH_ELEMENTAL"), 10.),
      make_pair(CreatureId("AIR_ELEMENTAL"), 10.),
      make_pair(CreatureId("GNOME"), 100.),
      make_pair(CreatureId("GNOME_CHIEF"), 20.),
      make_pair(CreatureId("DWARF"), 100.),
      make_pair(CreatureId("DWARF_FEMALE"), 40.),
      make_pair(CreatureId("JACKAL"), 200.),
      make_pair(CreatureId("BAT"), 200.),
      make_pair(CreatureId("SNAKE"), 150.),
      make_pair(CreatureId("SPIDER"), 200.),
      make_pair(CreatureId("FLY"), 100.),
      make_pair(CreatureId("RAT"), 100.)},
      {}, {{CreatureId("GNOME"), peaceful}, {CreatureId("GNOME_CHIEF"), peaceful} });
}

#include "pretty_archive.h"
template
void CreatureGroup::serialize(PrettyInputArchive& ar1, unsigned);
