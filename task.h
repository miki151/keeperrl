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

class Task : public UniqueEntity<Task> {
  public:

  class Callback {
    public:
    virtual void onConstructed(Vec2 pos, SquareType) {}
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
    virtual void onBedCreated(Vec2 pos, SquareType fromType, SquareType toType) {}
    virtual void onCopulated(Creature* who, Creature* with) {}
    virtual void onConsumed(Creature* consumer, Creature* who) {}

    SERIALIZATION_DECL(Callback);
  };

  Task();
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual bool isImpossible(const Level*);
  virtual bool canTransfer();
  virtual void cancel() {}
  virtual string getDescription() const = 0;
  bool isDone();

  static PTask construction(Callback*, Vec2 target, SquareType);
  static PTask buildTorch(Callback*, Vec2 target, Dir attachmentDir);
  static PTask bringItem(Callback*, Vec2 position, vector<Item*>, vector<Vec2> target, int numRetries = 10);
  static PTask applyItem(Callback*, Vec2 position, Item* item, Vec2 target);
  static PTask applySquare(Callback*, vector<Vec2> squares);
  static PTask pickAndEquipItem(Callback*, Vec2 position, Item* item);
  static PTask equipItem(Item* item);
  static PTask pickItem(Callback*, Vec2 position, vector<Item*> items);
  static PTask kill(Callback*, Creature*);
  static PTask torture(Callback*, Creature*);
  static PTask sacrifice(Callback*, Creature*);
  static PTask destroySquare(Vec2 position);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask chain(vector<PTask>);
  static PTask explore(Vec2);
  static PTask attackLeader(Collective*);
  static PTask killFighters(Collective*, int numFighters);
  static PTask stealFrom(Collective*, Callback*);
  static PTask createBed(Callback*, Vec2, SquareType fromType, SquareType toType);
  static PTask consumeItem(Callback*, vector<Item*> items);
  static PTask copulate(Callback*, Creature* target, int numTurns);
  static PTask consume(Callback*, Creature* target);
  static PTask stayInLocationUntil(const Location*, double time);
  static PTask eat(set<Vec2> hatcherySquares);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  protected:
  void setDone();

  private:
  bool SERIAL(done) = false;
};

#endif
