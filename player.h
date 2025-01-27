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
#include "spell_id.h"

class View;
class Model;
class Creature;
class Item;
struct ItemInfo;
class Game;
class VisibilityMap;
class Tutorial;
class MessageBuffer;
class UnknownLocations;
class DungeonLevel;
class FurnitureUsageType;
class Spell;

class Player : public Controller, public CreatureView, public EventListener<Player> {
  public:
  virtual ~Player() override;

  Player(Creature*, SMapMemory, SMessageBuffer, SVisibilityMap, SUnknownLocations,
      STutorial = nullptr);

  void onEvent(const GameEvent&);
  virtual void forceSteeds() const;
  virtual vector<Creature*> getTeam() const;

  SERIALIZATION_DECL(Player)

  protected:

  virtual void moveAction(Vec2 direction);

  // from CreatureView
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual const MapMemory& getMemory() const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getScrollCoord() const override;
  virtual bool showScrollCoordOnMinimap() const override;
  virtual Level* getCreatureViewLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getAnimationTime() const override;
  virtual CenterType getCenterType() const override;
  virtual const vector<Vec2>& getUnknownLocations(const Level*) const override;
  virtual vector<Vec2> getHighlightedPathTo(Vec2) const override;
  virtual vector<vector<Vec2>> getPathTo(UniqueEntity<Creature>::Id, Vec2) const override;
  virtual vector<vector<Vec2>> getPermanentPaths() const override;
  virtual optional<Vec2> getPlayerPosition() const override;

  // from Controller
  virtual void onKilled(Creature* attacker) override;
  virtual void makeMove() override;
  virtual void sleeping() override;
  virtual bool isPlayer() const override;
  virtual void privateMessage(const PlayerMessage& message) override;
  virtual MessageGenerator& getMessageGenerator() const override;
  virtual void grantWish(TString) override;

  // overridden by subclasses
  struct CommandInfo {
    PlayerInfo::CommandInfo commandInfo;
    function<void(Player*)> perform;
    bool actionKillsController;
  };
  virtual vector<CommandInfo> getCommands() const;
  virtual void onLostControl();
  virtual bool isTravelEnabled() const;
  virtual bool handleUserInput(UserInput);
  virtual void updateUnknownLocations();

  struct OtherCreatureCommand {
    int priority;
    ViewObjectAction name;
    bool allowAuto;
    function<void(Player*)> perform;
  };
  virtual vector<OtherCreatureCommand> getOtherCreatureCommands(Creature*) const;

  optional<Vec2> chooseDirection(const TString& question);
  using TargetResult = variant<none_t, Position, Keybinding>;
  TargetResult chooseTarget(Table<PassableInfo> passable, TargetType, const TString& message,
    optional<Keybinding>);

  SMapMemory SERIAL(levelMemory);
  SMessageBuffer SERIAL(messageBuffer);
  SVisibilityMap SERIAL(visibilityMap);
  SUnknownLocations SERIAL(unknownLocations);
  void showHistory();
  Game* getGame() const;
  Model* getModel() const;
  View* getView() const;

  bool tryToPerform(CreatureAction);

  bool canTravel() const;

  private:
  Level* getLevel() const;
  void considerKeeperModeTravelMusic();
  void creatureClickAction(Position, bool extended);
  void pickUpItemAction(int item, bool multi = false);
  void equipmentAction();
  void applyItem(vector<Item*> item);
  void throwItem(Item* item, optional<Position> target = none);
  void takeOffAction();
  void hideAction();
  void displayInventory();
  void handleItems(const vector<UniqueEntity<Item>::Id>&, ItemAction);
  void handleIntrinsicAttacks(const vector<UniqueEntity<Item>::Id>&, ItemAction);
  bool interruptedByEnemy();
  void targetAction();
  void payForAllItemsAction();
  void payForItemAction(const vector<Item*>&);
  void chatAction(optional<Vec2> dir = none);
  void mountAction();
  void giveAction(vector<Item*>);
  void spellAction(int);
  void fireAction();
  void scrollStairs(int);
  void tryToCast(const Spell*, Position target);
  bool teamCanSeeAndSameLevel(Position) const;
  string getInventoryItemName(const Item*, bool plural) const;
  string getPluralName(Item* item, int num);
  Vec2 SERIAL(travelDir);
  optional<Position> SERIAL(target);
  void retireMessages();
  ItemInfo getFurnitureUsageInfo(const TString& question, ViewId viewId) const;
  optional<FurnitureUsageType> getUsableUsageType() const;
  STutorial SERIAL(tutorial);
  virtual vector<TeamMemberAction> getTeamMemberActions(const Creature*) const;
  optional<GlobalTime> lastEnemyInterruption;
  void updateSquareMemory(Position);
  HeapAllocated<DungeonLevel> SERIAL(avatarLevel);
  vector<HashSet<ViewIdList>> halluIds;
  void generateHalluIds();
  ViewIdList shuffleViewId(const ViewIdList&) const;
  void fillCurrentLevelInfo(GameInfo&) const;
  optional<SpellId> highlightedSpell;
  optional<GlobalTime> friendlyFireWarningCooldown;
  void transferAction();
  vector<FXSpawnInfo> deferredAnimations;
};

