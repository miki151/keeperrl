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
#include "workshop_type.h"
#include "resource_id.h"

class Model;
class Technology;
class View;
class Collective;
class CollectiveTeams;
class MapMemory;
class VisibilityMap;
class UserInput;
class MinionAction;
struct TaskActionInfo;
struct EquipmentGroupAction;
struct AIActionInfo;
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
struct WorkshopOptionInfo;
struct FurnaceOptionInfo;
class ScriptedUIState;
namespace BuildInfoTypes {
  struct BuildType;
}

class PlayerControl : public CreatureView, public CollectiveControl, public EventListener<PlayerControl> {
  public:
  static PPlayerControl create(Collective* col, vector<string> introText, TribeAlignment);
  ~PlayerControl() override;

  void processInput(View* view, UserInput);
  MoveInfo getMove(Creature* c);

  void render(View*);
  void loadImmigrationAndWorkshops(ContentFactory*, const KeeperCreatureInfo&);

  void leaveControl();
  void teamMemberAction(TeamMemberAction, UniqueEntity<Creature>::Id);
  void toggleControlAllTeamMembers();
  void onControlledKilled(Creature* victim);
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
  TribeAlignment getTribeAlignment() const;

  void onEvent(const GameEvent&);
  const vector<Creature*>& getControlled() const;
  void controlSingle(Creature*);
  void checkKeeperDanger();
  void dismissKeeperWarning();

  optional<TeamId> getCurrentTeam() const;
  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  WModel getModel() const;
  void takeScreenshot();
  void addAllianceAttack(vector<Collective*> attackers);

  private:
  struct Private {};

  public:
  PlayerControl(Private, Collective*, TribeAlignment);

  //protected:
  // from CreatureView
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getScrollCoord() const override;
  virtual Level* getCreatureViewLevel() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getAnimationTime() const override;
  virtual CenterType getCenterType() const override;
  virtual const vector<Vec2>& getUnknownLocations(const Level*) const override;
  virtual optional<Vec2> getSelectionSize() const override;
  virtual vector<vector<Vec2>> getPathTo(UniqueEntity<Creature>::Id, Vec2) const override;
  virtual vector<vector<Vec2>> getGroupPathTo(const string&, Vec2) const override;
  virtual vector<vector<Vec2>> getTeamPathTo(TeamId, Vec2) const override;
  virtual vector<Vec2> getHighlightedPathTo(Vec2) const override;
  virtual CreatureView::PlacementInfo canPlaceItem(Vec2, int) const override;
  optional<QuartersInfo> getQuarters(Vec2) const override;

  // from CollectiveControl
  virtual void addAttack(const CollectiveAttack&) override;
  virtual void addMessage(const PlayerMessage&) override;
  virtual void addWindowMessage(ViewIdList, const string&) override;
  virtual void onMemberKilledOrStunned(Creature* victim, const Creature* killer) override;
  virtual void onConquered(Creature* victim, Creature* killer) override;
  virtual void onMemberAdded(Creature*) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;
  virtual void onClaimedSquare(Position) override;
  virtual void tick() override;
  virtual void update(bool currentlyActive) override;

  private:

  bool canAllyJoin(Creature* ally) const;
  Level* getCurrentLevel() const;
  void considerNightfallMessage();
  void considerWarning();
  void considerAllianceAttack();
  void considerNewAttacks();

  TribeId getTribeId() const;
  bool canSee(const Creature*) const;
  bool canSee(Position) const;
  bool isConsideredAttacking(const Creature*, const Collective* enemy);

  static string getWarningText(CollectiveWarning);
  void updateSquareMemory(Position);
  void updateKnownLocations(const Position&);
  vector<Collective*> getKnownVillains() const;
  Collective* getVillain(UniqueEntity<Collective>::Id num);

  Creature* getConsumptionTarget(View*, Creature* consumer);
  Creature* getCreature(UniqueEntity<Creature>::Id id) const;
  void commandTeam(TeamId);
  void setScrollPos(Position);

  void handleSelection(Vec2 pos, const BuildInfo&, bool dryRun);
  void handleSelection(Position, const BuildInfoTypes::BuildType&, bool dryRun);
  vector<CollectiveInfo::Button> fillButtons() const;
  VillageInfo::Village getVillageInfo(const Collective* enemy) const;
  string getTriggerLabel(const AttackTrigger&) const;
  void fillWorkshopInfo(CollectiveInfo&) const;
  vector<CollectiveInfo::QueuedItemInfo> getQueuedWorkshopItems() const;
  vector<CollectiveInfo::QueuedItemInfo> getFurnaceQueue() const;
  vector<WorkshopOptionInfo> getWorkshopOptions(int resourceIndex) const;
  vector<FurnaceOptionInfo> getFurnaceOptions() const;
  void fillImmigration(CollectiveInfo&) const;
  void fillImmigrationHelp(CollectiveInfo&) const;
  void fillLibraryInfo(CollectiveInfo&) const;
  void fillTechUnlocks(CollectiveInfo::LibraryInfo::TechInfo&) const;
  void fillCurrentLevelInfo(GameInfo&) const;

  void getEquipmentItem(View* view, ItemPredicate predicate);
  ItemInfo getWorkshopItem(const WorkshopItem&, int queuedCount) const;
  Item* chooseEquipmentItem(Creature* creature, vector<Item*> currentItems, ItemPredicate,
      ScriptedUIState* uiState = nullptr);
  Creature* chooseSteed(Creature* creature, vector<Creature*> allSteeds);

  int getNumMinions() const;
  void minionTaskAction(const TaskActionInfo&);
  void equipmentGroupAction(const EquipmentGroupAction&);
  void minionAIAction(const AIActionInfo&);
  void minionDragAndDrop(Vec2 pos, variant<string, UniqueEntity<Creature>::Id>);
  void fillMinions(CollectiveInfo&) const;
  vector<Creature*> getMinionGroup(const string& groupName) const;
  vector<PlayerInfo> getPlayerInfos(vector<Creature*>) const;
  void sortMinionsForUI(vector<Creature*>&) const;
  vector<CollectiveInfo::CreatureGroup> getCreatureGroups(vector<Creature*>) const;
  vector<CollectiveInfo::CreatureGroup> getAutomatonGroups(vector<Creature*>) const;
  void minionEquipmentAction(const EquipmentActionInfo&);
  void addEquipment(Creature*, EquipmentSlot);
  void addConsumableItem(Creature*);
  void handleEquipment(View* view, Creature* creature);
  void fillSteedInfo(Creature*, PlayerInfo&) const;
  void fillEquipment(Creature*, PlayerInfo&) const;
  void fillPromotions(Creature*, CollectiveInfo&) const;
  void handleTrading(Collective* ally);
  vector<Item*> getPillagedItems(Collective*) const;
  bool canPillage(const Collective*) const;
  void handlePillage(Collective* enemy);
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
  GlobalTime SERIAL(nextKeeperWarning) = GlobalTime(-1000);
  struct ChosenCreatureInfo {
    UniqueEntity<Creature>::Id id;
    string group;
  };
  optional<ChosenCreatureInfo> chosenCreature;
  void setChosenCreature(optional<UniqueEntity<Creature>::Id>, string group);
  struct ChosenWorkshopInfo {
    int resourceIndex;
    WorkshopType type;
  };
  optional<ChosenWorkshopInfo> chosenWorkshop;
  void setChosenWorkshop(optional<ChosenWorkshopInfo>);
  optional<TeamId> getChosenTeam() const;
  void setChosenTeam(optional<TeamId>, optional<UniqueEntity<Creature>::Id> = none);
  optional<TeamId> chosenTeam;
  void clearChosenInfo();
  string getMinionName(CreatureId) const;
  vector<PlayerMessage> SERIAL(messages);
  vector<PlayerMessage> SERIAL(messageHistory);
  vector<CollectiveAttack> SERIAL(newAttacks);
  vector<CollectiveAttack> SERIAL(notifiedAttacks);
  vector<string> SERIAL(hints);
  optional<PlayerMessage&> findMessage(PlayerMessage::Id);
  SVisibilityMap SERIAL(visibilityMap);
  bool firstRender = true;
  bool isNight = true;
  optional<UniqueEntity<Creature>::Id> draggedCreature;
  void updateMinionVisibility(const Creature*);
  STutorial SERIAL(tutorial);
  void acquireTech(TechId);
  SMessageBuffer SERIAL(controlModeMessages);
  unordered_set<int> dismissedNextWaves;
  vector<ImmigrantDataInfo> getPrisonerImmigrantData() const;
  vector<ImmigrantDataInfo> getUnrealizedPromotionsImmigrantData() const;
  vector<ImmigrantDataInfo> getNecromancerImmigrationHelp() const;
  void acceptPrisoner(int index);
  void rejectPrisoner(int index);
  struct StunnedInfo {
    vector<Creature*> creatures;
    Collective* collective = nullptr;
  };
  vector<StunnedInfo> getPrisonerImmigrantStack() const;
  vector<pair<Creature*, Collective*>> SERIAL(stunnedCreatures);
  ViewIdList getMinionGroupViewId(Creature*) const;
  SUnknownLocations SERIAL(unknownLocations);
  optional<LocalTime> lastWarningDismiss;
  set<pair<UniqueEntity<Collective>::Id, string>> SERIAL(dismissedVillageInfos);
  void considerTransferingLostMinions();
  vector<PItem> retrievePillageItems(Collective*, vector<Item*> items);
  TribeAlignment SERIAL(tribeAlignment);
  vector<BuildInfo> SERIAL(buildInfo);
  void loadBuildingMenu(const ContentFactory*, const KeeperCreatureInfo&);
  Level* currentLevel = nullptr;
  void scrollStairs(int dir);
  CollectiveInfo::QueuedItemInfo getQueuedItemInfo(const WorkshopQueuedItem&, int cnt, int itemIndex) const;
  vector<pair<Item*, Position>> getItemUpgradesFor(const WorkshopItem&) const;
  void fillDungeonLevel(AvatarLevelInfo&) const;
  void fillResources(CollectiveInfo&) const;
  bool takingScreenshot = false;
  void addBodyPart(Creature*);
  void handleBanishing(Creature*);
  optional<pair<ViewId,int>> getCostObj(CostInfo) const;
  optional<pair<ViewId,int>> getCostObj(const optional<CostInfo>&) const;
  ViewId getViewId(const BuildInfoTypes::BuildType&) const;
  EntityMap<Creature, LocalTime> leaderWoundedTime;
  void handleDestructionOrder(Position position, HighlightType, DestroyAction, bool dryRun);
  unordered_set<CollectiveResourceId, CustomHash<CollectiveResourceId>> SERIAL(usedResources);
  optional<vector<Collective*>> SERIAL(allianceAttack);
  enum class Selection { SELECT, DESELECT, NONE } selection = Selection::NONE;
  void considerTogglingCaptureOrderOnMinions() const;
  struct BattleSummary {
    vector<Creature*> SERIAL(minionsKilled);
    vector<Creature*> SERIAL(minionsCaptured);
    vector<Creature*> SERIAL(enemiesKilled);
    vector<Creature*> SERIAL(enemiesCaptured);
    SERIALIZE_ALL(minionsKilled, minionsCaptured, enemiesKilled, enemiesCaptured)
  };
  BattleSummary SERIAL(battleSummary);
  void presentAndClearBattleSummary();
};
