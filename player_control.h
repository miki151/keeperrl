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

#include "map_memory.h"
#include "creature_view.h"
#include "task.h"
#include "entity_set.h"
#include "event.h"
#include "view.h"
#include "collective_control.h"
#include "collective.h"
#include "event.h"
#include "visibility_map.h"

class Model;
class Technology;
class View;

class PlayerControl : public CreatureView, public CollectiveControl {
  public:
  PlayerControl(Collective*, Model*, Level*);
  void addImportantLongMessage(const string&, optional<Vec2> = none);

  void onConqueredLand(const string& name);

  void processInput(View* view, UserInput);
  void tick(double);
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

  struct RoomInfo {
    string name;
    string description;
    optional<TechId> techId;
  };
  static vector<RoomInfo> getRoomInfo();
  static vector<RoomInfo> getWorkshopInfo();

  enum class MinionOption;

  SERIALIZATION_DECL(PlayerControl);

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  // from CreatureView
  virtual const Level* getLevel() const;
  virtual const MapMemory& getMemory() const override;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const override;
  virtual void refreshGameInfo(GameInfo&) const override;
  virtual optional<Vec2> getPosition(bool force) const override;
  virtual optional<MovementInfo> getMovementInfo() const override;
  virtual vector<const Creature*> getVisibleEnemies() const override;
  virtual double getTime() const override;

  // from CollectiveControl
  virtual void update(Creature*) override;
  virtual void addAssaultNotification(const Collective*, const vector<Creature*>&, const string& message) override;
  virtual void removeAssaultNotification(const Collective*) override;
  virtual void addMessage(const PlayerMessage&) override;
  virtual void onDiscoveredLocation(const Location*) override;
  virtual void onCreatureKilled(const Creature* victim, const Creature* killer) override;
  virtual void onConstructed(Vec2, SquareType) override;

  private:

  REGISTER_HANDLER(TechBookEvent, Technology*);
  REGISTER_HANDLER(WorshipEvent, Creature* who, const Deity* to, WorshipType);
  REGISTER_HANDLER(WorshipCreatureEvent, Creature* who, const Creature* to, WorshipType);
  REGISTER_HANDLER(SunlightChangeEvent);
  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items);

  friend class KeeperControlOverride;

  Level* getLevel();
  const Tribe* getTribe() const;
  Tribe* getTribe();
  bool canSee(const Creature*) const;
  bool canSee(Vec2 position) const;
  MapMemory& getMemory(Level* l);
  void initialize();

  void considerDeityFight();
  void checkKeeperDanger();
  void addDeityServant(Deity*, Vec2 deityPos, Vec2 victimPos);
  static string getWarningText(Collective::Warning);
  void updateSquareMemory(Vec2);
  bool isEnemy(const Creature*) const;

  Creature* getConsumptionTarget(View*, Creature* consumer);
  void onWorshipEpithet(EpithetId);
  Creature* getCreature(UniqueEntity<Creature>::Id id);
  void handleCreatureButton(Creature* c, View* view);
  void controlSingle(const Creature*);
  void commandTeam(TeamId);
  struct BuildInfo {
    struct SquareInfo {
      SquareType type;
      Collective::CostInfo cost;
      string name;
      bool buildImmediatly;
      bool noCredit;
    } squareInfo;

    struct TrapInfo {
      TrapType type;
      string name;
      ViewId viewId;
    } trapInfo;

    struct MinionInfo {
      CreatureId id;
      EnumSet<MinionTrait> traits;
      Collective::CostInfo cost;
    } minionInfo;

    enum BuildType {
      DIG,
      SQUARE,
      IMP,
      MINION,
      TRAP,
      GUARD_POST,
      DESTROY,
      FETCH,
      DISPATCH,
      CLAIM_TILE,
      TORCH,
    } buildType;

    optional<TechId> techId;
    string help;
    char hotkey;
    string groupName;

    BuildInfo(SquareInfo info, optional<TechId> techId = none, const string& h = "", char hotkey = 0,
        string group = "");
    BuildInfo(TrapInfo info, optional<TechId> techId = none, const string& h = "", char hotkey = 0,
        string group = "");
    BuildInfo(DeityHabitat, Collective::CostInfo, const string& groupName, const string& h = "", char hotkey = 0);
    BuildInfo(const Creature*, Collective::CostInfo, const string& groupName, const string& h = "", char hotkey = 0);
    BuildInfo(MinionInfo, optional<TechId> techId, const string& groupName, const string& h = "");
    BuildInfo(BuildType type, const string& h = "", char hotkey = 0, string group = "");
  };
  void handleSelection(Vec2 pos, const BuildInfo&, bool rectangle, bool deselectOnly = false);
  vector<GameInfo::BandInfo::Button> fillButtons(const vector<BuildInfo>& buildInfo) const;
  vector<BuildInfo> getBuildInfo() const;
  static vector<BuildInfo> getBuildInfo(const Level*, const Tribe*);
  static vector<BuildInfo> workshopInfo;
  static vector<BuildInfo> libraryInfo;
  static vector<BuildInfo> minionsInfo;

  ViewObject getResourceViewObject(Collective::ResourceId id) const;
  optional<pair<ViewObject, int>> getCostObj(Collective::CostInfo) const;

  typedef GameInfo::BandInfo::TechButton TechButton;

  int getMinLibrarySize() const;

  struct TechInfo {
    TechButton button;
    function<void(PlayerControl*, View*)> butFun;
  };
  vector<TechInfo> getTechInfo() const;

  int getImpCost() const;
  bool canBuildDoor(Vec2 pos) const;
  bool canPlacePost(Vec2 pos) const;
  void handleMarket(View*, int prevItem = 0);
  void getEquipmentItem(View* view, ItemPredicate predicate);
  Item* chooseEquipmentItem(View* view, vector<Item*> currentItems, ItemPredicate predicate,
      int* index = nullptr, int* scrollPos = nullptr) const;

  void getMinionOptions(Creature*, vector<MinionOption>&, vector<View::ListElem>&);

  int getNumMinions() const;
  void minionView(View* view, Creature* creature, int prevItem = 0);
  void handleEquipment(View* view, Creature* creature, int prevItem = 0);
  void handleNecromancy(View*);
  void handlePersonalSpells(View*);
  void handleLibrary(View*);
  static ViewObject getTrapObject(TrapType, bool built);
  bool underAttack() const;
  void addToMemory(Vec2 pos);
  void getSquareViewIndex(const Square*, bool canSee, ViewIndex&) const;
  bool tryLockingDoor(Vec2 pos);
  void uncoverRandomLocation();
  Creature* getControlled();

  mutable unique_ptr<map<UniqueEntity<Level>::Id, MapMemory>> SERIAL(memory);
  optional<TeamId> getCurrentTeam() const;
  void setCurrentTeam(optional<TeamId>);
  optional<TeamId> SERIAL(currentTeam);
  Model* SERIAL(model);
  bool SERIAL2(showWelcomeMsg, true);
  struct SelectionInfo {
    Vec2 corner1;
    Vec2 corner2;
    bool deselect;
  };
  optional<SelectionInfo> rectSelection;
  double SERIAL2(lastControlKeeperQuestion, -100);
  int SERIAL2(startImpNum, -1);
  bool SERIAL2(retired, false);
  bool SERIAL2(payoutWarning, false);
  unordered_set<Vec2> SERIAL(surprises);
  string getMinionName(CreatureId) const;
  ViewObject getMinionViewObject(CreatureId) const;
  vector<PlayerMessage> SERIAL(messages);
  struct AssaultInfo : public NamedTupleBase<string, vector<Creature*>> {
    NAMED_TUPLE_STUFF(AssaultInfo);
    NAME_ELEM(0, message);
    NAME_ELEM(1, creatures);
  };
  map<const Collective*, AssaultInfo> SERIAL(assaultNotifications);
  struct CurrentWarningInfo : public NamedTupleBase<Collective::Warning, double> {
    NAMED_TUPLE_STUFF(CurrentWarningInfo);
    NAME_ELEM(0, warning);
    NAME_ELEM(1, lastView);
  };
  optional<CurrentWarningInfo> SERIAL(currentWarning);
  vector<string> SERIAL(hints);
  mutable queue<Vec2> scrollPos;
  optional<PlayerMessage> findMessage(PlayerMessage::Id);
  void updateVisibleCreatures(Rectangle range);
  vector<const Creature*> SERIAL(visibleEnemies);
  vector<const Creature*> SERIAL(visibleFriends);
  unordered_set<const Collective*> SERIAL(notifiedConquered);
  bool newTeam = false;
  VisibilityMap SERIAL(visibilityMap);
  bool firstRender = true;
};

#endif
