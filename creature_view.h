#ifndef _CREATURE_VIEW_H
#define _CREATURE_VIEW_H

#include "map_memory.h"
#include "view.h"

class CreatureView {
  public:
  virtual const MapMemory& getMemory(const Level* l) const = 0;
  virtual ViewIndex getViewIndex(Vec2 pos) const = 0;
  virtual void refreshGameInfo(View::GameInfo&) const = 0;
  virtual Vec2 getPosition() const = 0;
  virtual bool staticPosition() const { return true; }
  virtual bool canSee(const Creature*) const = 0;
  virtual bool canSee(Vec2 position) const = 0;
  virtual const Level* getLevel() const = 0;
  virtual vector<const Creature*> getUnknownAttacker() const = 0;
  virtual vector<const Creature*> getVisibleCreatures() const { return {}; }
};

#endif
