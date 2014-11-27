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

#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "enums.h"
#include "util.h"

class Creature;
class Location;
class MapMemory;
class Item;
class Level;
class PlayerMessage;

class Controller {
  public:
  Controller(Creature*);
  virtual void grantIdentify(int numItems) {};

  virtual bool isPlayer() const = 0;

  virtual void you(MsgType type, const string& param) = 0;
  virtual void you(const string& param) = 0;
  virtual void privateMessage(const PlayerMessage& message) {}

  virtual void onKilled(const Creature* attacker) {}
  virtual void onItemsAppeared(vector<Item*> items, const Creature* from) { }

  virtual void learnLocation(const Location*) { }

  virtual void makeMove() = 0;
  virtual void sleeping() {}

  virtual int getDebt(const Creature* debtor) const { return 0; }

  virtual void onBump(Creature*) = 0;

  virtual Controller* getPossessedController(Creature*);

  virtual ~Controller() {}

  SERIALIZATION_DECL(Controller);

  protected:
  Creature* SERIAL(creature);
};

class DoNothingController : public Controller {
  public:
  DoNothingController(Creature*);

  virtual bool isPlayer() const override;
  virtual void you(MsgType type, const string& param) override;
  virtual void you(const string& param) override;
  virtual void makeMove() override;
  virtual void onBump(Creature*) override;

  protected:
  SERIALIZATION_DECL(DoNothingController);
};

class ControllerFactory {
  public:
  ControllerFactory(function<Controller* (Creature*)>);
  PController get(Creature*);

  private:
  function<Controller* (Creature*)> fun;
};

#endif
