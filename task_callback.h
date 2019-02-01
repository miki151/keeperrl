#pragma once

#include "util.h"
#include "entity_set.h"
#include "position.h"

class TaskCallback : public OwnedObject<TaskCallback> {
  public:
  virtual void onConstructed(Position, FurnitureType) {}
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) {}
  virtual bool isConstructionReachable(Position) { return true; }
  virtual void onAppliedItem(Position, Item* item) {}
  virtual void onAppliedSquare(Creature*, Position) {}
  virtual void onCopulated(Creature* who, Creature* with) {}

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int) {
    ar & SUBCLASS(OwnedObject<TaskCallback>);
  }

  virtual ~TaskCallback() {}
};

