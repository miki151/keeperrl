#pragma once

#include "util.h"
#include "entity_set.h"
#include "position.h"

class TaskCallback : public OwnedObject<TaskCallback> {
  public:
  virtual void onConstructed(Position, FurnitureType) {}
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) {}
  virtual bool isConstructionReachable(Position) { return true; }
  virtual void onAppliedItem(Position, WItem item) {}
  virtual void onAppliedSquare(WCreature, Position) {}
  virtual void onAppliedItemCancel(Position) {}
  virtual void onTaskPickedUp(Position, EntitySet<Item>) {}
  virtual void onBrought(Position, EntitySet<Item>) {}
  virtual void onCantPickItem(EntitySet<Item> items) {}
  virtual void onKillCancelled(WCreature) {}
  virtual void onCopulated(WCreature who, WCreature with) {}

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int) {
    ar & SUBCLASS(OwnedObject<TaskCallback>);
  }

  virtual ~TaskCallback() {}
};

