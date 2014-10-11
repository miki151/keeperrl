/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _TRIGGER_H
#define _TRIGGER_H

#include "util.h"
#include "effect_type.h"

class Creature;
class CreatureView;
class Attack;
class Vision;
class Tribe;
class ViewObject;

class Trigger {
  public:
  virtual Optional<ViewObject> getViewObject(const CreatureView*) const;
  virtual ~Trigger();

  virtual void onCreatureEnter(Creature* c);
  virtual bool interceptsFlyingItem(Item* it) const;
  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir, Vision*);

  virtual bool isDangerous(const Creature* c) const;
  virtual void tick(double time);
  virtual void setOnFire(double size);
  virtual double getLightEmission() const;

  static PTrigger getPortal(const ViewObject&, Level*, Vec2 position);
  static PTrigger getTrap(const ViewObject&, Level*, Vec2 position, EffectType, Tribe*);
  static PTrigger getTorch(Dir attachmentDir, Level*, Vec2 position);
  static PTrigger getMeteorShower(Creature*, double duration);

  static const ViewObject& getTorchViewObject(Dir);

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
