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

PCreature CreatureGroup::random() {
  return random(MonsterAIFactory::monster());
}

PCreature CreatureGroup::random(const MonsterAIFactory& actorFactory) {
  CreatureId id;
  if (unique.size() > 0) {
    id = unique.back();
    unique.pop_back();
  } else
    id = Random.choose(creatures, weights);
  PCreature ret = CreatureFactory::fromId(id, getTribeFor(id), actorFactory, inventory);
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
    const vector<CreatureId>& u, EnumMap<CreatureId, optional<TribeId>> overrides)
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
      make_pair(CreatureGroup(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER}, { 1, 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId::DUKE)),
      make_pair(CreatureGroup(tribe, { CreatureId::WARRIOR}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId::SHAMAN)),
      make_pair(CreatureGroup(tribe, { CreatureId::ELF_ARCHER}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId::ELF_LORD)),
      make_pair(CreatureGroup(tribe, { CreatureId::DWARF}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId::DWARF_BARON)),
      make_pair(CreatureGroup(tribe, { CreatureId::LIZARDMAN}, { 1}, {}),
        CreatureGroup::singleType(tribe, CreatureId::LIZARDLORD))
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
  return CreatureGroup(tribe, { CreatureId::GNOME, CreatureId::GOBLIN, CreatureId::OGRE,
      CreatureId::SPECIAL_HLBN, CreatureId::SPECIAL_BLBW, CreatureId::WOLF, CreatureId::CAVE_BEAR,
      CreatureId::BAT, CreatureId::WEREWOLF, CreatureId::ZOMBIE, CreatureId::VAMPIRE, CreatureId::DOPPLEGANGER,
      CreatureId::SUCCUBUS},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {}, {}).increaseBaseLevel(ExperienceType::MELEE, 25);
}

CreatureGroup CreatureGroup::forrest(TribeId tribe) {
  return CreatureGroup(tribe,
      { CreatureId::DEER, CreatureId::FOX, CreatureId::BOAR },
      { 4, 2, 2}, {});
}

CreatureGroup CreatureGroup::waterCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId::WATER_ELEMENTAL, CreatureId::KRAKEN }, {20, 1}, {CreatureId::KRAKEN});
}

CreatureGroup CreatureGroup::elementals(TribeId tribe) {
  return CreatureGroup(tribe, {CreatureId::AIR_ELEMENTAL, CreatureId::FIRE_ELEMENTAL, CreatureId::WATER_ELEMENTAL,
      CreatureId::EARTH_ELEMENTAL}, {1, 1, 1, 1}, {});
}

CreatureGroup CreatureGroup::lavaCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId::FIRE_ELEMENTAL }, {1}, { });
}

CreatureGroup CreatureGroup::singleType(TribeId tribe, CreatureId id) {
  return CreatureGroup(tribe, { id}, {1}, {});
}

CreatureGroup CreatureGroup::gnomishMines(TribeId peaceful, TribeId enemy, int level) {
  return CreatureGroup({
      make_tuple(CreatureId::BANDIT, 100., enemy),
      make_tuple(CreatureId::GREEN_DRAGON, 5., enemy),
      make_tuple(CreatureId::RED_DRAGON, 5., enemy),
      make_tuple(CreatureId::SOFT_MONSTER, 5., enemy),
      make_tuple(CreatureId::CYCLOPS, 15., enemy),
      make_tuple(CreatureId::WITCH, 15., enemy),
      make_tuple(CreatureId::CLAY_GOLEM, 20., enemy),
      make_tuple(CreatureId::STONE_GOLEM, 20., enemy),
      make_tuple(CreatureId::IRON_GOLEM, 20., enemy),
      make_tuple(CreatureId::LAVA_GOLEM, 20., enemy),
      make_tuple(CreatureId::FIRE_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::WATER_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::EARTH_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::AIR_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::GNOME, 100., peaceful),
      make_tuple(CreatureId::GNOME_CHIEF, 20., peaceful),
      make_tuple(CreatureId::DWARF, 100., enemy),
      make_tuple(CreatureId::DWARF_FEMALE, 40., enemy),
      make_tuple(CreatureId::JACKAL, 200., enemy),
      make_tuple(CreatureId::BAT, 200., enemy),
      make_tuple(CreatureId::SNAKE, 150., enemy),
      make_tuple(CreatureId::SPIDER, 200., enemy),
      make_tuple(CreatureId::FLY, 100., enemy),
      make_tuple(CreatureId::RAT, 100., enemy)});
}
