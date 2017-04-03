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

#include "creature_action.h"
#include "controller.h"
#include "user_input.h"
#include "creature_view.h"
#include "map_memory.h"
#include "position.h"
#include "event_listener.h"
#include "game_info.h"

class View;
class Model;
class Creature;
class Item;
class ListElem;
struct ItemInfo;
class Game;
class VisibilityMap;
class Tutorial;

class Player : public Controller, public CreatureView, public EventListener<Player> {
  public:
  virtual ~Player();

  static ControllerFactory getFactory(MapMemory* levelMemory);
  Player(WCreature, bool adventurer, MapMemory*, STutorial = nullptr);

  void onEvent(const GameEvent&);

  SERIALIZATION_DECL(Player)

  protected:

  virtual void moveAction(Vec2 direction);

  // from CreatureView
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual const MapMemory& getMemory() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual WLevel getLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getLocalTime() const override;
  virtual bool isPlayerView() const override;
  virtual vector<Vec2> getUnknownLocations(WConstLevel) const override;

  // from Controller
  virtual void onKilled(WConstCreature attacker) override;
  virtual void makeMove() override;
  virtual void sleeping() override;
  virtual bool isPlayer() const override;
  virtual void you(MsgType type, const string& param) override;
  virtual void you(MsgType type, const vector<string>& param) override;
  virtual void you(const string& param) override;
  virtual void privateMessage(const PlayerMessage& message) override;
  virtual void onBump(WCreature) override;
  virtual void onDisplaced() override;

  // overridden by subclasses
  struct CommandInfo {
    PlayerInfo::CommandInfo commandInfo;
    function<void(Player*)> perform;
    bool actionKillsController;
  };
  virtual vector<CommandInfo> getCommands() const;
  virtual void onFellAsleep();
  virtual vector<WCreature> getTeam() const;

  MapMemory* SERIAL(levelMemory);
  void showHistory();
  WGame getGame() const;
  View* getView() const;

  bool tryToPerform(CreatureAction);

  private:

  void considerAdventurerMusic();
  void extendedAttackAction(UniqueEntity<Creature>::Id);
  void extendedAttackAction(WCreature other);
  void creatureAction(UniqueEntity<Creature>::Id);
  void pickUpItemAction(int item, bool multi = false);
  void equipmentAction();
  void applyItem(vector<WItem> item);
  void throwItem(vector<WItem> item, optional<Vec2> dir = none);
  void takeOffAction();
  void hideAction();
  void displayInventory();
  void handleItems(const EntitySet<Item>&, ItemAction);
  vector<ItemAction> getItemActions(const vector<WItem>&) const;
  bool interruptedByEnemy();
  void travelAction();
  void targetAction();
  void payForAllItemsAction();
  void payForItemAction(const vector<WItem>&);
  void chatAction(optional<Vec2> dir = none);
  void giveAction(vector<WItem>);
  void spellAction(SpellId);
  void fireAction();
  void fireAction(Vec2 dir);
  vector<WItem> chooseItem(const string& text, ItemPredicate, optional<UserInputId> exitAction = none);
  void getItemNames(vector<WItem> it, vector<ListElem>& names, vector<vector<WItem> >& groups,
      ItemPredicate = alwaysTrue<const WItem>());
  string getInventoryItemName(const WItem, bool plural) const;
  string getPluralName(WItem item, int num);
  bool SERIAL(travelling) = false;
  Vec2 SERIAL(travelDir);
  optional<Position> SERIAL(target);
  bool SERIAL(adventurer);
  bool SERIAL(displayGreeting);
  bool updateView = true;
  void retireMessages();
  vector<PlayerMessage> SERIAL(messages);
  vector<PlayerMessage> SERIAL(messageHistory);
  string getRemainingString(LastingEffect) const;
  vector<ItemInfo> getItemInfos(const vector<WItem>&) const;
  ItemInfo getItemInfo(const vector<WItem>&) const;
  ItemInfo getFurnitureUsageInfo(const string& question, ViewId viewId) const;
  optional<FurnitureUsageType> getUsableUsageType() const;
  struct TimePosInfo {
    Position pos;
    double time;
  };
  optional<TimePosInfo> currentTimePos;
  optional<TimePosInfo> previousTimePos;
  HeapAllocated<VisibilityMap> SERIAL(visibilityMap);
  STutorial SERIAL(tutorial);
};

