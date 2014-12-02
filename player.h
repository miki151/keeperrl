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

#include "creature_action.h"
#include "event.h"
#include "controller.h"
#include "item.h"
#include "user_input.h"
#include "view.h"
#include "creature_view.h"

class View;
class Model;
class Creature;

class Player : public Controller, public CreatureView {
  public:
  Player(Creature*, Model*, bool adventureMode, map<UniqueEntity<Level>::Id, MapMemory>* levelMemory);
  virtual ~Player();
  virtual Controller* getPossessedController(Creature* c) override;

  static ControllerFactory getFactory(Model*, map<UniqueEntity<Level>::Id, MapMemory>* levelMemory);

  SERIALIZATION_DECL(Player);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  virtual void moveAction(Vec2 direction);

  // from CreatureView
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual const MapMemory& getMemory() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Optional<Vec2> getPosition(bool force) const override;
  virtual const Level* getLevel() const override;
  virtual vector<const Creature*> getVisibleEnemies() const override;

  // from Controller
  virtual void onKilled(const Creature* attacker) override;
  virtual void makeMove() override;
  virtual void sleeping() override;
  virtual void grantIdentify(int numItems) override;
  virtual bool isPlayer() const override;
  virtual void you(MsgType type, const string& param) override;
  virtual void you(const string& param) override;
  virtual void privateMessage(const PlayerMessage& message) override;
  virtual void onItemsAppeared(vector<Item*> items, const Creature* from) override;
  virtual void learnLocation(const Location*) override;
  virtual void onBump(Creature*) override;


  // overridden by subclasses
  virtual bool unpossess();
  virtual void onFellAsleep();
  virtual const vector<Creature*> getTeam() const;

  map<UniqueEntity<Level>::Id, MapMemory>* SERIAL(levelMemory);
  Model* SERIAL(model);
  void showHistory();

  private:
  REGISTER_HANDLER(ThrowEvent, const Level*, const Creature*, const Item*, const vector<Vec2>& trajectory);
  REGISTER_HANDLER(ExplosionEvent, const Level*, Vec2);
  REGISTER_HANDLER(AlarmEvent, const Level*, Vec2);
  REGISTER_HANDLER(WorshipEvent, Creature* who, const Deity* to, WorshipType);

  void tryToPerform(CreatureAction);
  void attackAction(Creature* other);
  void pickUpAction(bool extended);
  void pickUpItemAction(int item);
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
  void consumeAction();
  void fireAction(Vec2 dir);
  vector<Item*> chooseItem(const string& text, ItemPredicate, Optional<UserInputId> exitAction = Nothing());
  void getItemNames(vector<Item*> it, vector<View::ListElem>& names, vector<vector<Item*> >& groups,
      ItemPredicate = alwaysTrue<const Item*>());
  string getPluralName(Item* item, int num);
  bool SERIAL2(travelling, false);
  Vec2 SERIAL(travelDir);
  Optional<Vec2> SERIAL(target);
  const Location* SERIAL2(lastLocation, nullptr);
  vector<const Creature*> SERIAL(specialCreatures);
  bool SERIAL(displayGreeting);
  bool SERIAL(adventureMode);
  vector<EpithetId> SERIAL(usedEpithets);
  bool updateView = true;
  void retireMessages();
  vector<PlayerMessage> SERIAL(messages);
  vector<string> SERIAL(messageHistory);
  string getRemainingString(LastingEffect) const;
};

#endif
