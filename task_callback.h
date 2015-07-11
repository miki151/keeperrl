#ifndef _TASK_CALLBACK_H
#define _TASK_CALLBACK_H

#include "util.h"
#include "entity_set.h"
#include "position.h"

class SquareType;

class TaskCallback {
  public:
  virtual void onConstructed(Position, const SquareType&) {}
  virtual bool isConstructionReachable(Position) { return true; }
  virtual void onTorchBuilt(Position, Trigger*) {}
  virtual void onAppliedItem(Position, Item* item) {}
  virtual void onAppliedSquare(Position) {}
  virtual void onAppliedItemCancel(Position) {}
  virtual void onPickedUp(Position, EntitySet<Item>) {}
  virtual void onBrought(Position, EntitySet<Item>) {}
  virtual void onCantPickItem(EntitySet<Item> items) {}
  virtual void onKillCancelled(Creature*) {}
  virtual void onBedCreated(Position, const SquareType& fromType, const SquareType& toType) {}
  virtual void onCopulated(Creature* who, Creature* with) {}
  virtual void onConsumed(Creature* consumer, Creature* who) {}

  template <class Archive> 
  void serialize(Archive&, const unsigned int) {}
};

#endif
