#ifndef _TRIGGER_H
#define _TRIGGER_H

#include "view_object.h"
#include "util.h"
#include "creature.h"

class Trigger {
  public:

  Optional<ViewObject> getViewObject() const;

  virtual void onCreatureEnter(Creature* c) {}
  virtual bool interceptsFlyingItem(Item* it) const { return false; }
  virtual void onInterceptFlyingItem(PItem it, const Attack& a, int remainingDist, Vec2 dir) {}

  virtual void tick(double time) {}

  static PTrigger getPortal(const ViewObject& obj, Level*, Vec2 position);
  static PTrigger getTrap(const ViewObject& obj, Level* l, Vec2 position, PEffect _effect, Tribe* _tribe);

  protected:
  Trigger(Level*, Vec2 position);
  Trigger(const ViewObject& obj, Level* l, Vec2 p);

  Optional<ViewObject> viewObject;
  Level* level;
  Vec2 position;
};

#endif
