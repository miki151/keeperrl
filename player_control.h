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

#ifndef _PLAYER_CONTROL_H
#define _PLAYER_CONTROL_H

#include "creature_view.h"
#include "entity_set.h"
#include "collective_control.h"
#include "game_info.h"
#include "map_memory.h"
#include "position.h"
#include "collective_warning.h"
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
template <typename T>
class EventProxy;
class SquareType;
class CostInfo;
struct WorkshopItem;

class PlayerControl : public CreatureView, public CollectiveControl {
  public:
  PlayerControl(Collective*, Level*);
  ~PlayerControl();
  void addImportantLongMessage(const string&, optional<Position> = none);

  void processInput(View* view, UserInput);
  MoveInfo getMove(Creature* c);

  const Creature* getKeeper() const;
  Creature* getKeeper();

  void render(View*);

  bool isTurnBased();
  void leaveControl();
  bool swapTeam();
  void onControlledKilled();

  enum class RequirementId {
    TECHNOLOGY,
    VILLAGE_CONQUERED,
  };
  typedef EnumVariant<RequirementId, TYPES(TechId),
      ASSIGN(TechId, RequirementId::TECHNOLOGY)> Requirement;

  static string getRequirementText(Requirement);

  struct RoomInfo {
    string name;
    string description;
    vector<Requirement> requirements;
  };
  static vector<RoomInfo> getRoomInfo();

  SERIALIZATION_DECL(PlayerControl);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  vector<Creature*> getTeam(const Creature*);

  protected:
  // from CreatureView
  virtual Level* getLevel() const override;
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getLocalTime() const override;
  virtual bool isPlayerView() const override;

  // from CollectiveControl
  virtual void addAttack(const CollectiveAttack&) override;
  virtual void addMessage(const PlayerMessage&) override;
  virtual void onMemberKilled(const Creature* victim, const Creature* killer) override;
  virtual void onConstructed(Position, const SquareType&) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onNoEnemies() override;
  virtual void tick() override;
  virtual void update(bool currentlyActive) override;

  private:
  HeapAllocated<EventProxy<PlayerControl>> SERIAL(eventProxy);
  friend EventProxy<PlayerControl>;
  void onEvent(const GameEvent&);

  void considerNightfallMessage();
  void considerWarning();

  TribeId getTribeId() const;
  bool canSee(const Creature*) const;
  bool canSee(Position) const;
  void initialize();
  bool isConsideredAttacking(const Creature*);

  void checkKeeperDanger();
  static string getWarningText(CollectiveWarning);
  void updateSquareMemory(Position);
  void updateKnownLocations(const Position&);
  bool isEnemy(const Creature*) const;
  vector<Collective*> getKnownVillains(VillainType) const;
  Collective* getVillain(int num);
  void scrollToMiddle(const vector<Position>&);

  Creature* getConsumptionTarget(View*, Creature* consumer);
  Creature* getCreature(UniqueEntity<Creature>::Id id) const;
  void controlSingle(const Creature*);
  void commandTeam(TeamId);
  void setScrollPos(Position);

  struct BuildInfo;
  bool meetsRequirement(Requirement) const;
  bool canSelectRectangle(const BuildInfo&);
  void handleSelection(Vec2 pos, const BuildInfo&, bool rectangle, bool deselectOnly = false);
  vector<CollectiveInfo::Button> fillButtons(const vector<BuildInfo>& buildInfo) const;
  VillageInfo::Village getVillageInfo(const Collective* enemy) const;
  void fillWorkshopInfo(CollectiveInfo&) const;
  static const vector<BuildInfo>& getBuildInfo();
  static vector<BuildInfo> workshopInfo;
  static vector<BuildInfo> libraryInfo;
  static vector<BuildInfo> minionsInfo;

  typedef CollectiveInfo::TechButton TechButton;

  int getMinLibrarySize() const;

  struct TechInfo {
    TechButton button;
    function<void(PlayerControl*, View*)> butFun;
  };
  vector<TechInfo> getTechInfo() const;
  struct WorkshopInfo {
    CollectiveInfo::WorkshopButton button;
    WorkshopType workshopType;
  };
  vector<WorkshopInfo> getWorkshopInfo() const;

  int getImpCost() const;
  void getEquipmentItem(View* view, ItemPredicate predicate);
  ItemInfo getWorkshopItem(const WorkshopItem&) const;
  Item* chooseEquipmentItem(Creature* creature, vector<Item*> currentItems, ItemPredicate predicate,
      double* scrollPos = nullptr);

  int getNumMinions() const;
  void minionTaskAction(const TaskActionInfo&);
  void minionDragAndDrop(const CreatureDropInfo&);
  void fillMinions(CollectiveInfo&) const;
  vector<Creature*> getMinionsLike(Creature*) const;
  vector<PlayerInfo> getPlayerInfos(vector<Creature*>) const;
  void sortMinionsForUI(vector<Creature*>&) const;
  vector<CollectiveInfo::CreatureGroup> getCreatureGroups(vector<Creature*>) const;
  vector<CollectiveInfo::CreatureGroup> getEnemyGroups() const;
  void minionEquipmentAction(const EquipmentActionInfo&);
  void addEquipment(Creature*, EquipmentSlot);
  void addConsumableItem(Creature*);
  void handleEquipment(View* view, Creature* creature);
  void fillEquipment(Creature*, PlayerInfo&) const;
  void handlePersonalSpells(View*);
  void handleLibrary(View*);
  void handleRecruiting(Collective* ally);
  void handleTrading(Collective* ally);
  void handleRansom(bool pay);
  static ViewObject getTrapObject(TrapType, bool built);
  void addToMemory(Position);
  void getSquareViewIndex(Position, bool canSee, ViewIndex&) const;
  void onSquareClick(Position);
  Creature* getControlled() const;
  optional<TeamId> getCurrentTeam() const;
  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  Model* getModel();
  const Model* getModel() const;
  Game* getGame() const;
  View* getView() const;

  mutable unique_ptr<MapMemory> SERIAL(memory);
  bool SERIAL(showWelcomeMsg) = true;
  struct SelectionInfo {
    Vec2 corner1;
    Vec2 corner2;
    bool deselect;
  };
  optional<SelectionInfo> rectSelection;
  void updateSelectionSquares();
  double SERIAL(lastControlKeeperQuestion) = -100;
  int SERIAL(startImpNum) = -1;
  optional<UniqueEntity<Creature>::Id> chosenCreature;
  void setChosenCreature(optional<UniqueEntity<Creature>::Id>);
  optional<WorkshopType> chosenWorkshop;
  void setChosenWorkshop(optional<WorkshopType>);
  optional<TeamId> getChosenTeam() const;
  void setChosenTeam(optional<TeamId>, optional<UniqueEntity<Creature>::Id> = none);
  optional<TeamId> chosenTeam;
  void clearChosenInfo();
  unordered_set<Position, CustomHash<Position>> SERIAL(surprises);
  string getMinionName(CreatureId) const;
  vector<PlayerMessage> SERIAL(messages);
  vector<PlayerMessage> SERIAL(messageHistory);
  vector<CollectiveAttack> SERIAL(newAttacks);
  vector<CollectiveAttack> SERIAL(ransomAttacks);
  EnumMap<CollectiveWarning, double> SERIAL(warningTimes);
  double SERIAL(lastWarningTime) = -10000;
  vector<string> SERIAL(hints);
  optional<PlayerMessage> findMessage(PlayerMessage::Id);
  void updateVisibleCreatures();
  vector<Vec2> SERIAL(visibleEnemies);
  HeapAllocated<VisibilityMap> SERIAL(visibilityMap);
  set<const Location*> SERIAL(knownLocations);
  set<const Collective*> SERIAL(knownVillains);
  set<const Collective*> SERIAL(knownVillainLocations);
  bool firstRender = true;
  bool isNight = true;
  optional<UniqueEntity<Creature>::Id> draggedCreature;
};

#endif
