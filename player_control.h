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
class ScrollPosition;
class Tutorial;
struct BuildInfo;
class MoveInfo;

class PlayerControl : public CreatureView, public CollectiveControl, public EventListener<PlayerControl> {
  public:
  static PPlayerControl create(WCollective col, vector<string> introText);
  ~PlayerControl();

  void processInput(View* view, UserInput);
  MoveInfo getMove(WCreature c);

  WConstCreature getKeeper() const;
  WCreature getKeeper();

  void render(View*);

  bool isTurnBased();
  void leaveControl();
  void teamMemberAction(TeamMemberAction, UniqueEntity<Creature>::Id);
  void toggleControlAllTeamMembers();
  void onControlledKilled(WConstCreature victim);
  void onSunlightVisibilityChanged();
  void setTutorial(STutorial);
  STutorial getTutorial() const;

  SERIALIZATION_DECL(PlayerControl)

  vector<WCreature> getTeam(WConstCreature);
  optional<FurnitureType> getMissingTrainingDummy(WConstCreature);
  bool canControlInTeam(WConstCreature) const;
  bool canControlSingle(WConstCreature) const;
  void addToCurrentTeam(WCreature c);

  void onEvent(const GameEvent&);
  const vector<WCreature>& getControlled() const;

  private:
  struct Private {};

  public:
  PlayerControl(Private, WCollective);

  protected:
  // from CreatureView
  virtual WLevel getLevel() const override;
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getAnimationTime() const override;
  virtual CenterType getCenterType() const override;
  virtual vector<Vec2> getUnknownLocations(WConstLevel) const override;

  // from CollectiveControl
  virtual void addAttack(const CollectiveAttack&) override;
  virtual void addMessage(const PlayerMessage&) override;
  virtual void onMemberKilled(WConstCreature victim, WConstCreature killer) override;
  virtual void onMemberAdded(WCreature) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onClaimedSquare(Position) override;
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;
  virtual void onNoEnemies() override;
  virtual void onPositionDiscovered(Position) override;
  virtual void tick() override;
  virtual void update(bool currentlyActive) override;

  private:

  void considerNightfallMessage();
  void considerWarning();

  TribeId getTribeId() const;
  bool canSee(WConstCreature) const;
  bool canSee(Position) const;
  void initialize();
  bool isConsideredAttacking(WConstCreature, WConstCollective enemy);

  void checkKeeperDanger();
  static string getWarningText(CollectiveWarning);
  void updateSquareMemory(Position);
  void updateKnownLocations(const Position&);
  bool isEnemy(WConstCreature) const;
  vector<WCollective> getKnownVillains() const;
  WCollective getVillain(UniqueEntity<Collective>::Id num);
  void scrollToMiddle(const vector<Position>&);

  WCreature getConsumptionTarget(View*, WCreature consumer);
  WCreature getCreature(UniqueEntity<Creature>::Id id) const;
  void controlSingle(WCreature);
  void commandTeam(TeamId);
  void setScrollPos(Position);

  bool canSelectRectangle(const BuildInfo&);
  void handleSelection(Vec2 pos, const BuildInfo&, bool rectangle, bool deselectOnly = false);
  vector<CollectiveInfo::Button> fillButtons(const vector<BuildInfo>& buildInfo) const;
  VillageInfo::Village getVillageInfo(WConstCollective enemy) const;
  void fillWorkshopInfo(CollectiveInfo&) const;
  void fillImmigration(CollectiveInfo&) const;
  void fillImmigrationHelp(CollectiveInfo&) const;
  void fillLibraryInfo(CollectiveInfo&) const;
  static const vector<BuildInfo>& getBuildInfo();

  typedef CollectiveInfo::TechButton TechButton;

  int getMinLibrarySize() const;

  struct TechInfo {
    TechButton button;
    function<void(PlayerControl*, View*)> butFun;
  };
  vector<TechInfo> getTechInfo() const;

  void getEquipmentItem(View* view, ItemPredicate predicate);
  ItemInfo getWorkshopItem(const WorkshopItem&) const;
  WItem chooseEquipmentItem(WCreature creature, vector<WItem> currentItems, ItemPredicate predicate,
      ScrollPosition* scrollPos = nullptr);

  int getNumMinions() const;
  void minionTaskAction(const TaskActionInfo&);
  void minionDragAndDrop(const CreatureDropInfo&);
  void fillMinions(CollectiveInfo&) const;
  vector<WCreature> getMinionsLike(WCreature) const;
  vector<PlayerInfo> getPlayerInfos(vector<WCreature>, UniqueEntity<Creature>::Id chosenId) const;
  void sortMinionsForUI(vector<WCreature>&) const;
  vector<CollectiveInfo::CreatureGroup> getCreatureGroups(vector<WCreature>) const;
  vector<CollectiveInfo::CreatureGroup> getEnemyGroups() const;
  void minionEquipmentAction(const EquipmentActionInfo&);
  void addEquipment(WCreature, EquipmentSlot);
  void addConsumableItem(WCreature);
  void handleEquipment(View* view, WCreature creature);
  void fillEquipment(WCreature, PlayerInfo&) const;
  void handleTrading(WCollective ally);
  void handlePillage(WCollective enemy);
  void handleRansom(bool pay);
  static ViewObject getTrapObject(TrapType, bool built);
  void addToMemory(Position);
  void getSquareViewIndex(Position, bool canSee, ViewIndex&) const;
  void onSquareClick(Position);
  optional<TeamId> getCurrentTeam() const;
  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  WModel getModel() const;
  WGame getGame() const;
  View* getView() const;
  PController createMinionController(WCreature);

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
  vector<CollectiveAttack> SERIAL(ransomAttacks);
  vector<string> SERIAL(hints);
  optional<PlayerMessage> findMessage(PlayerMessage::Id);
  void updateVisibleCreatures();
  vector<Vec2> SERIAL(visibleEnemies);
  SVisibilityMap SERIAL(visibilityMap);
  bool firstRender = true;
  bool isNight = true;
  optional<UniqueEntity<Creature>::Id> draggedCreature;
  void updateMinionVisibility(WConstCreature);
  STutorial SERIAL(tutorial);
  void setChosenLibrary(bool);
  void acquireTech(int index);
  SMessageBuffer SERIAL(controlModeMessages);
  unordered_set<int> dismissedNextWaves;
  vector<ImmigrantDataInfo> getPrisonerImmigrantData() const;
  void acceptPrisoner(int index);
  void rejectPrisoner(int index);
  struct StunnedInfo {
    vector<WCreature> creatures;
    WCollective collective;
  };
  vector<StunnedInfo> getPrisonerImmigrantStack() const;
  string getMinionGroupName(WCreature) const;
  ViewId getMinionGroupViewId(WCreature) const;
};

