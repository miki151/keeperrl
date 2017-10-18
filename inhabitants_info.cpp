#include "stdafx.h"
#include "inhabitants_info.h"
#include "creature_factory.h"
#include "monster_ai.h"
#include "minion_trait.h"

SERIALIZE_DEF(CreatureList, count, uniques, all)


CreatureList::CreatureList() {}

CreatureList::CreatureList(int c, CreatureId id) : count(c), all(1, make_pair(1, id)) {

}

CreatureList::CreatureList(int c, vector<CreatureId> ids) : count(c),
    all(ids.transform([](const CreatureId id) { return make_pair(1, id); })) {
}

CreatureList::CreatureList(int c, vector<pair<int, CreatureId>> ids) : count(c), all(ids) {}

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
    auto creature = CreatureFactory::fromId(*id, tribe, aiFactory);
    ret.push_back(std::move(creature));
  }
  return ret;
}

auto InhabitantsInfo::generateCreatures(RandomGen& random, TribeId tribe, MonsterAIFactory aiFactory) -> Generated {
  Generated ret;
  bool wasLeader = false;
  if (leader) {
    ret.push_back(make_pair(CreatureFactory::fromId(*leader, tribe, aiFactory),
        EnumSet<MinionTrait>{MinionTrait::LEADER}));
    wasLeader = true;
  }
  auto addCreatures = [&](const CreatureList& info, EnumSet<MinionTrait> traits) {
    for (auto& creature : info.generate(random, tribe, aiFactory)) {
      if (!wasLeader) {
        wasLeader = true;
        traits.insert(MinionTrait::LEADER);
      }
      ret.push_back(make_pair(std::move(creature), traits));
    }
  };
  addCreatures(fighters, EnumSet<MinionTrait>{MinionTrait::FIGHTER});
  addCreatures(civilians, EnumSet<MinionTrait>{});
  return ret;
}
