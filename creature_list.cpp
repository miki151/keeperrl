#include "stdafx.h"
#include "creature_list.h"
#include "creature_factory.h"
#include "tribe.h"
#include "monster_ai.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item_type.h"
#include "view_id.h"

SERIALIZE_DEF(CreatureList, OPTION(count), OPTION(uniques), NAMED(all), OPTION(baseLevelIncrease), OPTION(expLevelIncrease), OPTION(inventory))


CreatureList::CreatureList() {}

CreatureList::~CreatureList() {}

CreatureList& CreatureList::operator =(CreatureList&&) =default;
CreatureList& CreatureList::operator =(const CreatureList&) = default;
CreatureList::CreatureList(CreatureList&&) noexcept = default;
CreatureList::CreatureList(const CreatureList&) = default;

CreatureList::CreatureList(int c, CreatureId id) : count(c, c + 1), all(1, make_pair(1, id)) {
}

CreatureList::CreatureList(CreatureId id) : CreatureList(1, id) {
}

CreatureList::CreatureList(int c, vector<CreatureId> ids) : count(c, c + 1),
    all(ids.transform([](const CreatureId id) { return make_pair(1, id); })) {
}

CreatureList::CreatureList(int c, vector<pair<int, CreatureId>> ids) : count(c, c + 1), all(ids) {}

string CreatureList::getSummary(CreatureFactory* factory) const {
  string ret;
  if (!uniques.empty())
    ret = uniques[0].data();
  else
    ret = all[0].second.data();
  int inc = 0;
  for (auto exp : ENUM_ALL(ExperienceType))
    inc = max(inc, baseLevelIncrease[exp]);
  ret += " " + toString(inc);
  return ret;
}

CreatureList& CreatureList::addInventory(vector<ItemType> v) {
  inventory = v;
  return *this;
}

CreatureList& CreatureList::clearBaseLevel() {
  baseLevelIncrease.clear();
  return *this;
}

CreatureList& CreatureList::clearExpLevel() {
  expLevelIncrease.clear();
  return *this;
}

CreatureList& CreatureList::increaseBaseLevel(EnumMap<ExperienceType, int> l) {
  for (auto exp : ENUM_ALL(ExperienceType))
    baseLevelIncrease[exp] += l[exp];
  return *this;
}

CreatureList& CreatureList::increaseExpLevel(EnumMap<ExperienceType, int> l) {
  for (auto exp : ENUM_ALL(ExperienceType))
    expLevelIncrease[exp] += l[exp];
  return *this;
}

CreatureList& CreatureList::addUnique(CreatureId id) {
  uniques.push_back(id);
  if (count.getStart() < uniques.size())
    count = Range(uniques.size(), max(count.getEnd(), uniques.size() + 1));
  return *this;
}

ViewIdList CreatureList::getViewId(CreatureFactory* factory) const {
  if (!uniques.empty())
    return factory->getViewId(uniques[0]);
  else
    return factory->getViewId(all[0].second);
}

vector<PCreature> CreatureList::generate(RandomGen& random, CreatureFactory* factory, TribeId tribe,
    MonsterAIFactory aiFactory, bool nonUnique) const {
  vector<PCreature> ret;
  vector<CreatureId> uniquesCopy = uniques;
  if (nonUnique)
    uniquesCopy.clear();
  for (int i : Range(random.get(count))) {
    optional<CreatureId> id;
    if (!uniquesCopy.empty()) {
      id = uniquesCopy.back();
      uniquesCopy.pop_back();
    } else
      id = random.choose(all);
    auto creature = factory->fromId(*id, tribe, aiFactory, inventory);
    for (auto exp : ENUM_ALL(ExperienceType)) {
      creature->getAttributes().increaseBaseExpLevel(exp, baseLevelIncrease[exp]);
      creature->increaseExpLevel(exp, expLevelIncrease[exp]);
    }
    ret.push_back(std::move(creature));
  }
  return ret;
}

#include "pretty_archive.h"
template
void CreatureList::serialize(PrettyInputArchive& ar1, unsigned);
