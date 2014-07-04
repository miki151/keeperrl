#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "monster_ai.h"

class Creature;
class CollectiveControl;

enum class MinionTrait {
  FIGHTER,
  
  ENUM_END
};

class Collective {
  public:
  Collective();
  void addCreature(Creature*);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void tick(double time);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;

  vector<Creature*>& getCreatures(MinionTrait);
  const vector<Creature*>& getCreatures(MinionTrait) const;
  void setTrait(Creature* c, MinionTrait);

  private:
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> byTrait;
  PCollectiveControl SERIAL(control);
};

#endif
