#ifndef _VILLAGE_CONTROL_H
#define _VILLAGE_CONTROL_H

#include "event.h"
#include "monster_ai.h"
#include "view.h"

class VillageControl : public EventListener {
  public:
  virtual ~VillageControl();
  void addCreature(Creature* c);

  virtual MoveInfo getMove(Creature* c) = 0;

  void tick(double time);

  bool isConquered() const;
  vector<Creature*> getAliveCreatures() const;
  bool currentlyAttacking() const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;

  View::GameInfo::VillageInfo::Village getVillageInfo() const;

  class AttackTrigger {
    public:
    virtual void tick(double time) = 0;
    bool startedAttack(const Creature*);
    void setVillageControl(VillageControl*);

    virtual ~AttackTrigger() {}

    SERIALIZATION_DECL(AttackTrigger);

    protected:
    set<const Creature*> SERIAL(fightingCreatures);
    VillageControl* SERIAL2(control, nullptr);
  };

  static AttackTrigger* getPowerTrigger(double killedCoeff, double powerCoeff);
  static AttackTrigger* getFinalTrigger(vector<VillageControl*> otherControls);

  friend class AttackTrigger;
  friend class PowerTrigger;
  friend class FinalTrigger;

  static PVillageControl topLevelVillage(Collective* villain, const Location* villageLocation, AttackTrigger*);
  static PVillageControl dwarfVillage(Collective* villain, const Level*, 
      StairDirection dir, StairKey key, AttackTrigger*);

  SERIALIZATION_DECL(VillageControl);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  VillageControl(Collective* villain, const Level*, string name, AttackTrigger*);
  vector<Creature*> SERIAL(allCreatures);
  Collective* SERIAL2(villain, nullptr);
  const Level* SERIAL2(level, nullptr);
  string SERIAL(name);
  Tribe* SERIAL2(tribe, nullptr);
  unique_ptr<AttackTrigger> SERIAL(attackTrigger);
  bool SERIAL2(atWar, false);
};

#endif
