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
  Controller(const Controller&) = delete;
  Controller(Controller&&) = delete;

  virtual bool isPlayer() const = 0;

  virtual void you(MsgType type, const string& param) = 0;
  virtual void you(MsgType type, const vector<string>& param) = 0;
  virtual void you(const string& param) = 0;
  virtual void privateMessage(const PlayerMessage& message) {}

  virtual void onKilled(const Creature* attacker) {}
  virtual void onItemsGiven(vector<Item*> items, Creature* from) { }
  virtual void onDisplaced() {}

  virtual void makeMove() = 0;
  virtual void sleeping() {}
  virtual bool isCustomController() { return false; }

  virtual void onBump(Creature*) = 0;

  virtual ~Controller() {}

  SERIALIZATION_DECL(Controller);

  protected:
  Creature* getCreature() const;

  private:
  Creature* SERIAL(creature);
};

class DoNothingController : public Controller {
  public:
  DoNothingController(Creature*);

  virtual bool isPlayer() const override;
  virtual void you(MsgType type, const string& param) override;
  virtual void you(MsgType type, const vector<string>& param) override;
  virtual void you(const string& param) override;
  virtual void makeMove() override;
  virtual void onBump(Creature*) override;

  protected:
  SERIALIZATION_DECL(DoNothingController);
};

class ControllerFactory {
  public:
  ControllerFactory(function<SController(Creature*)>);
  SController get(Creature*) const;

  private:
  function<SController(Creature*)> fun;
};

