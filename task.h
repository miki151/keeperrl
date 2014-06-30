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
    virtual void onBrought(Vec2 pos, vector<Item*> items) {}
    virtual void onAppliedItem(Vec2 pos, Item* item) {}
    virtual void onAppliedSquare(Vec2 pos) {}
    virtual void onAppliedItemCancel(Vec2 pos) {}
    virtual void onPickedUp(Vec2 pos, EntitySet) {}
    virtual void onCantPickItem(EntitySet items) {}

    SERIALIZATION_DECL(Callback);
  };

  class Mapping {
    public:
    Task* addTask(PTask, const Creature* = nullptr);
    Task* getTask(const Creature*) const;
    void removeTask(Task*);
    void removeTask(UniqueId);
    void takeTask(const Creature*, Task*);
    void freeTask(Task*);

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);

    SERIAL_CHECKER;

    protected:
    map<Task*, const Creature*> SERIAL(taken);
    map<const Creature*, Task*> SERIAL(taskMap);
    vector<PTask> SERIAL(tasks);
  };

  Task(Callback*, Vec2 position);
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual string getInfo() = 0;
  virtual bool isImpossible(const Level*) { return false; }
  virtual bool canTransfer() { return true; }
  virtual void cancel() {}
  bool isDone();

  Vec2 getPosition();

  static PTask construction(Callback*, Vec2 target, SquareType);
  static PTask bringItem(Callback*, Vec2 position, vector<Item*>, Vec2 target);
  static PTask applyItem(Callback* col, Vec2 position, Item* item, Vec2 target);
  static PTask applySquare(Callback*, set<Vec2> squares);
  static PTask eat(Callback*, set<Vec2> hatcherySquares);
  static PTask equipItem(Callback* col, Vec2 position, Item* item);
  static PTask unEquipItem(Callback* col, Vec2 position, Item* item);
  static PTask pickItem(Callback* col, Vec2 position, vector<Item*> items);

  SERIALIZATION_DECL(Task);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  void setDone();
  void setPosition(Vec2);
  MoveInfo getMoveToPosition(Creature*, bool stepOnTile = false);
  Callback* getCallback();

  private:
  Vec2 SERIAL(position);
  bool SERIAL2(done, false);
  Callback* SERIAL(callback);
};

#endif
