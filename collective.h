#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "monster_ai.h"
#include "task.h"
#include "event.h"

class Creature;
class CollectiveControl;
class Tribe;
class Deity;

RICH_ENUM(MinionTrait,
  FIGHTER,
);

class Collective : public EventListener {
  public:
  Collective();
  void addCreature(Creature*);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void tick(double time);
  const Tribe* getTribe() const;
  Tribe* getTribe();
  double getStanding(const Deity*) const;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onWorshipEvent(Creature* who, const Deity*, WorshipType) override;
  virtual void onAttackEvent(Creature* victim, Creature* attacker) override;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;

  vector<Creature*>& getCreatures(MinionTrait);
  const vector<Creature*>& getCreatures(MinionTrait) const;
  void setTrait(Creature* c, MinionTrait);

  double getTechCostMultiplier() const;
  double getCraftingMultiplier() const;
  double getWarMultiplier() const;
  double getBeastMultiplier() const;
  double getUndeadMultiplier() const;

  double getMorale(const Creature*) const;
  void addMorale(const Creature*, double);
  double getEfficiency(const Creature*) const;
  const Creature* getLeader() const;
  Creature* getLeader();
  void setLeader(Creature*);

  ~Collective();

  private:
  double getStanding(EpithetId id) const;
  void onEpithetWorship(Creature*, WorshipType, EpithetId);
  void considerHealingLeader();
  vector<Creature*> SERIAL(creatures);
  Creature* SERIAL2(leader, nullptr);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  PCollectiveControl SERIAL(control);
  Task::Mapping SERIAL(taskMap);
  Tribe* SERIAL2(tribe, nullptr);
  map<const Deity*, double> SERIAL(deityStanding);
  map<const Creature*, double> SERIAL(morale);
};

#endif
