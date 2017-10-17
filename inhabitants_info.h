#pragma once

#include "creature_factory.h"

struct InhabitantsInfo {
  using CreatureFreq = pair<int, CreatureId>;
  using CreaturesInfo = pair<int, vector<CreatureFreq>>;
  optional<CreatureId> leader;
  CreaturesInfo fighters;
  CreaturesInfo civilians;
  using CreatureList = vector<pair<PCreature, EnumSet<MinionTrait>>>;
  CreatureList generateCreatures(RandomGen&, TribeId, MonsterAIFactory);
};
