#pragma once

#include "stdafx.h"
#include "creature_factory.h"
#include "creature_list.h"

struct InhabitantsInfo {
  struct Unique {};
  CreatureList SERIAL(leader);
  CreatureList SERIAL(fighters);
  CreatureList SERIAL(civilians);
  optional<CreatureList> SERIAL(steeds);
  template <typename Archive>
  void serialize(Archive&, unsigned int);
  using Generated = vector<pair<PCreature, EnumSet<MinionTrait>>>;
  Generated generateCreatures(RandomGen&, CreatureFactory*, TribeId, MonsterAIFactory);
};
