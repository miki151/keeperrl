#include "stdafx.h"
#include "inhabitants_info.h"
#include "monster_ai.h"
#include "minion_trait.h"

SERIALIZE_DEF(InhabitantsInfo, NAMED(leader), NAMED(fighters), NAMED(civilians))

auto InhabitantsInfo::generateCreatures(RandomGen& random, CreatureFactory* factory, TribeId tribe,
    MonsterAIFactory aiFactory) -> Generated {
  Generated ret;
  bool wasLeader = false;
  auto addCreatures = [&](const CreatureList& info, EnumSet<MinionTrait> traits) {
    for (auto& creature : info.generate(random, factory, tribe, aiFactory)) {
      auto myTraits = traits;
      if (!wasLeader) {
        wasLeader = true;
        myTraits.insert(MinionTrait::LEADER);
      }
      ret.push_back(make_pair(std::move(creature), myTraits));
    }
  };
  addCreatures(leader, {});
  addCreatures(fighters, EnumSet<MinionTrait>{MinionTrait::FIGHTER});
  addCreatures(civilians, EnumSet<MinionTrait>{});
  return ret;
}

#include "pretty_archive.h"
template
void InhabitantsInfo::serialize(PrettyInputArchive& ar1, unsigned);
