#pragma once

#include "util.h"
#include "view_id.h"
#include "attr_type.h"

class TribeId;
class MonsterAIFactory;
class ItemType;

struct CreatureList {
  CreatureList();
  ~CreatureList();
  CreatureList(const CreatureList&);
  CreatureList(CreatureList&&) noexcept;
  CreatureList& operator = (const CreatureList&);
  CreatureList& operator = (CreatureList&&);
  CreatureList(int count, CreatureId);
  explicit CreatureList(CreatureId);
  CreatureList(int count, vector<CreatureId>);
  CreatureList(int count, vector<pair<int, CreatureId>>);
  CreatureList& addUnique(CreatureId);
  CreatureList& setCombatExperience(int);
  CreatureList& increaseExpLevel(const HashMap<AttrType, int>&);
  CreatureList& addInventory(vector<ItemType>);

  string getSummary(CreatureFactory* factory) const;

  ViewIdList getViewId(CreatureFactory*) const;
  vector<PCreature> generate(RandomGen&, CreatureFactory*, TribeId, MonsterAIFactory, bool nonUnique = false) const;

  template<typename Archive>
  void serialize(Archive&, const unsigned int);

  //private:
  Range SERIAL(count) = Range(0, 1);
  vector<CreatureId> SERIAL(uniques);
  using Freq = pair<int, CreatureId>;
  vector<Freq> SERIAL(all);
  int SERIAL(combatExperience) = 0;
  HashMap<AttrType, int> SERIAL(expLevelIncrease);
  vector<ItemType> SERIAL(inventory);
};
