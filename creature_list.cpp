#include "stdafx.h"
#include "creature_list.h"
#include "creature_factory.h"
#include "tribe.h"
#include "monster_ai.h"
#include "creature.h"
#include "creature_attributes.h"
#include "item_type.h"
#include "view_id.h"

SERIALIZE_DEF(CreatureList, count, uniques, all, baseLevelIncrease, inventory)


CreatureList::CreatureList() {}

CreatureList::~CreatureList() {}

CreatureList& CreatureList::operator =(CreatureList&&) =default;
CreatureList& CreatureList::operator =(const CreatureList&) = default;
CreatureList::CreatureList(CreatureList&&) = default;
CreatureList::CreatureList(const CreatureList&) = default;

CreatureList::CreatureList(int c, CreatureId id) : count(c), all(1, make_pair(1, id)) {

}

CreatureList::CreatureList(int c, vector<CreatureId> ids) : count(c),
    all(ids.transform([](const CreatureId id) { return make_pair(1, id); })) {
}

CreatureList::CreatureList(int c, vector<pair<int, CreatureId>> ids) : count(c), all(ids) {}

string CreatureList::getSummary() const {
  auto ret = toLower(EnumInfo<ViewId>::getString(getViewId()));
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

CreatureList& CreatureList::increaseBaseLevel(EnumMap<ExperienceType, int> l) {
  for (auto exp : ENUM_ALL(ExperienceType))
    baseLevelIncrease[exp] += l[exp];
  return *this;
}

CreatureList& CreatureList::addUnique(CreatureId id) {
  uniques.push_back(id);
  return *this;
}

ViewId CreatureList::getViewId() const {
  if (!uniques.empty())
    return CreatureFactory::getViewId(uniques[0]);
  else
    return CreatureFactory::getViewId(all[0].second);
}

vector<PCreature> CreatureList::generate(RandomGen& random, TribeId tribe, MonsterAIFactory aiFactory) const {
  vector<PCreature> ret;
  vector<CreatureId> uniquesCopy = uniques;
  for (int i : Range(count)) {
    optional<CreatureId> id;
    if (!uniquesCopy.empty()) {
      id = uniquesCopy.back();
      uniquesCopy.pop_back();
    } else
      id = random.choose(all);
    auto creature = CreatureFactory::fromId(*id, tribe, aiFactory, inventory);
    for (auto exp : ENUM_ALL(ExperienceType))
      creature->getAttributes().increaseBaseExpLevel(exp, baseLevelIncrease[exp]);
    ret.push_back(std::move(creature));
  }
  return ret;
}
