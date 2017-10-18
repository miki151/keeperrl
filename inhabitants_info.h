#pragma once

#include "stdafx.h"
#include "creature_factory.h"

struct CreatureList {
  CreatureList();
  CreatureList(int count, CreatureId);
  CreatureList(int count, vector<CreatureId>);
  CreatureList(int count, vector<pair<int, CreatureId>>);
  CreatureList& addUnique(CreatureId);
  ViewId getViewId() const;
  vector<PCreature> generate(RandomGen&, TribeId, MonsterAIFactory) const;

  template<typename Archive>
  void serialize(Archive&, const unsigned int);

  private:
  int SERIAL(count) = 0;
  vector<CreatureId> SERIAL(uniques);
  using Freq = pair<int, CreatureId>;
  vector<Freq> SERIAL(all);
};

struct InhabitantsInfo {
  struct Unique {};
  optional<CreatureId> leader;
  CreatureList fighters;
  CreatureList civilians;
  using Generated = vector<pair<PCreature, EnumSet<MinionTrait>>>;
  Generated generateCreatures(RandomGen&, TribeId, MonsterAIFactory);
};
