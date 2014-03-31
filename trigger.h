#ifndef _TRIGGER_H
#define _TRIGGER_H

#include "view_object.h"
#include "util.h"
#include "creature.h"

class Trigger {
  public:
  virtual Optional<ViewObject> getViewObject(const CreatureView*) const;
  virtual ~Trigger();

  virtual void onCreatureEnter(Creature* c);
  virtual bool interceptsFlyingItem(Item* it) const;
  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir);

  virtual bool isDangerous(const Creature* c) const;
  virtual void tick(double time);

  static PTrigger getPortal(const ViewObject& obj, Level*, Vec2 position);
  static PTrigger getTrap(const ViewObject& obj, Level* l, Vec2 position, EffectType effect, Tribe* tribe);

  template <class Archive>
  static void registerTypes(Archive& ar);

  SERIALIZATION_DECL(Trigger);

  protected:
  Trigger(Level*, Vec2 position);
  Trigger(const ViewObject& obj, Level* l, Vec2 p);

  Optional<ViewObject> SERIAL(viewObject);
  Level* SERIAL(level);
  Vec2 SERIAL(position);
};

#endif
