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
  vector<const Creature*> getAliveCreatures() const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;

  class AttackTrigger {
    public:
    virtual void tick(double time) = 0;
    bool startedAttack(const Creature*);
    void setVillageControl(VillageControl*);

    virtual ~AttackTrigger() {}

    SERIALIZATION_DECL(AttackTrigger);

    protected:
    set<const Creature*> fightingCreatures;
    VillageControl* control = nullptr;
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
  vector<const Creature*> allCreatures;
  Collective* villain = nullptr;
  const Level* level = nullptr;
  string name;
  Tribe* tribe = nullptr;
  unique_ptr<AttackTrigger> attackTrigger;
};

#endif
