#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "monster_ai.h"
#include "task.h"
#include "event.h"

class Creature;
class CollectiveControl;
class Tribe;

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

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;

  vector<Creature*>& getCreatures(MinionTrait);
  const vector<Creature*>& getCreatures(MinionTrait) const;
  void setTrait(Creature* c, MinionTrait);

  ~Collective();

  private:
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  PCollectiveControl SERIAL(control);
  Task::Mapping SERIAL(taskMap);
  Tribe* SERIAL2(tribe, nullptr);
};

#endif
