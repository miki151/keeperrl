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
#include "user_input.h"
#include "creature_view.h"

class View;
class Model;
class Creature;
class Item;
class ListElem;
struct ItemInfo;

class Player : public Controller, public CreatureView {
  public:
  virtual ~Player();

  static ControllerFactory getFactory(Model*, map<UniqueEntity<Level>::Id, MapMemory>* levelMemory);

  SERIALIZATION_DECL(Player);

  protected:
  Player(Creature*, Model*, bool greeting, map<UniqueEntity<Level>::Id, MapMemory>* levelMemory);

  virtual void moveAction(Vec2 direction);

  // from CreatureView
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual const MapMemory& getMemory() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual optional<Vec2> getPosition(bool force) const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual const Level* getLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getTime() const override;

  // from Controller
  virtual void onKilled(const Creature* attacker) override;
  virtual void makeMove() override;
  virtual void sleeping() override;
  virtual bool isPlayer() const override;
  virtual void you(MsgType type, const string& param) override;
  virtual void you(MsgType type, const vector<string>& param) override;
  virtual void you(const string& param) override;
  virtual void privateMessage(const PlayerMessage& message) override;
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

  void tryToPerform(CreatureAction);
  void attackAction(Creature* other);
  void attackAction(UniqueEntity<Creature>::Id);
  void pickUpItemAction(int item, bool multi = false);
  void equipmentAction();
  void applyItem(vector<Item*> item);
  void throwItem(vector<Item*> item, optional<Vec2> dir = none);
  void takeOffAction();
  void hideAction();
  void displayInventory();
  void handleItems(const vector<UniqueEntity<Item>::Id>&, ItemAction);
  vector<ItemAction> getItemActions(const vector<Item*>&) const;
  bool interruptedByEnemy();
  void travelAction();
  void targetAction();
  void payDebtAction();
  void chatAction(optional<Vec2> dir = none);
  void spellAction(SpellId);
  void consumeAction();
  void fireAction(Vec2 dir);
  vector<Item*> chooseItem(const string& text, ItemPredicate, optional<UserInputId> exitAction = none);
  void getItemNames(vector<Item*> it, vector<ListElem>& names, vector<vector<Item*> >& groups,
      ItemPredicate = alwaysTrue<const Item*>());
  string getInventoryItemName(const Item*, bool plural) const;
  string getPluralName(Item* item, int num);
  bool SERIAL(travelling) = false;
  Vec2 SERIAL(travelDir);
  optional<Vec2> SERIAL(target);
  const Location* SERIAL(lastLocation) = nullptr;
  vector<const Creature*> SERIAL(specialCreatures);
  bool SERIAL(displayGreeting);
  vector<EpithetId> SERIAL(usedEpithets);
  bool updateView = true;
  void retireMessages();
  vector<PlayerMessage> SERIAL(messages);
  vector<string> SERIAL(messageHistory);
  string getRemainingString(LastingEffect) const;
  vector<ItemInfo> getItemInfos(const vector<Item*>&) const;
  ItemInfo getItemInfo(const vector<Item*>&) const;
  ItemInfo getApplySquareInfo(const Square*) const;
  ItemInfo getApplySquareInfo(const string& question, ViewId viewId) const;
  optional<SquareApplyType> getUsableSquareApplyType() const;
  struct TimePosInfo {
    Vec2 pos;
    double time;
  };
  TimePosInfo currentTimePos = {Vec2(-1, -1), 0.0};
  TimePosInfo previousTimePos = {Vec2(-1, -1), 0.0};
};

#endif
