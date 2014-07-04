#ifndef _COLLECTIVE_CONTROL_H
#define _COLLECTIVE_CONTROL_H

#include "monster_ai.h"

class Creature;
class Collective;

class CollectiveControl {
  public:
  CollectiveControl(Collective*);
  virtual MoveInfo getMove(Creature*) = 0;
  virtual void tick(double time) = 0;

  SERIALIZATION_DECL(CollectiveControl);

  virtual ~CollectiveControl();

  static PCollectiveControl idle(Collective*);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Collective* getCollective();
  const Collective* getCollective() const;

  private:
  Collective* SERIAL2(collective, nullptr);
};


#endif
