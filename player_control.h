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
#include "event.h"
#include "collective_control.h"
#include "event.h"
#include "cost_info.h"
#include "game_info.h"
#include "square_type.h"
#include "position.h"

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
struct EquipmentActionInfo;

class PlayerControl : public CreatureView, public CollectiveControl {
  public:
  PlayerControl(Collective*, Model*, Level*);
  ~PlayerControl();
  void addImportantLongMessage(const string&, optional<Position> = none);

  void onConqueredLand();

  void processInput(View* view, UserInput);
  MoveInfo getMove(Creature* c);

  bool isRetired() const;
  const Creature* getKeeper() const;
  Creature* getKeeper();
  void addImp(Creature*);
  void addKeeper(Creature*);

  void render(View*);

  bool isTurnBased();
  void retire();
  void leaveControl();

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
  static vector<RoomInfo> getWorkshopInfo();

  SERIALIZATION_DECL(PlayerControl);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  void onTechBookRead(Technology*);
  vector<Creature*> getTeam(const Creature*);

  protected:
  // from CreatureView
  virtual const Level* getLevel() const;
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual Vec2 getPosition() const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual vector<Vec2> getVisibleEnemies() const override;
  virtual double getTime() const override;
  virtual bool isPlayerView() const override;

  // from CollectiveControl
  virtual void update(Creature*) override;
  virtual void addAttack(const CollectiveAttack&) override;
  virtual void addMessage(const PlayerMessage&) override;
  virtual void onDiscoveredLocation(const Location*) override;
  virtual void onMemberKilled(const Creature* victim, const Creature* killer) override;
  virtual void onConstructed(Position, const SquareType&) override;
  virtual void onNoEnemies() override;
  virtual void tick(double) override;

  private:

  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items);

  void considerNightfall();

  friend class KeeperControlOverride;

  Level* getLevel();
  const Tribe* getTribe() const;
  Tribe* getTribe();
  bool canSee(const Creature*) const;
  bool canSee(Position) const;
  void initialize();

  void considerDeityFight();
  void checkKeeperDanger();
  static string getWarningText(CollectiveWarning);
  void updateSquareMemory(Position);
  bool isEnemy(const Creature*) const;

  Creature* getConsumptionTarget(View*, Creature* consumer);
  void onWorshipEpithet(EpithetId);
  Creature* getCreature(UniqueEntity<Creature>::Id id);
  void handleAddToTeam(Creature* c);
  void controlSingle(const Creature*);
  void commandTeam(TeamId);

  struct BuildInfo;
  bool meetsRequirement(Requirement) const;
  void handleSelection(Vec2 pos, const BuildInfo&, bool rectangle, bool deselectOnly = false);
  vector<CollectiveInfo::Button> fillButtons(const vector<BuildInfo>& buildInfo) const;
  VillageInfo::Village getVillageInfo(const Collective* enemy) const;
  vector<BuildInfo> getBuildInfo() const;
  static vector<BuildInfo> getBuildInfo(const Level*, const Tribe*);
  static vector<BuildInfo> workshopInfo;
  static vector<BuildInfo> libraryInfo;
  static vector<BuildInfo> minionsInfo;

  ViewId getResourceViewId(CollectiveResourceId id) const;
  optional<pair<ViewId, int>> getCostObj(CostInfo) const;
  CostInfo getRoomCost(SquareType, CostInfo baseCost, double exponent) const;

  typedef CollectiveInfo::TechButton TechButton;

  int getMinLibrarySize() const;

  struct TechInfo {
    TechButton button;
    function<void(PlayerControl*, View*)> butFun;
  };
  vector<TechInfo> getTechInfo() const;

  int getImpCost() const;
  bool canBuildDoor(Position) const;
  bool canPlacePost(Position) const;
  void getEquipmentItem(View* view, ItemPredicate predicate);
  Item* chooseEquipmentItem(Creature* creature, vector<Item*> currentItems, ItemPredicate predicate,
      double* scrollPos = nullptr);

  int getNumMinions() const;
  void minionView(Creature* creature);
  void minionTaskAction(Creature*, const TaskActionInfo&);
  vector<PlayerInfo> getMinionGroup(Creature* like);
  void minionEquipmentAction(Creature* creature, const EquipmentActionInfo&);
  void addEquipment(Creature*, EquipmentSlot);
  void addConsumableItem(Creature*);
  void handleEquipment(View* view, Creature* creature);
  void fillEquipment(Creature*, PlayerInfo&);
  void handlePersonalSpells(View*);
  void handleLibrary(View*);
  void handleRecruiting(Collective* ally);
  void handleTrading(Collective* ally);
  void handleRansom(bool pay);
  static ViewObject getTrapObject(TrapType, bool built);
  void addToMemory(Position);
  void getSquareViewIndex(Position, bool canSee, ViewIndex&) const;
  void tryLockingDoor(Position);
  void uncoverRandomLocation();
  Creature* getControlled();
  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;

  mutable unique_ptr<MapMemory> SERIAL(memory);
  optional<TeamId> getCurrentTeam() const;
  void setCurrentTeam(optional<TeamId>);
  optional<TeamId> currentTeam;
  Model* SERIAL(model);
  bool SERIAL(showWelcomeMsg) = true;
  struct SelectionInfo {
    Vec2 corner1;
    Vec2 corner2;
    bool deselect;
  };
  optional<SelectionInfo> rectSelection;
  double SERIAL(lastControlKeeperQuestion) = -100;
  int SERIAL(startImpNum) = -1;
  bool SERIAL(retired) = false;
  bool SERIAL(payoutWarning) = false;
  unordered_set<Position> SERIAL(surprises);
  string getMinionName(CreatureId) const;
  vector<PlayerMessage> SERIAL(messages);
  vector<CollectiveAttack> SERIAL(newAttacks);
  vector<CollectiveAttack> SERIAL(ransomAttacks);
  struct CurrentWarningInfo {
    CollectiveWarning SERIAL(warning);
    double SERIAL(lastView);
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar & SVAR(warning) & SVAR(lastView);
    }
  };
  optional<CurrentWarningInfo> SERIAL(currentWarning);
  vector<string> SERIAL(hints);
  optional<PlayerMessage> findMessage(PlayerMessage::Id);
  void updateVisibleCreatures();
  vector<const Creature*> SERIAL(visibleEnemies);
  vector<const Creature*> SERIAL(visibleFriends);
  unordered_set<const Collective*> SERIAL(notifiedConquered);
  bool newTeam = false;
  HeapAllocated<VisibilityMap> SERIAL(visibilityMap);
  bool firstRender = true;
  bool isNight = true;
};

#endif
