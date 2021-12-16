#include "stdafx.h"
#include "inhabitants_info.h"
#include "monster_ai.h"
#include "minion_trait.h"
#include "item_type.h"
#include "creature.h"

SERIALIZE_DEF(InhabitantsInfo, OPTION(leader), OPTION(fighters), OPTION(civilians), OPTION(steeds))

auto InhabitantsInfo::generateCreatures(RandomGen& random, CreatureFactory* factory, TribeId tribe,
    MonsterAIFactory aiFactory) -> Generated {
  Generated ret;
  bool wasLeader = false;
  int numRiders = 0;
  auto addCreatures = [&](const CreatureList& info, EnumSet<MinionTrait> traits) {
    for (auto& creature : info.generate(random, factory, tribe, aiFactory, false)) {
      auto myTraits = traits;
      if (!wasLeader) {
        wasLeader = true;
        myTraits.insert(MinionTrait::LEADER);
      }
      if ((myTraits.contains(MinionTrait::FIGHTER) || myTraits.contains(MinionTrait::LEADER)) &&
          creature->isAffected(LastingEffect::RIDER))
        ++numRiders;
      ret.push_back(make_pair(std::move(creature), myTraits));
    }
  };
  addCreatures(leader, {});
  addCreatures(fighters, EnumSet<MinionTrait>{MinionTrait::FIGHTER});
  addCreatures(civilians, EnumSet<MinionTrait>{});
  if (steeds) {
    steeds->count = Range::singleElem(numRiders);
    addCreatures(*steeds, EnumSet<MinionTrait>{});
  }
  return ret;
}

namespace {
struct LeaderInfo {
  vector<CreatureId> SERIAL(id);
  EnumMap<ExperienceType, int> SERIAL(baseLevelIncrease);
  EnumMap<ExperienceType, int> SERIAL(expLevelIncrease);
  vector<ItemType> SERIAL(inventory);
  SERIALIZE_ALL(NAMED(id), OPTION(baseLevelIncrease), OPTION(expLevelIncrease), OPTION(inventory))
};
}

#include "pretty_archive.h"
template <>
void InhabitantsInfo::serialize(PrettyInputArchive& ar1, unsigned) {
  optional<LeaderInfo> leader;
  if (this->leader.count.contains(1))
    leader = LeaderInfo {
      this->leader.all.transform([](auto& elem) { return elem.second;}),
      this->leader.baseLevelIncrease,
      this->leader.expLevelIncrease,
      this->leader.inventory
    };
  ar1(OPTION(leader), OPTION(fighters), OPTION(civilians), OPTION(steeds));
  ar1(endInput());
  if (leader)
    this->leader = CreatureList(1, leader->id)
      .increaseExpLevel(leader->expLevelIncrease)
      .increaseBaseLevel(leader->baseLevelIncrease)
      .addInventory(leader->inventory);
  else
   this->leader = CreatureList();
}
