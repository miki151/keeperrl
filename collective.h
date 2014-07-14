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

  ~Collective();

  private:
  double getStanding(EpithetId id) const;
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  PCollectiveControl SERIAL(control);
  Task::Mapping SERIAL(taskMap);
  Tribe* SERIAL2(tribe, nullptr);
  map<const Deity*, double> SERIAL(deityStanding);
};

#endif
