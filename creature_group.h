#pragma once

#include "creature_factory.h"

class CreatureGroup {
  public:
  static CreatureGroup singleCreature(TribeId, CreatureId);

  static CreatureGroup splashHeroes(TribeId);
  static CreatureGroup splashLeader(TribeId);
  static CreatureGroup splashMonsters(TribeId);
  static CreatureGroup forrest(TribeId);
  static CreatureGroup singleType(TribeId, CreatureId);
  static CreatureGroup lavaCreatures(TribeId tribe);
  static CreatureGroup waterCreatures(TribeId tribe);
  static CreatureGroup elementals(TribeId tribe);
  static CreatureGroup gnomishMines(TribeId peaceful, TribeId enemy, int level);

  PCreature random(const CreatureFactory*, const MonsterAIFactory&);
  PCreature random(const CreatureFactory*);

  CreatureGroup& increaseBaseLevel(ExperienceType, int);
  CreatureGroup& increaseLevel(ExperienceType, int);
  CreatureGroup& increaseLevel(EnumMap<ExperienceType, int>);
  CreatureGroup& addInventory(vector<ItemType>);

  ~CreatureGroup();
  CreatureGroup& operator = (const CreatureGroup&);
  CreatureGroup(const CreatureGroup&);

  SERIALIZATION_DECL(CreatureGroup)

  private:
  CreatureGroup(TribeId tribe, const vector<CreatureId>& creatures, const vector<double>& weights,
      const vector<CreatureId>& unique = {}, EnumMap<CreatureId, optional<TribeId>> overrides = {});
  CreatureGroup(const vector<tuple<CreatureId, double, TribeId>>& creatures,
      const vector<CreatureId>& unique = {});
  static void initSplash(TribeId);
  TribeId getTribeFor(CreatureId);
  optional<TribeId> SERIAL(tribe);
  vector<CreatureId> SERIAL(creatures);
  vector<double> SERIAL(weights);
  vector<CreatureId> SERIAL(unique);
  EnumMap<CreatureId, optional<TribeId>> SERIAL(tribeOverrides);
  EnumMap<ExperienceType, int> SERIAL(baseLevelIncrease);
  EnumMap<ExperienceType, int> SERIAL(levelIncrease);
  vector<ItemType> SERIAL(inventory);
};
