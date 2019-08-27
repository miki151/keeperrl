#pragma once

#include "creature_factory.h"

class CreatureGroup {
  public:
  static CreatureGroup singleType(TribeId, CreatureId);

  static CreatureGroup splashHeroes(TribeId);
  static CreatureGroup splashLeader(TribeId);
  static CreatureGroup splashMonsters(TribeId);
  static CreatureGroup forrest(TribeId);
  static CreatureGroup snow(TribeId);
  static CreatureGroup lavaCreatures(TribeId tribe);
  static CreatureGroup waterCreatures(TribeId tribe);
  static CreatureGroup iceCreatures(TribeId tribe);

  PCreature random(CreatureFactory*, const MonsterAIFactory&);
  PCreature random(CreatureFactory*);

  CreatureGroup& increaseBaseLevel(ExperienceType, int);
  CreatureGroup& addInventory(vector<ItemType>);

  ~CreatureGroup();
  CreatureGroup& operator = (const CreatureGroup&);
  CreatureGroup(const CreatureGroup&);

  SERIALIZATION_DECL(CreatureGroup)

  private:
  CreatureGroup(TribeId tribe, const vector<CreatureId>& creatures, const vector<double>& weights,
      const vector<CreatureId>& unique = {}, map<CreatureId, optional<TribeId>> overrides = {});
  CreatureGroup(TribeId tribe, const vector<pair<CreatureId, double>>& creatures,
      const vector<CreatureId>& unique = {}, map<CreatureId, optional<TribeId>> overrides = {});
  static void initSplash(TribeId);
  TribeId getTribeFor(CreatureId);
  optional<TribeId> SERIAL(tribe);
  vector<CreatureId> SERIAL(creatures);
  vector<double> SERIAL(weights);
  vector<CreatureId> SERIAL(unique);
  map<CreatureId, optional<TribeId>> SERIAL(tribeOverrides);
  EnumMap<ExperienceType, int> SERIAL(baseLevelIncrease);
  vector<ItemType> SERIAL(inventory);
};
