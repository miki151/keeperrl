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
template <typename T>
class EventProxy;
class VisibilityMap;

class Player : public Controller, public CreatureView {
  public:
  virtual ~Player();

  static ControllerFactory getFactory(Model*, MapMemory* levelMemory);

  SERIALIZATION_DECL(Player);

  protected:
  Player(Creature*, Model*, bool adventurer, MapMemory*);

  virtual void moveAction(Vec2 direction);

  // from CreatureView
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual const MapMemory& getMemory() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual Level* getLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getLocalTime() const override;
  virtual bool isPlayerView() const override;

  // from Controller
  virtual void onKilled(const Creature* attacker) override;
  virtual void makeMove() override;
  virtual void sleeping() override;
  virtual bool isPlayer() const override;
  virtual void you(MsgType type, const string& param) override;
  virtual void you(MsgType type, const vector<string>& param) override;
  virtual void you(const string& param) override;
  virtual void privateMessage(const PlayerMessage& message) override;
  virtual void onBump(Creature*) override;
  virtual void onDisplaced() override;

  // overridden by subclasses
  struct CommandInfo {
    PlayerInfo::CommandInfo commandInfo;
    function<void(Player*)> perform;
    bool actionKillsController;
  };
  virtual vector<CommandInfo> getCommands() const;
  virtual void onFellAsleep();
  virtual vector<Creature*> getTeam() const;

  MapMemory* SERIAL(levelMemory);
  void showHistory();
  Game* getGame() const;
  View* getView() const;

  bool tryToPerform(CreatureAction);

  private:
  HeapAllocated<EventProxy<Player>> SERIAL(eventProxy);
  friend EventProxy<Player>;
  void onEvent(const GameEvent&);

  void considerAdventurerMusic();
  void extendedAttackAction(UniqueEntity<Creature>::Id);
  void extendedAttackAction(Creature* other);
  void creatureAction(UniqueEntity<Creature>::Id);
  void pickUpItemAction(int item, bool multi = false);
  void equipmentAction();
  void applyItem(vector<Item*> item);
  void throwItem(vector<Item*> item, optional<Vec2> dir = none);
  void takeOffAction();
  void hideAction();
  void displayInventory();
  void handleItems(const EntitySet<Item>&, ItemAction);
  vector<ItemAction> getItemActions(const vector<Item*>&) const;
  bool interruptedByEnemy();
  void travelAction();
  void targetAction();
  void payDebtAction();
  void chatAction(optional<Vec2> dir = none);
  void giveAction(vector<Item*>);
  void spellAction(SpellId);
  void fireAction(Vec2 dir);
  vector<Item*> chooseItem(const string& text, ItemPredicate, optional<UserInputId> exitAction = none);
  void getItemNames(vector<Item*> it, vector<ListElem>& names, vector<vector<Item*> >& groups,
      ItemPredicate = alwaysTrue<const Item*>());
  string getInventoryItemName(const Item*, bool plural) const;
  string getPluralName(Item* item, int num);
  bool SERIAL(travelling) = false;
  Vec2 SERIAL(travelDir);
  optional<Position> SERIAL(target);
  const Location* SERIAL(lastLocation) = nullptr;
  bool SERIAL(adventurer);
  bool SERIAL(displayGreeting);
  bool updateView = true;
  void retireMessages();
  vector<PlayerMessage> SERIAL(messages);
  vector<PlayerMessage> SERIAL(messageHistory);
  string getRemainingString(LastingEffect) const;
  vector<ItemInfo> getItemInfos(const vector<Item*>&) const;
  ItemInfo getItemInfo(const vector<Item*>&) const;
  ItemInfo getFurnitureUsageInfo(const string& question, ViewId viewId) const;
  optional<FurnitureUsageType> getUsableUsageType() const;
  struct TimePosInfo {
    Position pos;
    double time;
  };
  optional<TimePosInfo> currentTimePos;
  optional<TimePosInfo> previousTimePos;
  HeapAllocated<VisibilityMap> SERIAL(visibilityMap);
};

