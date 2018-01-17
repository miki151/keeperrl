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
class MessageBuffer;

class Player : public Controller, public CreatureView, public EventListener<Player> {
  public:
  virtual ~Player() override;

  Player(WCreature, bool adventurer, SMapMemory, SMessageBuffer, SVisibilityMap, STutorial = nullptr);

  void onEvent(const GameEvent&);

  SERIALIZATION_DECL(Player)

  protected:

  virtual void moveAction(Vec2 direction);

  // from CreatureView
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual const MapMemory& getMemory() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual WLevel getLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getAnimationTime() const override;
  virtual CenterType getCenterType() const override;
  virtual vector<Vec2> getUnknownLocations(WConstLevel) const override;

  // from Controller
  virtual void onKilled(WConstCreature attacker) override;
  virtual void makeMove() override;
  virtual void sleeping() override;
  virtual bool isPlayer() const override;
  virtual void privateMessage(const PlayerMessage& message) override;
  virtual MessageGenerator& getMessageGenerator() const override;
  virtual void onStartedControl() override;
  virtual void onEndedControl() override;

  // overridden by subclasses
  struct CommandInfo {
    PlayerInfo::CommandInfo commandInfo;
    function<void(Player*)> perform;
    bool actionKillsController;
  };
  virtual vector<CommandInfo> getCommands() const;
  virtual void onFellAsleep();
  virtual vector<WCreature> getTeam() const;
  virtual bool isTravelEnabled() const;
  virtual bool handleUserInput(UserInput);
  struct OtherCreatureCommand {
    string name;
    bool allowAuto;
    function<void(Player*)> perform;
  };
  virtual vector<OtherCreatureCommand> getOtherCreatureCommands(WCreature) const;

  optional<Vec2> chooseDirection(const string& question);

  SMapMemory SERIAL(levelMemory);
  void showHistory();
  WGame getGame() const;
  WModel getModel() const;
  View* getView() const;

  bool tryToPerform(CreatureAction);

  private:

  void considerAdventurerMusic();
  void creatureClickAction(Position, bool extended);
  void pickUpItemAction(int item, bool multi = false);
  void equipmentAction();
  void applyItem(vector<WItem> item);
  void throwItem(vector<WItem> item, optional<Vec2> dir = none);
  void takeOffAction();
  void hideAction();
  void displayInventory();
  void handleItems(const EntitySet<Item>&, ItemAction);
  void handleIntrinsicAttacks(const EntitySet<Item>&, ItemAction);
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
      ItemPredicate = alwaysTrue<WConstItem>());
  string getInventoryItemName(WConstItem, bool plural) const;
  string getPluralName(WItem item, int num);
  bool SERIAL(travelling) = false;
  Vec2 SERIAL(travelDir);
  optional<Position> SERIAL(target);
  bool SERIAL(adventurer);
  bool SERIAL(displayGreeting);
  bool updateView = true;
  void retireMessages();
  SMessageBuffer SERIAL(messageBuffer);
  string getRemainingString(LastingEffect) const;
  ItemInfo getFurnitureUsageInfo(const string& question, ViewId viewId) const;
  optional<FurnitureUsageType> getUsableUsageType() const;
  SVisibilityMap SERIAL(visibilityMap);
  STutorial SERIAL(tutorial);
  vector<TeamMemberAction> getTeamMemberActions(WConstCreature) const;
  optional<GlobalTime> lastEnemyInterruption;
};

