#include "stdafx.h"
#include "creature_group.h"
#include "monster_ai.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item_type.h"
#include "tribe.h"


CreatureGroup CreatureGroup::singleType(TribeId tribe, CreatureId id) {
  return CreatureGroup(tribe, { id}, {1}, {});
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
  PCreature ret = creatureFactory->fromId(id, getTribeFor(id), actorFactory);
  for (auto exp : ENUM_ALL(ExperienceType))
    ret->getAttributes().increaseBaseExpLevel(exp, baseLevelIncrease[exp]);
  return ret;
}

CreatureGroup& CreatureGroup::increaseBaseLevel(ExperienceType t, int l) {
  baseLevelIncrease[t] = l;
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

CreatureGroup CreatureGroup::iceCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId("WATER_ELEMENTAL") }, {1});
}

CreatureGroup CreatureGroup::waterCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId("WATER_ELEMENTAL") }, {1}, {CreatureId("KRAKEN")});
}

CreatureGroup CreatureGroup::lavaCreatures(TribeId tribe) {
  return CreatureGroup(tribe, { CreatureId("FIRE_ELEMENTAL") }, {1}, { });
}
