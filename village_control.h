#ifndef _VILLAGE_CONTROL_H
#define _VILLAGE_CONTROL_H

#include "event.h"
#include "monster_ai.h"

class VillageControl : public EventListener {
  public:
  virtual ~VillageControl();
  void addCreature(Creature* c, int attackPoints);
  void onEnteredDungeon(int attackTime);

  virtual MoveInfo getMove(Creature* c) = 0;
  bool startedAttack(Creature* c);

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;

  static VillageControl* humanVillage(Collective* villain, const Location* villageLocation, 
      StairDirection dir, StairKey key);
  static VillageControl* dwarfVillage(Collective* villain, const Level*, 
      StairDirection dir, StairKey key);

  protected:
  VillageControl(Collective* villain, const Level*, StairDirection, StairKey, string name);
  struct AttackInfo {
    vector<const Creature*> creatures;
    int attackTime = -1;
    bool msg = true;
  };
  map<int, AttackInfo> attackInfo;
  vector<const Creature*> allCreatures;
  int numAttacks = 0;
  unordered_set<int> messages;
  Collective* villain;
  const Level* level;
  StairDirection direction;
  StairKey stairKey;
  string name;
  Tribe* tribe = nullptr;
  int attackPoints = 0;
};

#endif
