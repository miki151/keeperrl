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
#include "view_id.h"
#include "storage_id.h"

class TaskCallback;
class CreatureList;

using WTaskCallback = WeakPointer<TaskCallback>;

class TaskPredicate : public OwnedObject<TaskPredicate> {
  public:
  virtual bool apply() const = 0;
  virtual ~TaskPredicate();

  static PTaskPredicate outsidePositions(Creature*, PositionSet);
  static PTaskPredicate always();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

enum class TaskPerformResult { NEVER, YES, NOT_NOW };

class Task : public UniqueEntity<Task>, public OwnedObject<Task> {
  public:

  Task(bool transferable = false);
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual bool isBogus() const;
  virtual bool canTransfer();
  virtual void cancel() {}
  virtual string getDescription() const = 0;
  bool canPerform(const Creature* c, const MovementType&) const;
  bool canPerform(const Creature* c) const;
  virtual bool canPerformByAnyone() const;
  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const;
  virtual optional<Position> getPosition() const;
  virtual optional<StorageId> getStorageId(bool dropOnly) const;
  optional<ViewId> getViewId() const;
  bool isDone() const;
  void setViewId(ViewId);

  static PTask construction(WTaskCallback, Position, FurnitureType);
  static PTask destruction(WTaskCallback, Position, const Furniture*, DestroyAction, WPositionMatching);
  static PTask bringItem(Position position, vector<Item*>, const PositionSet& target);
  static PTask applyItem(WTaskCallback, Position target, Item*);
  enum SearchType { LAZY, RANDOM_CLOSE };
  enum ActionType { APPLY, NONE };
  static PTask applySquare(WTaskCallback, vector<pair<Position, FurnitureLayer>>, SearchType, ActionType);
  static PTask archeryRange(WTaskCallback, vector<Position>);
  static PTask pickAndEquipItem(Position, Item* item);
  static PTask equipItem(Item*);
  static PTask pickUpItem(Position, vector<Item*>, optional<StorageId> = none);
  static PTask kill(Creature*);
  static PTask torture(Creature*);
  static PTask disassemble(Creature*);
  static PTask sacrifice(WTaskCallback, Creature*);
  static PTask disappear();
  static PTask chain(PTask, PTask);
  static PTask chain(PTask, PTask, PTask);
  static PTask chain(vector<PTask>);
  static PTask explore(Position);
  static PTask abuseMinion(Collective*, Creature*);
  static PTask attackCreatures(vector<Creature*>);
  static PTask campAndSpawn(Collective* target, const CreatureList&, int numAttacks);
  static PTask killFighters(Collective*, int numFighters);
  static PTask stealFrom(Collective*);
  static PTask consumeItem(WTaskCallback, vector<Item*> items);
  static PTask copulate(WTaskCallback, Creature* target, int numTurns);
  static PTask consume(Creature* target);
  static PTask eat(vector<Position> hatcherySquares);
  static PTask goTo(Position);
  static PTask stayIn(vector<Position>);
  static PTask idle();
  static PTask alwaysDone(PTask);
  static PTask doneWhen(PTask, PTaskPredicate);
  static PTask follow(Creature*);
  static PTask goToTryForever(Position);
  static PTask transferTo(WModel);
  static PTask goToAndWait(Position, TimeInterval waitTime);
  static PTask whipping(Position, Creature* whipped);
  static PTask dropItemsAnywhere(vector<Item*>);
  static PTask dropItems(vector<Item*>, vector<Position>);
  static PTask dropItems(vector<Item*>, StorageId, Collective*);
  struct PickUpAndDrop {
    PTask pickUp;
    PTask drop;
  };
  static PickUpAndDrop pickUpAndDrop(Position origin, vector<Item*>, StorageId, Collective*);
  static PTask spider(Position origin, const vector<Position>& posClose);
  static PTask withTeam(Collective*, TeamId, PTask);
  static PTask installBodyPart(WTaskCallback, Creature* target, Item*);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  protected:
  void setDone();

  private:
  bool SERIAL(done) = false;
  bool SERIAL(transfer);
  optional<ViewId> SERIAL(viewId);
};
