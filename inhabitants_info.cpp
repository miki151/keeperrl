#include "stdafx.h"
#include "inhabitants_info.h"
#include "creature_factory.h"
#include "monster_ai.h"
#include "minion_trait.h"

auto InhabitantsInfo::generateCreatures(RandomGen& random, TribeId tribe, MonsterAIFactory aiFactory) -> CreatureList {
  CreatureList ret;
  bool wasLeader = false;
  if (leader) {
    ret.push_back(make_pair(CreatureFactory::fromId(*leader, tribe, aiFactory),
        EnumSet<MinionTrait>{MinionTrait::LEADER}));
    wasLeader = true;
  }
  auto addCreatures = [&](CreaturesInfo& info, EnumSet<MinionTrait> traits) {
    for (int i : Range(info.first)) {
      auto creature = CreatureFactory::fromId(random.choose(info.second), tribe, aiFactory);
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
