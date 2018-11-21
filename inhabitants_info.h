#pragma once

#include "stdafx.h"
#include "creature_factory.h"
#include "creature_list.h"

struct InhabitantsInfo {
  struct Unique {};
  CreatureList leader;
  CreatureList fighters;
  CreatureList civilians;
  using Generated = vector<pair<PCreature, EnumSet<MinionTrait>>>;
  Generated generateCreatures(RandomGen&, TribeId, MonsterAIFactory);
};
