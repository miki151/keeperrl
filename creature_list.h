#pragma once

#include "util.h"
#include "experience_type.h"

class TribeId;
class MonsterAIFactory;
class ItemType;

struct CreatureList {
  CreatureList();
  ~CreatureList();
  CreatureList(const CreatureList&);
  CreatureList(CreatureList&&);
  CreatureList& operator = (const CreatureList&);
  CreatureList& operator = (CreatureList&&);
  CreatureList(int count, CreatureId);
  CreatureList(CreatureId);
  CreatureList(int count, vector<CreatureId>);
  CreatureList(int count, vector<pair<int, CreatureId>>);
  CreatureList& addUnique(CreatureId);
  CreatureList& increaseBaseLevel(EnumMap<ExperienceType, int>);
  CreatureList& addInventory(vector<ItemType>);

  string getSummary() const;

  ViewId getViewId() const;
  vector<PCreature> generate(RandomGen&, TribeId, MonsterAIFactory) const;

  template<typename Archive>
  void serialize(Archive&, const unsigned int);

  private:
  int SERIAL(count) = 0;
  vector<CreatureId> SERIAL(uniques);
  using Freq = pair<int, CreatureId>;
  vector<Freq> SERIAL(all);
  EnumMap<ExperienceType, int> SERIAL(baseLevelIncrease);
  vector<ItemType> SERIAL(inventory);
};
