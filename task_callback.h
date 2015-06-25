#ifndef _TASK_CALLBACK_H
#define _TASK_CALLBACK_H

#include "util.h"
#include "entity_set.h"

class SquareType;

class TaskCallback {
  public:
  virtual void onConstructed(Vec2 pos, const SquareType&) {}
  virtual void onConstructionCancelled(Vec2 pos) {}
  virtual bool isConstructionReachable(Vec2 pos) { return true; }
  virtual void onTorchBuilt(Vec2 pos, Trigger*) {}
  virtual void onAppliedItem(Vec2 pos, Item* item) {}
  virtual void onAppliedSquare(Vec2 pos) {}
  virtual void onAppliedItemCancel(Vec2 pos) {}
  virtual void onPickedUp(Vec2 pos, EntitySet<Item>) {}
  virtual void onBrought(Vec2 pos, EntitySet<Item>) {}
  virtual void onCantPickItem(EntitySet<Item> items) {}
  virtual void onKillCancelled(Creature*) {}
  virtual void onBedCreated(Vec2 pos, const SquareType& fromType, const SquareType& toType) {}
  virtual void onCopulated(Creature* who, Creature* with) {}
  virtual void onConsumed(Creature* consumer, Creature* who) {}

  template <class Archive> 
  void serialize(Archive&, const unsigned int) {}
};

#endif
