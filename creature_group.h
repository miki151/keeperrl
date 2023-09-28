#pragma once

#include "creature_factory.h"

class CreatureGroup {
  public:
  static CreatureGroup singleType(TribeId, CreatureId);

  static CreatureGroup lavaCreatures(TribeId tribe);
  static CreatureGroup waterCreatures(TribeId tribe);
  static CreatureGroup iceCreatures(TribeId tribe);

  PCreature random(CreatureFactory*, const MonsterAIFactory&);
  PCreature random(CreatureFactory*);

  CreatureGroup& setCombatExperience(int);

  ~CreatureGroup();
  CreatureGroup& operator = (const CreatureGroup&);
  CreatureGroup(const CreatureGroup&);

  private:
  CreatureGroup(TribeId tribe, const vector<CreatureId>& creatures, const vector<double>& weights,
      const vector<CreatureId>& unique = {}, map<CreatureId, optional<TribeId>> overrides = {});
  CreatureGroup(TribeId tribe, const vector<pair<CreatureId, double>>& creatures,
      const vector<CreatureId>& unique = {}, map<CreatureId, optional<TribeId>> overrides = {});
  TribeId getTribeFor(CreatureId);
  optional<TribeId> tribe;
  vector<CreatureId> creatures;
  vector<double> weights;
  vector<CreatureId> unique;
  map<CreatureId, optional<TribeId>> tribeOverrides;
  int combatExperience = 0;
};
