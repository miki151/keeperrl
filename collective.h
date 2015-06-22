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

#include "monster_ai.h"
#include "task.h"
#include "minion_task.h"
#include "cost_info.h"
#include "square_apply_type.h"
#include "resource_id.h"
#include "square_type.h"

class Creature;
class CollectiveControl;
class Tribe;
class Deity;
class Level;
class Trigger;
struct ImmigrantInfo;
struct AttractionInfo;
class Spell;
class MinionEquipment;
class TaskMap;
class KnownTiles;
class CollectiveTeams;
class ConstructionMap;
class Technology;
class CollectiveConfig;
class MinionAttraction;

RICH_ENUM(CollectiveWarning,
    DIGGING,
    RESOURCE_STORAGE,
    EQUIPMENT_STORAGE,
    LIBRARY,
    BEDS,
    TRAINING,
    NO_HATCHERY,
    WORKSHOP,
    NO_WEAPONS,
    LOW_MORALE,
    GRAVES,
    CHESTS,
    NO_PRISON,
    LARGER_PRISON,
    TORTURE_ROOM,
//    ALTAR,
    MORE_CHESTS,
    MANA,
    MORE_LIGHTS
);

enum class AttackPrerequisite {
  THRONE
};

class Collective : public Task::Callback {
  public:
  void addCreature(Creature*, EnumSet<MinionTrait>);
  void addCreature(PCreature, Vec2, EnumSet<MinionTrait>);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void tick(double time);
  const Tribe* getTribe() const;
  Tribe* getTribe();
  double getStanding(const Deity*) const;
  double getWarLevel() const;
  Level* getLevel();
  const Level* getLevel() const;
  double getTime() const;
  void update(Creature*);
  void addNewCreatureMessage(const vector<Creature*>&);
  void setTask(const Creature*, PTask, bool priority = false);
  bool hasTask(const Creature*) const;
  void cancelTask(const Creature*);
  void banishCreature(Creature*);
  bool wasBanished(const Creature*) const;
  bool meetsPrerequisites(const vector<AttackPrerequisite>&) const;

  typedef CollectiveWarning Warning;
  typedef CollectiveResourceId ResourceId;

  SERIALIZATION_DECL(Collective);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;
  bool isConquered() const;

  const vector<Creature*>& getCreatures(SpawnType) const;
  const vector<Creature*>& getCreatures(MinionTrait) const;
  vector<Creature*> getCreaturesAnyOf(EnumSet<MinionTrait>) const;
  vector<Creature*> getCreatures(EnumSet<MinionTrait> with, EnumSet<MinionTrait> without = {}) const;
  bool hasTrait(const Creature*, MinionTrait) const;
  bool hasAnyTrait(const Creature*, EnumSet<MinionTrait>) const;
  void setTrait(Creature* c, MinionTrait);
  void removeTrait(Creature* c, MinionTrait);

  double getTechCostMultiplier() const;
  double getCraftingMultiplier() const;
  double getWarMultiplier() const;
  double getBeastMultiplier() const;
  double getUndeadMultiplier() const;

  double getEfficiency(const Creature*) const;
  const Creature* getLeader() const;
  Creature* getLeader();

  const set<Vec2>& getSquares(SquareType) const;
  const set<Vec2>& getSquares(SquareApplyType) const;
  vector<SquareType> getSquareTypes() const;
  vector<Vec2> getAllSquares(const vector<SquareType>&, bool centerOnly = false) const;
  const set<Vec2>& getAllSquares() const;
  void claimSquare(Vec2);
  void changeSquareType(Vec2 pos, SquareType from, SquareType to);
  bool containsSquare(Vec2 pos) const;
  bool isKnownSquare(Vec2 pos) const;
  bool underAttack() const;

  double getEfficiency(Vec2) const;
  bool hasEfficiency(Vec2) const;

  bool isGuardPost(Vec2) const;
  void addGuardPost(Vec2);
  void removeGuardPost(Vec2);
  void freeFromGuardPost(const Creature*);
  bool usesEquipment(const Creature* c) const;

  ~Collective();

  void setWarning(Warning, bool state = true);
  bool isWarning(Warning) const;

  struct ResourceInfo {
    vector<SquareType> storageType;
    optional<ItemIndex> itemIndex;
    ItemId itemId;
    string name;
    bool dontDisplay;
  };

  struct MinionTaskInfo {
    enum Type { APPLY_SQUARE, EXPLORE, COPULATE, CONSUME, EAT } type;
    MinionTaskInfo(vector<SquareType>, const string& description, optional<Warning> = none, double cost = 0,
        bool centerOnly = false);
    MinionTaskInfo(Type, const string& description, optional<Warning> = none);
    vector<SquareType> squares;
    string description;
    optional<Warning> warning;
    double cost = 0;
    bool centerOnly = false;
  };

  map<MinionTask, MinionTaskInfo> getTaskInfo() const;

  const static map<ResourceId, ResourceInfo> resourceInfo;


  int numResource(ResourceId) const;
  int numResourcePlusDebt(ResourceId) const;
  bool hasResource(CostInfo) const;
  void takeResource(CostInfo);
  void returnResource(CostInfo);

  struct ItemFetchInfo {
    ItemIndex index;
    ItemPredicate predicate;
    vector<SquareType> destination;
    bool oneAtATime;
    vector<SquareType> additionalPos;
    Warning warning;
  };

  const ConstructionMap& getConstructions() const;

  void setMinionTask(const Creature* c, MinionTask task);
  optional<MinionTask> getMinionTask(const Creature*) const;
  bool isMinionTaskPossible(Creature* c, MinionTask task);

  set<TrapType> getNeededTraps() const;

  vector<Item*> getAllItems(bool includeMinions = true) const;
  vector<Item*> getAllItems(ItemPredicate predicate, bool includeMinions = true) const;
  vector<Item*> getAllItems(ItemIndex, bool includeMinions = true) const;
  static void sortByEquipmentValue(vector<Item*>&);
  static SquareType getHatcheryType(Tribe* tribe);

  static vector<SquareType> getEquipmentStorageSquares();
  vector<pair<Item*, Vec2>> getTrapItems(TrapType, set<Vec2> = set<Vec2>()) const;

  void orderExecution(Creature*);
  void orderSacrifice(Creature*);
  void orderTorture(Creature*);

  const map<UniqueEntity<Creature>::Id, string>& getMinionTaskStrings() const;

  void addTrap(Vec2, TrapType);
  void removeTrap(Vec2);
  void addConstruction(Vec2, SquareType, CostInfo, bool immediately, bool noCredit);
  void removeConstruction(Vec2);
  void destroySquare(Vec2);
  bool isPlannedTorch(Vec2) const;
  bool canPlaceTorch(Vec2) const;
  void removeTorch(Vec2);
  void addTorch(Vec2);
  void fetchAllItems(Vec2);
  void dig(Vec2);
  void cancelMarkedTask(Vec2);
  void cutTree(Vec2);
  double getDangerLevel(bool includeExecutions = true) const;
  bool isMarked(Vec2) const;
  HighlightType getMarkHighlight(Vec2) const;
  void setPriorityTasks(Vec2);
  bool hasPriorityTasks(Vec2) const;

  bool hasTech(TechId id) const;
  void acquireTech(Technology*, bool free = false);
  vector<Technology*> getTechnologies() const;
  double getTechCost(Technology*);
  static vector<Spell*> getSpellLearning(const Technology*);
  vector<Spell*> getAllSpells() const;
  vector<Spell*> getAvailableSpells() const;
  TechId getNeededTech(Spell*) const;
  void addKnownTile(Vec2 pos);

  vector<const Creature*> getKills() const;
  int getPoints() const;

  MinionEquipment& getMinionEquipment();
  const MinionEquipment& getMinionEquipment() const;

  vector<Vec2> getExtendedTiles(int maxRadius, int minRadius = 0) const;

  struct DormInfo;
  static const EnumMap<SpawnType, DormInfo>& getDormInfo();
  static optional<SquareType> getSecondarySquare(SquareType);
  static optional<Vec2> chooseBedPos(const set<Vec2>& lair, const set<Vec2>& beds);

  struct MinionPaymentInfo : public NamedTupleBase<int, double, int> {
    NAMED_TUPLE_STUFF(MinionPaymentInfo);
    NAME_ELEM(0, salary);
    NAME_ELEM(1, workAmount);
    NAME_ELEM(2, debt);
  };

  int getNextPayoutTime() const;
  int getSalary(const Creature*) const;
  int getNextSalaries() const;
  bool hasMinionDebt() const;

  int getPopulationSize() const;
  int getMaxPopulation() const;

  bool tryLockingDoor(Vec2);
  void orderConsumption(Creature* consumer, Creature* who);
  vector<Creature*>getConsumptionTargets(Creature* consumer);

  void addAssaultNotification(const Collective*, const vector<Creature*>&, const string& message);
  void removeAssaultNotification(const Collective*);

  void onKilled(Creature* victim, Creature* killer);
  void onAlarm(Vec2);
  void onTorture(const Creature* who, const Creature* torturer);
  void onSurrender(Creature* who);
  void onTrapTrigger(Vec2 pos);
  void onTrapDisarm(const Creature*, Vec2 pos);
  void onSquareDestroyed(Vec2 pos);
  void onEquip(const Creature*, const Item*);

  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  void freeTeamMembers(TeamId);
  void ownItems(const Creature* who, const vector<Item*>);

  const string& getName() const;

  const TaskMap& getTaskMap() const;

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  protected:
  // From Task::Callback
  virtual void onAppliedItem(Vec2 pos, Item* item) override;
  virtual void onAppliedItemCancel(Vec2 pos) override;
  virtual void onPickedUp(Vec2 pos, EntitySet<Item>) override;
  virtual void onCantPickItem(EntitySet<Item> items) override;
  virtual void onConstructed(Vec2, const SquareType&) override;
  virtual void onConstructionCancelled(Vec2) override;
  virtual void onTorchBuilt(Vec2, Trigger*) override;
  virtual void onAppliedSquare(Vec2 pos) override;
  virtual void onKillCancelled(Creature*) override;
  virtual void onBedCreated(Vec2, const SquareType& fromType, const SquareType& toType) override;
  virtual void onCopulated(Creature* who, Creature* with) override;
  virtual void onConsumed(Creature* consumer, Creature* who) override;
  virtual bool isConstructionReachable(Vec2) override;

  private:
  friend class CollectiveBuilder;
  Collective(Level*, const CollectiveConfig&, Tribe*, EnumMap<ResourceId, int> credit, const string& name);
  void updateEfficiency(Vec2, SquareType);
  int getPaymentAmount(const Creature*) const;
  void makePayouts();
  void cashPayouts();
  void removeCreature(Creature*);

  const vector<ItemFetchInfo>& getFetchInfo() const;
  void fetchItems(Vec2 pos, const ItemFetchInfo&);

  void addMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForBanishing(const Creature*);

  double getAttractionValue(const MinionAttraction&);
  double getImmigrantChance(const ImmigrantInfo&);

  bool isItemNeeded(const Item*) const;
  void addProducesMessage(const Creature*, const vector<PItem>&);
  
  HeapAllocated<MinionEquipment> SERIAL(minionEquipment);
  EnumMap<ResourceId, int> SERIAL(credit);
  HeapAllocated<TaskMap> SERIAL(taskMap);
  vector<TechId> SERIAL(technologies);
  int SERIAL(numFreeTech) = 0;
  bool isItemMarked(const Item*) const;
  void markItem(const Item*);
  void unmarkItem(UniqueEntity<Item>::Id);

  HeapAllocated<KnownTiles> SERIAL(knownTiles);

  struct CurrentTaskInfo : NamedTupleBase<MinionTask, double> {
    NAMED_TUPLE_STUFF(CurrentTaskInfo);
    NAME_ELEM(0, task);
    NAME_ELEM(1, finishTime);
  };
  map<UniqueEntity<Creature>::Id, CurrentTaskInfo> SERIAL(currentTasks);
  optional<Vec2> getTileToExplore(const Creature*, MinionTask) const;
  PTask getStandardTask(Creature* c);
  PTask getEquipmentTask(Creature* c);
  PTask getHealingTask(Creature* c);
  bool isTaskGood(const Creature*, MinionTask) const;
  PTask generateMinionTask(Creature*, MinionTask);
  void setRandomTask(const Creature*);

  void handleSurprise(Vec2 pos);
  EnumSet<Warning> SERIAL(warnings);
  MoveInfo getDropItems(Creature*);
  MoveInfo getWorkerMove(Creature*);
  MoveInfo getTeamMemberMove(Creature*);
  void autoEquipment(Creature* creature, bool replace);
  Item* getWorstItem(const Creature*, vector<Item*> items) const;
  int getTaskDuration(const Creature*, MinionTask) const;
  map<UniqueEntity<Creature>::Id, string> SERIAL(minionTaskStrings);
  double getStanding(EpithetId id) const;
  void onEpithetWorship(Creature*, WorshipType, EpithetId);
  void considerHealingLeader();
  bool considerImmigrant(const ImmigrantInfo&);
  bool considerNonSpawnImmigrant(const ImmigrantInfo&, vector<PCreature>);
  vector<Vec2> getSpawnPos(const vector<Creature*>&);
  void considerImmigration();
  int tryBuildingBeds(SpawnType spawnType, int numBeds);
  void considerBirths();
  void considerWeaponWarning();
  void considerMoraleWarning();
  void decayMorale();
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  EnumMap<SpawnType, vector<Creature*>> SERIAL(bySpawnType);
  PCollectiveControl SERIAL(control);
  Tribe* SERIAL(tribe) = nullptr;
  map<const Deity*, double> SERIAL(deityStanding);
  Level* SERIAL(level) = nullptr;
  unordered_map<SquareType, set<Vec2>> SERIAL(mySquares);
  unordered_map<SquareApplyType, set<Vec2>> SERIAL(mySquares2);
  map<Vec2, int> SERIAL(squareEfficiency);
  set<Vec2> SERIAL(allSquares);
  struct AlarmInfo : NamedTupleBase<double, Vec2> {
    NAMED_TUPLE_STUFF(AlarmInfo);
    AlarmInfo() { finishTime() = -1000; }
    NAME_ELEM(0, finishTime);
    NAME_ELEM(1, position);
  } SERIAL(alarmInfo);
  MoveInfo getAlarmMove(Creature* c);
  struct GuardPostInfo : public NamedTupleBase<const Creature*> {
    NAME_ELEM(0, attender);
  };
  map<Vec2, GuardPostInfo> SERIAL(guardPosts);
  MoveInfo getGuardPostMove(Creature* c);
  HeapAllocated<ConstructionMap> SERIAL(constructions);
  EntitySet<Item> SERIAL(markedItems);
  ItemPredicate unMarkedItems() const;
  enum class PrisonerState { SURRENDER, PRISON, EXECUTE, TORTURE, SACRIFICE };
  struct PrisonerInfo : public NamedTupleBase<PrisonerState, UniqueEntity<Task>::Id> {
    NAMED_TUPLE_STUFF(PrisonerInfo);
    NAME_ELEM(0, state);
    NAME_ELEM(1, task);
  };
  PTask getPrisonerTask(Creature* prisoner);
  void clearPrisonerTask(Creature* prisoner);
  map<Creature*, PrisonerInfo> SERIAL(prisonerInfo);
  void updateConstructions();
  void delayDangerousTasks(const vector<Vec2>& enemyPos, double delayTime);
  bool isDelayed(Vec2 pos);
  unordered_map<Vec2, double> SERIAL(delayedPos);
  vector<Vec2> getEnemyPositions() const;
  double manaRemainder = 0;
  double getKillManaScore(const Creature*) const;
  void addMana(double);
  vector<const Creature*> SERIAL(kills);
  int SERIAL(points) = 0;
  unordered_map<const Creature*, MinionPaymentInfo> SERIAL(minionPayment);
  int SERIAL(nextPayoutTime);
  unordered_map<const Creature*, vector<AttractionInfo>> SERIAL(minionAttraction);
  double getAttractionOccupation(const MinionAttraction&);
  Creature* getCopulationTarget(Creature* succubus);
  Creature* getConsumptionTarget(Creature* consumer);
  deque<Creature*> SERIAL(pregnancies);
  mutable vector<ItemFetchInfo> itemFetchInfo;
  HeapAllocated<CollectiveTeams> SERIAL(teams);
  set<const Location*> SERIAL(knownLocations);
  string SERIAL(name);
  HeapAllocated<CollectiveConfig> SERIAL(config);
  vector<const Creature*> SERIAL(banished);
};

#endif
