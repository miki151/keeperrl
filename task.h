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

#ifndef _TASK_H
#define _TASK_H

#include "monster_ai.h"
#include "unique_entity.h"
#include "entity_set.h"
#include "square_type.h"

class Task : public UniqueEntity {
  public:

  class Callback {
    public:
    virtual void onConstructed(Vec2 pos, SquareType) {}
    virtual void onAppliedItem(Vec2 pos, Item* item) {}
    virtual void onAppliedSquare(Vec2 pos) {}
    virtual void onAppliedItemCancel(Vec2 pos) {}
    virtual void onPickedUp(Vec2 pos, EntitySet) {}
    virtual void onCantPickItem(EntitySet items) {}
    virtual void onKillCancelled(Creature*) {}

    SERIALIZATION_DECL(Callback);
  };

  Task();
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual bool isImpossible(const Level*);
  virtual bool canTransfer();
  virtual void cancel() {}
  bool isDone();

  static PTask construction(Callback*, Vec2 target, SquareType);
  static PTask bringItem(Callback*, Vec2 position, vector<Item*>, vector<Vec2> target);
  static PTask applyItem(Callback*, Vec2 position, Item* item, Vec2 target);
  static PTask applySquare(Callback*, vector<Vec2> squares);
  static PTask equipItem(Callback*, Vec2 position, Item* item);
  static PTask pickItem(Callback*, Vec2 position, vector<Item*> items);
  static PTask kill(Callback*, Creature*);
  static PTask torture(Callback*, Creature*);
  static PTask sacrifice(Callback*, Creature*);
  static PTask destroySquare(Vec2 position);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask explore(Vec2);
  static PTask attackCollective(Collective*);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  void setDone();

  private:
  SERIAL_CHECKER;
  bool SERIAL2(done, false);
};

#endif
