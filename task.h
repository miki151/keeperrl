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
#include "game_time.h"

class TaskCallback;
class CreatureFactory;

using WTaskCallback = WeakPointer<TaskCallback>;

class Task : public UniqueEntity<Task>, public OwnedObject<Task> {
  public:

  Task(bool transferable = false);
  virtual ~Task();

  virtual MoveInfo getMove(WCreature) = 0;
  virtual bool isBogus() const;
  virtual bool isBlocked(WConstCreature) const;
  virtual bool canTransfer();
  virtual void cancel() {}
  virtual string getDescription() const = 0;
  virtual bool canPerform(WConstCreature c);
  virtual optional<Position> getPosition() const;
  optional<ViewId> getViewId() const;
  bool isDone();
  void setViewId(ViewId);

  static PTask construction(WTaskCallback, Position, FurnitureType);
  static PTask destruction(WTaskCallback, Position, WConstFurniture, DestroyAction);
  static PTask bringItem(WTaskCallback, Position position, vector<WItem>, const set<Position>& target,
      int numRetries = 10);
  static PTask applyItem(WTaskCallback, Position, WItem, Position target);
  enum SearchType { LAZY, RANDOM_CLOSE };
  enum ActionType { APPLY, NONE };
  static PTask applySquare(WTaskCallback, vector<Position>, SearchType, ActionType);
  static PTask archeryRange(WTaskCallback, vector<Position>);
  static PTask pickAndEquipItem(WTaskCallback, Position, WItem);
  static PTask equipItem(WItem);
  static PTask pickItem(WTaskCallback, Position, vector<WItem>);
  static PTask kill(WTaskCallback, WCreature);
  static PTask torture(WTaskCallback, WCreature);
  static PTask sacrifice(WTaskCallback, WCreature);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask chain(PTask, PTask, PTask);
  static PTask chain(vector<PTask>);
  static PTask explore(Position);
  static PTask attackCreatures(vector<WCreature>);
  static PTask campAndSpawn(WCollective target, const CreatureFactory&, int defenseSize,
      Range attackSize, int numAttacks);
  static PTask killFighters(WCollective, int numFighters);
  static PTask stealFrom(WCollective, WTaskCallback);
  static PTask consumeItem(WTaskCallback, vector<WItem> items);
  static PTask copulate(WTaskCallback, WCreature target, int numTurns);
  static PTask consume(WTaskCallback, WCreature target);
  static PTask eat(vector<Position> hatcherySquares);
  static PTask goTo(Position);
  static PTask stayIn(vector<Position>);
  static PTask idle();
  static PTask follow(WCreature);
  static PTask goToTryForever(Position);
  static PTask transferTo(WModel);
  static PTask goToAndWait(Position, TimeInterval waitTime);
  static PTask whipping(Position, WCreature whipped);
  static PTask dropItems(vector<WItem>);
  static PTask spider(Position origin, const vector<Position>& posClose);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  protected:
  void setDone();

  private:
  bool SERIAL(done) = false;
  bool SERIAL(transfer);
  optional<ViewId> SERIAL(viewId);
};
