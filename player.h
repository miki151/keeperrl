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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "creature.h"

class View;
class Model;

class Player : public Controller, public EventListener {
  public:
  Player(Creature*, View*, Model*, bool displayGreeting, map<Level*, MapMemory>* levelMemory);
  virtual ~Player();
  virtual void grantIdentify(int numItems) override;

  virtual bool isPlayer() const override;

  virtual void you(MsgType type, const string& param) const override;
  virtual void you(const string& param) const override;
  virtual void privateMessage(const string& message) const override;

  virtual void onKilled(const Creature* attacker) override;
  virtual void onItemsAppeared(vector<Item*> items, const Creature* from) override;

  virtual const MapMemory& getMemory() const override;
  virtual void learnLocation(const Location*) override;

  virtual void makeMove() override;
  virtual void sleeping() override;

  virtual void onBump(Creature*);

  static ControllerFactory getFactory(View*, Model*, map<Level*, MapMemory>* levelMemory);
  
  virtual const Level* getListenerLevel() const override;
  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) override;
  virtual void onExplosionEvent(const Level* level, Vec2 pos) override;
  virtual void onAlarmEvent(const Level*, Vec2 pos) override;

  SERIALIZATION_DECL(Player);

  private:
  void tryToPerform(Creature::Action);
  void pickUpAction(bool extended);
  void itemsMessage();
  void dropAction(bool extended);
  void equipmentAction();
  void applyAction();
  void applyItem(vector<Item*> item);
  void throwAction(Optional<Vec2> dir = Nothing());
  void throwItem(vector<Item*> item, Optional<Vec2> dir = Nothing());
  void takeOffAction();
  void hideAction();
  void displayInventory();
  bool interruptedByEnemy();
  void travelAction();
  void targetAction();
  void payDebtAction();
  void chatAction(Optional<Vec2> dir = Nothing());
  void spellAction();
  void fireAction(Vec2 dir);
  vector<Item*> chooseItem(const string& text, ItemPredicate, Optional<UserInput::Type> exitAction = Nothing());
  void getItemNames(vector<Item*> it, vector<View::ListElem>& names, vector<vector<Item*> >& groups,
      ItemPredicate = alwaysTrue<const Item*>());
  string getPluralName(Item* item, int num);
  Creature* SERIAL(creature);
  bool SERIAL2(travelling, false);
  Vec2 SERIAL(travelDir);
  Optional<Vec2> SERIAL(target);
  const Location* SERIAL2(lastLocation, nullptr);
  vector<const Creature*> SERIAL(specialCreatures);
  bool SERIAL(displayGreeting);
  map<Level*, MapMemory>* SERIAL(levelMemory);
  Model* SERIAL(model);
};

#endif
