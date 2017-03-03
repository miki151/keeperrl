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

#pragma once

#include "move_info.h"
#include "unique_entity.h"
#include "entity_set.h"
#include "position.h"
#include "destroy_action.h"

class SquareType;
class Location;
class TaskCallback;
class CreatureFactory;

class Task : public UniqueEntity<Task> {
  public:

  Task(bool transferable = false);
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual bool isBogus() const;
  virtual bool isBlocked(Creature*) const;
  virtual bool canTransfer();
  virtual void cancel() {}
  virtual string getDescription() const = 0;
  virtual bool canPerform(const Creature* c);
  virtual optional<Position> getPosition() const;
  optional<ViewId> getViewId() const;
  bool isDone();
  void setViewId(ViewId);

  static PTask construction(TaskCallback*, Position, FurnitureType);
  static PTask destruction(TaskCallback*, Position, const Furniture*, DestroyAction);
  static PTask buildTorch(TaskCallback*, Position, Dir attachmentDir);
  static PTask bringItem(TaskCallback*, Position position, vector<Item*>, const set<Position>& target,
      int numRetries = 10);
  static PTask applyItem(TaskCallback*, Position, Item*, Position target);
  enum SearchType { LAZY, RANDOM_CLOSE };
  enum ActionType { APPLY, NONE };
  static PTask applySquare(TaskCallback*, vector<Position>, SearchType, ActionType);
  static PTask pickAndEquipItem(TaskCallback*, Position, Item*);
  static PTask equipItem(Item*);
  static PTask pickItem(TaskCallback*, Position, vector<Item*>);
  static PTask kill(TaskCallback*, Creature*);
  static PTask torture(TaskCallback*, Creature*);
  static PTask sacrifice(TaskCallback*, Creature*);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask chain(PTask, PTask, PTask);
  static PTask chain(vector<PTask>);
  static PTask explore(Position);
  static PTask attackLeader(Collective*);
  static PTask campAndSpawn(Collective* target, const CreatureFactory&, int defenseSize,
      Range attackSize, int numAttacks);
  static PTask killFighters(Collective*, int numFighters);
  static PTask stealFrom(Collective*, TaskCallback*);
  static PTask createBed(TaskCallback*, Position, const SquareType& fromType, const SquareType& toType);
  static PTask consumeItem(TaskCallback*, vector<Item*> items);
  static PTask copulate(TaskCallback*, Creature* target, int numTurns);
  static PTask consume(TaskCallback*, Creature* target);
  static PTask eat(set<Position> hatcherySquares);
  static PTask goTo(Position);
  static PTask goToTryForever(Position);
  static PTask transferTo(Model*);
  static PTask goToAndWait(Position, double waitTime);
  static PTask whipping(Position, Creature* whipped);
  static PTask dropItems(vector<Item*>);
  static PTask spider(Position origin, const vector<Position>& posClose, const vector<Position>& posFurther);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  protected:
  void setDone();

  private:
  bool SERIAL(done) = false;
  bool SERIAL(transfer);
  optional<ViewId> SERIAL(viewId);
};
