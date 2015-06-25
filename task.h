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

#include "move_info.h"
#include "unique_entity.h"
#include "entity_set.h"

class SquareType;
class Location;
class TaskCallback;

class Task : public UniqueEntity<Task> {
  public:

  Task();
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual bool isImpossible(const Level*);
  virtual bool canTransfer();
  virtual void cancel() {}
  virtual string getDescription() const = 0;
  bool isDone();

  static PTask construction(TaskCallback*, Vec2 target, const SquareType&);
  static PTask buildTorch(TaskCallback*, Vec2 target, Dir attachmentDir);
  static PTask bringItem(TaskCallback*, Vec2 position, vector<Item*>, vector<Vec2> target, int numRetries = 10);
  static PTask applyItem(TaskCallback*, Vec2 position, Item* item, Vec2 target);
  static PTask applySquare(TaskCallback*, vector<Vec2> squares);
  static PTask pickAndEquipItem(TaskCallback*, Vec2 position, Item* item);
  static PTask equipItem(Item* item);
  static PTask pickItem(TaskCallback*, Vec2 position, vector<Item*> items);
  static PTask kill(TaskCallback*, Creature*);
  static PTask torture(TaskCallback*, Creature*);
  static PTask sacrifice(TaskCallback*, Creature*);
  static PTask destroySquare(Vec2 position);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask chain(vector<PTask>);
  static PTask explore(Vec2);
  static PTask attackLeader(Collective*);
  static PTask killFighters(Collective*, int numFighters);
  static PTask stealFrom(Collective*, TaskCallback*);
  static PTask createBed(TaskCallback*, Vec2, const SquareType& fromType, const SquareType& toType);
  static PTask consumeItem(TaskCallback*, vector<Item*> items);
  static PTask copulate(TaskCallback*, Creature* target, int numTurns);
  static PTask consume(TaskCallback*, Creature* target);
  static PTask stayInLocationUntil(const Location*, double time);
  static PTask eat(set<Vec2> hatcherySquares);
  static PTask goTo(Vec2);
  static PTask dropItems(vector<Item*>);

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
