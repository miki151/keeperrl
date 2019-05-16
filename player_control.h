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

#include "creature_view.h"
#include "entity_set.h"
#include "collective_control.h"
#include "game_info.h"
#include "map_memory.h"
#include "position.h"
#include "event_listener.h"
#include "keeper_creature_info.h"

class Model;
class Technology;
class View;
class Collective;
class CollectiveTeams;
class MapMemory;
class VisibilityMap;
class ListElem;
class UserInput;
class MinionAction;
struct TaskActionInfo;
struct CreatureDropInfo;
struct EquipmentActionInfo;
struct TeamCreatureInfo;
class CostInfo;
struct WorkshopItem;
struct WorkshopQueuedItem;
class ScrollPosition;
class Tutorial;
struct BuildInfo;
class MoveInfo;
class UnknownLocations;
class AttackTrigger;
class ImmigrantInfo;
class GameConfig;

class PlayerControl : public CreatureView, public CollectiveControl, public EventListener<PlayerControl> {
  public:
  static PPlayerControl create(WCollective col, vector<string> introText, KeeperCreatureInfo);
  ~PlayerControl() override;

  void processInput(View* view, UserInput);
  MoveInfo getMove(Creature* c);

  const Creature* getKeeper() const;
  Creature* getKeeper();

  void render(View*);
  optional<string> reloadImmigrationAndWorkshops(const GameConfig*, ContentFactory*);

  void leaveControl();
  void teamMemberAction(TeamMemberAction, UniqueEntity<Creature>::Id);
  void toggleControlAllTeamMembers();
  void onControlledKilled(const Creature* victim);
  void onSunlightVisibilityChanged();
  void setTutorial(STutorial);
  STutorial getTutorial() const;
  bool isEnemy(const Creature*) const;
  void addToMemory(Position);

  SERIALIZATION_DECL(PlayerControl)

  vector<Creature*> getTeam(const Creature*);
  optional<FurnitureType> getMissingTrainingDummy(const Creature*);
  bool canControlInTeam(const Creature*) const;
  bool canControlSingle(const Creature*) const;
  void addToCurrentTeam(Creature* c);
  void updateUnknownLocations();
  vector<Creature*> getConsumptionTargets(Creature* consumer) const;
  const KeeperCreatureInfo& getKeeperCreatureInfo() const;

  void onEvent(const GameEvent&);
  const vector<Creature*>& getControlled() const;

  optional<TeamId> getCurrentTeam() const;
  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  WModel getModel() const;

  private:
  struct Private {};

  public:
  PlayerControl(Private, WCollective, KeeperCreatureInfo);

  protected:
  // from CreatureView
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getScrollCoord() const override;
  virtual Level* getCreatureViewLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getAnimationTime() const override;
  virtual CenterType getCenterType() const override;
  virtual const vector<Vec2>& getUnknownLocations(WConstLevel) const override;

  // from CollectiveControl
  virtual void addAttack(const CollectiveAttack&) override;
  virtual void addMessage(const PlayerMessage&) override;
  virtual void onMemberKilled(const Creature* victim, const Creature* killer) override;
  virtual void onMemberAdded(Creature*) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;
  virtual void onClaimedSquare(Position) override;
  virtual void onNoEnemies() override;
  virtual void tick() override;
  virtual void update(bool currentlyActive) override;

  private:

  WLevel getCurrentLevel() const;
  void considerNightfallMessage();
  void considerWarning();

  TribeId getTribeId() const;
  bool canSee(const Creature*) const;
  bool canSee(Position) const;
  bool isConsideredAttacking(const Creature*, WConstCollective enemy);

  void checkKeeperDanger();
  static string getWarningText(CollectiveWarning);
  void updateSquareMemory(Position);
  void updateKnownLocations(const Position&);
  vector<WCollective> getKnownVillains() const;
  WCollective getVillain(UniqueEntity<Collective>::Id num);

  Creature* getConsumptionTarget(View*, Creature* consumer);
  Creature* getCreature(UniqueEntity<Creature>::Id id) const;
  void controlSingle(Creature*);
  void commandTeam(TeamId);
  void setScrollPos(Position);

  void handleSelection(Vec2 pos, const BuildInfo&, bool rectangle, bool deselectOnly = false);
  vector<CollectiveInfo::Button> fillButtons() const;
  VillageInfo::Village getVillageInfo(WConstCollective enemy) const;
  string getTriggerLabel(const AttackTrigger&) const;
  void fillWorkshopInfo(CollectiveInfo&) const;
  void fillImmigration(CollectiveInfo&) const;
  void fillImmigrationHelp(CollectiveInfo&) const;
  void fillLibraryInfo(CollectiveInfo&) const;
  void fillCurrentLevelInfo(GameInfo&) const;

  void getEquipmentItem(View* view, ItemPredicate predicate);
  ItemInfo getWorkshopItem(const WorkshopItem&, int queuedCount) const;
  Item* chooseEquipmentItem(Creature* creature, vector<Item*> currentItems, ItemPredicate predicate,
      ScrollPosition* scrollPos = nullptr);

  int getNumMinions() const;
  void minionTaskAction(const TaskActionInfo&);
  void minionDragAndDrop(const CreatureDropInfo&);
  void fillMinions(CollectiveInfo&) const;
  vector<Creature*> getMinionsLike(Creature*) const;
  vector<PlayerInfo> getPlayerInfos(vector<Creature*>, UniqueEntity<Creature>::Id chosenId) const;
  void sortMinionsForUI(vector<Creature*>&) const;
  vector<CollectiveInfo::CreatureGroup> getCreatureGroups(vector<Creature*>) const;
  vector<CollectiveInfo::CreatureGroup> getEnemyGroups() const;
  void minionEquipmentAction(const EquipmentActionInfo&);
  void addEquipment(Creature*, EquipmentSlot);
  void addConsumableItem(Creature*);
  void handleEquipment(View* view, Creature* creature);
  void fillEquipment(Creature*, PlayerInfo&) const;
  void handleTrading(WCollective ally);
  vector<Item*> getPillagedItems(WCollective) const;
  bool canPillage(WConstCollective) const;
  void handlePillage(WCollective enemy);
  void handleRansom(bool pay);
  ViewObject getTrapObject(FurnitureType, bool built) const;
  void getSquareViewIndex(Position, bool canSee, ViewIndex&) const;
  void onSquareClick(Position);
  WGame getGame() const;
  View* getView() const;
  PController createMinionController(Creature*);

  mutable SMapMemory SERIAL(memory);
  vector<string> SERIAL(introText);
  struct SelectionInfo {
    Vec2 corner1;
    Vec2 corner2;
    bool deselect;
  };
  optional<SelectionInfo> rectSelection;
  void updateSelectionSquares();
  GlobalTime SERIAL(lastControlKeeperQuestion) = GlobalTime(-1000);
  optional<UniqueEntity<Creature>::Id> chosenCreature;
  void setChosenCreature(optional<UniqueEntity<Creature>::Id>);
  optional<WorkshopType> chosenWorkshop;
  void setChosenWorkshop(optional<WorkshopType>);
  optional<TeamId> getChosenTeam() const;
  void setChosenTeam(optional<TeamId>, optional<UniqueEntity<Creature>::Id> = none);
  optional<TeamId> chosenTeam;
  void clearChosenInfo();
  bool chosenLibrary = false;
  string getMinionName(CreatureId) const;
  vector<PlayerMessage> SERIAL(messages);
  vector<PlayerMessage> SERIAL(messageHistory);
  vector<CollectiveAttack> SERIAL(newAttacks);
  vector<CollectiveAttack> SERIAL(notifiedAttacks);
  vector<CollectiveAttack> SERIAL(ransomAttacks);
  vector<string> SERIAL(hints);
  optional<PlayerMessage&> findMessage(PlayerMessage::Id);
  void updateVisibleCreatures();
  vector<Vec2> SERIAL(visibleEnemies);
  SVisibilityMap SERIAL(visibilityMap);
  bool firstRender = true;
  bool isNight = true;
  optional<UniqueEntity<Creature>::Id> draggedCreature;
  void updateMinionVisibility(const Creature*);
  STutorial SERIAL(tutorial);
  void setChosenLibrary(bool);
  void acquireTech(TechId);
  SMessageBuffer SERIAL(controlModeMessages);
  unordered_set<int> dismissedNextWaves;
  vector<ImmigrantDataInfo> getPrisonerImmigrantData() const;
  void acceptPrisoner(int index);
  void rejectPrisoner(int index);
  struct StunnedInfo {
    vector<Creature*> creatures;
    WCollective collective = nullptr;
  };
  vector<StunnedInfo> getPrisonerImmigrantStack() const;
  vector<pair<Creature*, WCollective>> SERIAL(stunnedCreatures);
  string getMinionGroupName(Creature*) const;
  ViewId getMinionGroupViewId(Creature*) const;
  SUnknownLocations SERIAL(unknownLocations);
  optional<LocalTime> lastWarningDismiss;
  set<pair<UniqueEntity<Collective>::Id, string>> SERIAL(dismissedVillageInfos);
  void considerTransferingLostMinions();
  vector<PItem> retrievePillageItems(WCollective, vector<Item*> items);
  KeeperCreatureInfo SERIAL(keeperCreatureInfo);
  vector<BuildInfo> SERIAL(buildInfo);
  void reloadData();
  void reloadBuildingMenu();
  WLevel currentLevel = nullptr;
  void scrollStairs(bool up);
  CollectiveInfo::QueuedItemInfo getQueuedItemInfo(const WorkshopQueuedItem&) const;
  vector<pair<vector<Item*>, Position>> getItemUpgradesFor(const WorkshopItem&) const;
  void fillDungeonLevel(AvatarLevelInfo&) const;
};

