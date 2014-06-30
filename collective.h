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

#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "map_memory.h"
#include "creature_view.h"
#include "markov_chain.h"
#include "minion_equipment.h"
#include "task.h"
#include "entity_set.h"
#include "sectors.h"
#include "event.h"
#include "spell_info.h"
#include "view.h"

enum class MinionType {
  IMP,
  NORMAL,
  UNDEAD,
  GOLEM,
  BEAST,
  KEEPER,
  PRISONER,
};

ENUM_HASH(MinionType);

enum class MinionTask { SLEEP,
  GRAVE,
  TRAIN,
  WORKSHOP,
  STUDY,
  LABORATORY,
  PRISON,
  TORTURE
};

class Model;
class Technology;
class View;

class Collective : public CreatureView, public EventListener, public Task::Callback {
  public:
  Collective(Model*, Level*, Tribe*);
  virtual const MapMemory& getMemory() const override;
  MapMemory& getMemory(Level* l);
  virtual ViewIndex getViewIndex(Vec2 pos) const override;
  virtual void refreshGameInfo(GameInfo&) const  override;
  virtual Vec2 getPosition() const  override;
  virtual bool canSee(const Creature*) const  override;
  virtual bool canSee(Vec2 position) const  override;
  virtual vector<const Creature*> getUnknownAttacker() const override;
  virtual Tribe* getTribe() const override;
  virtual bool isEnemy(const Creature*) const override;

  virtual bool staticPosition() const override;

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override;
  virtual void onCombatEvent(const Creature*) override;
  virtual void onTriggerEvent(const Level*, Vec2 pos) override;
  virtual void onSquareReplacedEvent(const Level*, Vec2 pos) override;
  virtual void onChangeLevelEvent(const Creature*, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) override;
  virtual void onAlarmEvent(const Level*, Vec2 pos) override;
  virtual void onTechBookEvent(Technology*) override;
  virtual void onEquipEvent(const Creature*, const Item*) override;
  virtual void onPickupEvent(const Creature* c, const vector<Item*>& items);
  virtual void onSurrenderEvent(Creature* who, const Creature* to);
  virtual void onTortureEvent(Creature* who, const Creature* torturer);

  void onConqueredLand(const string& name);

  void processInput(View* view, UserInput);
  void tick();
  void update(Creature*);
  MoveInfo getMove(Creature* c);
  void addCreature(Creature* c, MinionType);

  virtual const Level* getLevel() const;

  virtual void onConstructed(Vec2 pos, SquareType) override;
  virtual void onBrought(Vec2 pos, vector<Item*> items) override;
  virtual void onAppliedItem(Vec2 pos, Item* item) override;
  virtual void onAppliedSquare(Vec2 pos) override;
  virtual void onAppliedItemCancel(Vec2 pos) override;
  virtual void onPickedUp(Vec2 pos, EntitySet) override;
  virtual void onCantPickItem(EntitySet items) override;

  bool isRetired() const;
  const Creature* getKeeper() const;
  Vec2 getDungeonCenter() const;
  double getDangerLevel(bool includeExecutions = true) const;

  void render(View*);

  bool isTurnBased();
  void retire();

  struct RoomInfo {
    string name;
    string description;
    Optional<TechId> techId;
  };
  static vector<RoomInfo> getRoomInfo();
  static vector<RoomInfo> getWorkshopInfo();
  static vector<SpellInfo> getSpellLearning(const Technology*);

  static vector<CreatureId> getSpawnInfo(const Technology*);

  enum class ResourceId {
    GOLD,
    WOOD,
    IRON,
    STONE,
  };

  struct SpawnInfo {
    CreatureId id;
    int manaCost;
    Optional<TechId> techId;
  };

  enum class Warning { DIGGING, STORAGE, WOOD, IRON, STONE, GOLD, LIBRARY, MINIONS, BEDS, TRAINING, WORKSHOP, LABORATORY, NO_WEAPONS, GRAVES, CHESTS, NO_PRISON, LARGER_PRISON, TORTURE_ROOM, MORE_CHESTS, MANA};

  struct ResourceInfo {
    vector<SquareType> storageType;
    ItemPredicate predicate;
    ItemId itemId;
    string name;
    Warning warning;
  };

  static constexpr const char* const warningText[] {
    "Start digging into the mountain to build a dungeon.",
    "You need to build a storage room.",
    "Cut down some trees for wood.",
    "You need to mine more iron.",
    "You need to mine more stone.",
    "You need to mine more gold.",
    "Build a library to start research.",
    "Use the library tab in the top-right to summon some minions.",
    "You need to build beds for your minions.",
    "Build a training room for your minions.",
    "Build a workshop to produce equipment and traps.",
    "Build a laboratory to produce potions.",
    "You need weapons for your minions.",
    "You need a graveyard to collect corpses",
    "You need to build a treasure room.",
    "You need to build a prison.",
    "You need a larger prison.",
    "You need to build a torture room.",
    "You need a larger treasure room.",
    "Kill or torture some innocent beings for more mana.",
  };

  const static int numWarnings = 20;
  bool warning[numWarnings] = {0};

  enum class MinionOption { POSSESS, EQUIPMENT, INFO, WAKE_UP, PRISON, TORTURE, EXECUTE, LABOR, TRAINING,
    WORKSHOP, LAB, STUDY };

  SERIALIZATION_DECL(Collective);

  template <class Archive>
  static void registerTypes(Archive& ar);

  private:
  Creature* addCreature(PCreature c, Vec2 v, MinionType);
  Creature* getCreature(UniqueId id);
  void handleCreatureButton(Creature* c, View* view);
  void handleSurprise(Vec2 pos);
  bool knownPos(Vec2) const;
  void unpossess();
  void possess(const Creature*, View*);
  struct CostInfo {
    ResourceId id;
    int value;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };
  struct BuildInfo {
    struct SquareInfo {
      SquareType type;
      CostInfo cost;
      string name;
      bool buildImmediatly;
    } squareInfo;

    struct TrapInfo {
      TrapType type;
      string name;
      ViewId viewId;
    } trapInfo;

    enum BuildType { DIG, SQUARE, IMP, TRAP, GUARD_POST, DESTROY, IMPALED_HEAD, FETCH } buildType;

    Optional<TechId> techId;
    string help;
    char hotkey;
    string groupName;

    BuildInfo(SquareInfo info, Optional<TechId> techId = Nothing(), const string& h = "", char hotkey = 0,
        string group = "");
    BuildInfo(BuildType type, SquareInfo info, const string& h = "", char hotkey = 0);
    BuildInfo(TrapInfo info, Optional<TechId> techId = Nothing(), const string& h = "", char hotkey = 0);
    BuildInfo(DeityHabitat, CostInfo, const string& groupName, const string& h = "", char hotkey = 0);
    BuildInfo(BuildType type, const string& h = "", char hotkey = 0);
  };
  void handleSelection(Vec2 pos, const BuildInfo&, bool rectangle);
  vector<GameInfo::BandInfo::Button> fillButtons(const vector<BuildInfo>& buildInfo) const;
  static vector<BuildInfo> buildInfo;
  static void initBuildInfo();
  static vector<BuildInfo> workshopInfo;
  static vector<BuildInfo> libraryInfo;
  static vector<BuildInfo> minionsInfo;

  const static map<ResourceId, ResourceInfo> resourceInfo;

  ViewObject getResourceViewObject(ResourceId id) const;
  Optional<pair<ViewObject, int>> getCostObj(CostInfo) const;

  vector<Vec2> getAllSquares(const vector<SquareType>&) const;

  map<ResourceId, int> SERIAL(credit);

  struct ItemFetchInfo {
    ItemPredicate predicate;
    vector<SquareType> destination;
    bool oneAtATime;
    vector<SquareType> additionalPos;
    Warning warning;
  };

  vector<ItemFetchInfo> getFetchInfo() const;
  void fetchItems(Vec2 pos, ItemFetchInfo, bool ignoreDelayed = false);

  vector<Technology*> SERIAL(technologies);
  bool hasTech(TechId id) const;
  int getMinLibrarySize() const;
  void acquireTech(Technology*, bool free = false);
  double getTechCost();
  int SERIAL2(numFreeTech, 0);

  typedef GameInfo::BandInfo::TechButton TechButton;

  struct TechInfo {
    TechButton button;
    function<void(Collective*, View*)> butFun;
  };
  vector<TechInfo> getTechInfo() const;

  MoveInfo getBeastMove(Creature* c);
  MoveInfo getMinionMove(Creature* c);
  MoveInfo getGuardPostMove(Creature* c);
  MoveInfo getExecutionMove(Creature* c);
  MoveInfo getPossessedMove(Creature* c);
  MoveInfo getBacktrackMove(Creature* c);
  MoveInfo getAlarmMove(Creature* c);
  MoveInfo getDropItems(Creature *c);

  bool isDownstairsVisible() const;
  void delayDangerousTasks(const vector<Vec2>& enemyPos, double delayTime);
  bool isDelayed(Vec2 pos);
  double getTime() const;
  unordered_map<Vec2, double> SERIAL(delayedPos);
  int numResource(ResourceId) const;
  void takeResource(CostInfo);
  void returnResource(CostInfo);
  int getImpCost() const;
  bool canBuildDoor(Vec2 pos) const;
  bool canPlacePost(Vec2 pos) const;
  void freeFromGuardPost(const Creature*);
  void handleMarket(View*, int prevItem = 0);
  void getEquipmentItem(View* view, ItemPredicate predicate);
  vector<Item*> getAllItems(ItemPredicate predicate, bool includeMinions = true) const;
  Item* chooseEquipmentItem(View* view, vector<Item*> currentItems, ItemPredicate predicate,
      int* index = nullptr, double* scrollPos = nullptr) const;
  bool usesEquipment(const Creature* c) const;
  void autoEquipment(Creature* creature, bool replace);
  Item* getWorstItem(vector<Item*> items) const;
  MinionType getMinionType(const Creature*) const;
  void setMinionType(Creature*, MinionType type);

  void getMinionOptions(Creature*, vector<MinionOption>&, vector<View::ListElem>&);

  int getNumMinions() const;
  void minionView(View* view, Creature* creature, int prevItem = 0);
  void handleEquipment(View* view, Creature* creature, int prevItem = 0);
  void handleNecromancy(View*);
  void handleMatterAnimation(View*);
  void handleBeastTaming(View*);
  void handleHumanoidBreeding(View*);
  void handleSpawning(View* view, SquareType spawnSquare, const string& info1, 
      const string& info2, const string& title, MinionType minionType, vector<SpawnInfo> spawnInfo,
      Optional<vector<pair<Vec2, Item*>>> genItems = Nothing(), string genItemsInfo = "", string info3 = "");
  void handlePersonalSpells(View*);
  void handleLibrary(View*);
  void updateConstructions();
  static ViewObject getTrapObject(TrapType type);
  bool isInCombat(const Creature*) const;
  bool underAttack() const;
  void addToMemory(Vec2 pos);
  void updateMemory();
  bool isItemMarked(const Item*) const;
  void markItem(const Item*);
  void unmarkItem(UniqueId);
  bool tryLockingDoor(Vec2 pos);
  void addKnownTile(Vec2 pos);

  vector<pair<Item*, Vec2>> getTrapItems(TrapType, set<Vec2> = {}) const;
  ItemPredicate unMarkedItems(ItemType) const;
  MarkovChain<MinionTask> getTasksForMinion(Creature* c);
  vector<Creature*> SERIAL(creatures);
  vector<Creature*> SERIAL(minions);
  unordered_map<MinionType, vector<Creature*>> SERIAL(minionByType);
  EntitySet SERIAL(markedItems);

  class TaskMap : public Task::Mapping {
    public:
    Task* addTaskCost(PTask, CostInfo);
    void markSquare(Vec2 pos, PTask);
    void unmarkSquare(Vec2 pos);
    Task* getMarked(Vec2 pos) const;
    CostInfo removeTask(Task*);
    CostInfo removeTask(UniqueId);
    bool isLocked(const Creature*, const Task*) const;
    void lock(const Creature*, const Task*);
    void clearAllLocked();
    Task* getTaskForImp(Creature*);
    void freeTaskDelay(Task*, double delayTime);

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);

    SERIAL_CHECKER;

    private:
    map<Vec2, Task*> SERIAL(marked);
    map<Task*, CostInfo> SERIAL(completionCost);
    set<pair<const Creature*, UniqueId>> SERIAL(lockedTasks);
    map<UniqueId, double> SERIAL(delayedTasks);
  } SERIAL(taskMap);

  struct TrapInfo {
    TrapType type;
    bool armed;
    double marked;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };
  map<Vec2, TrapInfo> SERIAL(traps);
  set<TrapType> getNeededTraps() const;

  struct ConstructionInfo {
    CostInfo cost;
    bool built;
    double marked;
    SquareType type;
    UniqueId task;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };
  void setMinionTask(Creature* c, MinionTask task);
  MinionTask getMinionTask(Creature* c) const;
  map<Vec2, ConstructionInfo> SERIAL(constructions);
  map<UniqueId, MarkovChain<MinionTask>> SERIAL(minionTasks);
  map<UniqueId, string> SERIAL(minionTaskStrings);
  unordered_map<SquareType, set<Vec2>> SERIAL(mySquares);
  map<Vec2, int> SERIAL(squareEfficiency);
  void updateEfficiency(Vec2, SquareType);
  double getEfficiency(Vec2) const;
  bool hasEfficiency(Vec2) const;
  set<Vec2> SERIAL(myTiles);
  Level* SERIAL(level);
  Creature* SERIAL2(keeper, nullptr);
  mutable unique_ptr<map<Level*, MapMemory>> SERIAL(memory);
  Table<bool> SERIAL(knownTiles);
  set<Vec2> SERIAL(borderTiles);
  bool SERIAL2(gatheringTeam, false);
  vector<Creature*> SERIAL(team);
  map<const Level*, Vec2> SERIAL(teamLevelChanges);
  map<const Level*, Vec2> SERIAL(levelChangeHistory);
  Creature* SERIAL2(possessed, nullptr);
  MinionEquipment SERIAL(minionEquipment);
  struct GuardPostInfo {
    const Creature* attender;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };
  map<Vec2, GuardPostInfo> SERIAL(guardPosts);
  double SERIAL(mana);
  int SERIAL2(points, 0);
  Model* SERIAL(model);
  vector<const Creature*> SERIAL(kills);
  bool SERIAL2(showWelcomeMsg, true);
  Optional<Vec2> rectSelectCorner;
  Optional<Vec2> rectSelectCorner2;
  unordered_map<const Creature*, double> SERIAL(lastCombat);
  double SERIAL2(lastControlKeeperQuestion, -100);
  int SERIAL2(startImpNum, -1);
  bool SERIAL2(retired, false);
  Tribe* SERIAL2(tribe, nullptr);
  struct AlarmInfo {
    double finishTime = -1000;
    Vec2 position;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  } SERIAL(alarmInfo);
  struct PrisonerInfo {
    enum State { SURRENDER, PRISON, EXECUTE } state;
    const Creature* attender;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };
  map<Creature*, PrisonerInfo> SERIAL(prisonerInfo);
  int SERIAL2(executions, 0);
  unique_ptr<Sectors> SERIAL(sectors);
  unique_ptr<Sectors> SERIAL(flyingSectors);
  unordered_set<Vec2> SERIAL(surprises);
};

#endif
