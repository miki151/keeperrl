#ifndef _VILLAGE_CONTROL_H
#define _VILLAGE_CONTROL_H

#include "event.h"
#include "monster_ai.h"

class VillageControl : public EventListener {
  public:
  virtual ~VillageControl();
  void addCreature(Creature* c);

  virtual MoveInfo getMove(Creature* c) = 0;

  void tick(double time);

  bool isConquered() const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;

  static PVillageControl topLevelVillage(Collective* villain, const Location* villageLocation,
      double killedCoeff, double powerCoeff);
  static PVillageControl dwarfVillage(Collective* villain, const Level*, 
      StairDirection dir, StairKey key, double killedCoeff, double powerCoeff);

  SERIALIZATION_DECL(VillageControl);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  VillageControl(Collective* villain, const Level*, string name, double killedCoeff, double powerCoeff);
  void calculateAttacks();
  double getCurrentTrigger();
  bool startedAttack(Creature* creature);
  vector<const Creature*> allCreatures;
  vector<const Creature*> aliveCreatures;
  Collective* villain = nullptr;
  const Level* level = nullptr;
  string name;
  Tribe* tribe = nullptr;
  double killedPoints = 0;
  double killedCoeff;
  double powerCoeff;
  double lastAttack = 0;
  double lastMyAttack = 0;
  bool lastAttackLaunched = false;
  set<double> triggerAmounts;
  set<const Creature*> fightingCreatures;
};

#endif
