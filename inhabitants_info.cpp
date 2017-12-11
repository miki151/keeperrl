#include "stdafx.h"
#include "inhabitants_info.h"
#include "monster_ai.h"
#include "minion_trait.h"

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
      auto myTraits = traits;
      if (!wasLeader) {
        wasLeader = true;
        myTraits.insert(MinionTrait::LEADER);
      }
      ret.push_back(make_pair(std::move(creature), myTraits));
    }
  };
  addCreatures(fighters, EnumSet<MinionTrait>{MinionTrait::FIGHTER});
  addCreatures(civilians, EnumSet<MinionTrait>{});
  return ret;
}
