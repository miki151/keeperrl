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

class Task : public UniqueEntity {
  public:
  Task(Collective*, Vec2 position);
  virtual ~Task();

  virtual MoveInfo getMove(Creature*) = 0;
  virtual string getInfo() = 0;
  virtual bool isImpossible(const Level*) { return false; }
  virtual bool canTransfer() { return true; }
  virtual void cancel() {}
  bool isDone();

  Vec2 getPosition();

  static PTask construction(Collective*, Vec2 target, SquareType);
  static PTask bringItem(Collective*, Vec2 position, vector<Item*>, Vec2 target);
  static PTask applyItem(Collective* col, Vec2 position, Item* item, Vec2 target);
  static PTask applySquare(Collective*, set<Vec2> squares);
  static PTask eat(Collective*, set<Vec2> hatcherySquares);
  static PTask equipItem(Collective* col, Vec2 position, Item* item);
  static PTask unEquipItem(Collective* col, Vec2 position, Item* item);
  static PTask pickItem(Collective* col, Vec2 position, vector<Item*> items);

  SERIALIZATION_DECL(Task);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  void setDone();
  void setPosition(Vec2);
  MoveInfo getMoveToPosition(Creature*, bool stepOnTile = false);
  Collective* getCollective();

  private:
  Vec2 SERIAL(position);
  bool SERIAL2(done, false);
  Collective* SERIAL(collective);
};

#endif
