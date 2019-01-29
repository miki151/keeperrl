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
class CreatureGroup;

using WTaskCallback = WeakPointer<TaskCallback>;

class TaskPredicate : public OwnedObject<TaskPredicate> {
  public:
  virtual bool apply() const = 0;
  virtual ~TaskPredicate() {}

  static PTaskPredicate outsidePositions(WCreature, PositionSet);
  static PTaskPredicate always();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

class Task : public UniqueEntity<Task>, public OwnedObject<Task> {
  public:

  Task(bool transferable = false);
  virtual ~Task();

  virtual MoveInfo getMove(WCreature) = 0;
  virtual bool isBogus() const;
  virtual bool canTransfer();
  virtual void cancel() {}
  virtual string getDescription() const = 0;
  virtual bool canPerform(WConstCreature c) const;
  virtual optional<Position> getPosition() const;
  virtual optional<StorageId> getStorageId(bool dropOnly) const;
  optional<ViewId> getViewId() const;
  bool isDone();
  void setViewId(ViewId);

  static PTask construction(WTaskCallback, Position, FurnitureType);
  static PTask destruction(WTaskCallback, Position, WConstFurniture, DestroyAction, WPositionMatching);
  static PTask bringItem(Position position, vector<WItem>, const PositionSet& target);
  static PTask applyItem(WTaskCallback, Position target, WItem);
  enum SearchType { LAZY, RANDOM_CLOSE };
  enum ActionType { APPLY, NONE };
  static PTask applySquare(WTaskCallback, vector<Position>, SearchType, ActionType);
  static PTask archeryRange(WTaskCallback, vector<Position>);
  static PTask pickAndEquipItem(Position, WItem item);
  static PTask equipItem(WItem);
  static PTask pickUpItem(Position, vector<WItem>, optional<StorageId> = none);
  static PTask kill(WTaskCallback, WCreature);
  static PTask torture(WTaskCallback, WCreature);
  static PTask sacrifice(WTaskCallback, WCreature);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask chain(PTask, PTask, PTask);
  static PTask chain(vector<PTask>);
  static PTask explore(Position);
  static PTask attackCreatures(vector<WCreature>);
  static PTask campAndSpawn(WCollective target, const CreatureGroup&, int defenseSize,
      Range attackSize, int numAttacks);
  static PTask killFighters(WCollective, int numFighters);
  static PTask stealFrom(WCollective);
  static PTask consumeItem(WTaskCallback, vector<WItem> items);
  static PTask copulate(WTaskCallback, WCreature target, int numTurns);
  static PTask consume(WCreature target);
  static PTask eat(vector<Position> hatcherySquares);
  static PTask goTo(Position);
  static PTask stayIn(vector<Position>);
  static PTask idle();
  static PTask alwaysDone(PTask);
  static PTask doneWhen(PTask, PTaskPredicate);
  static PTask follow(WCreature);
  static PTask goToTryForever(Position);
  static PTask transferTo(WModel);
  static PTask goToAndWait(Position, TimeInterval waitTime);
  static PTask whipping(Position, WCreature whipped);
  static PTask dropItemsAnywhere(vector<WItem>);
  static PTask dropItems(vector<WItem>, vector<Position>);
  static PTask dropItems(vector<WItem>, StorageId, WCollective);
  struct PickUpAndDrop {
    PTask pickUp;
    PTask drop;
  };
  static PickUpAndDrop pickUpAndDrop(Position origin, vector<WItem>, StorageId, WCollective);
  static PTask spider(Position origin, const vector<Position>& posClose);
  static PTask withTeam(WCollective, TeamId, PTask);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  protected:
  void setDone();

  private:
  bool SERIAL(done) = false;
  bool SERIAL(transfer);
  optional<ViewId> SERIAL(viewId);
};
