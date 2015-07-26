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

#include "move_info.h"
#include "task_callback.h"
#include "minion_task.h"
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
struct CostInfo;

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

class Collective : public TaskCallback {
  public:
  void addCreature(Creature*, EnumSet<MinionTrait>);
  void addCreature(PCreature, Position, EnumSet<MinionTrait>);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void tick(double time);
  const Tribe* getTribe() const;
  Tribe* getTribe();
  double getStanding(const Deity*) const;
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
  double getBeastMultiplier() const;
  double getUndeadMultiplier() const;

  vector<Creature*> getRecruits() const;
  void recruit(Creature*, Collective* to);

  double getEfficiency(const Creature*) const;
  const Creature* getLeader() const;
  Creature* getLeader();

  const set<Position>& getSquares(SquareType) const;
  const set<Position>& getSquares(SquareApplyType) const;
  vector<SquareType> getSquareTypes() const;
  vector<Position> getAllSquares(const vector<SquareType>&, bool centerOnly = false) const;
  const set<Position>& getAllSquares() const;
  void claimSquare(Position);
  void changeSquareType(Position pos, SquareType from, SquareType to);
  bool containsSquare(Position pos) const;
  bool isKnownSquare(Position pos) const;

  double getEfficiency(Position) const;
  bool hasEfficiency(Position) const;

  bool usesEquipment(const Creature* c) const;

  ~Collective();

  void setWarning(Warning, bool state = true);
  bool isWarning(Warning) const;

  struct ResourceInfo;
  struct MinionTaskInfo;

  map<MinionTask, MinionTaskInfo> getTaskInfo() const;

  struct ResourceInfo {
    vector<SquareType> storageType;
    optional<ItemIndex> itemIndex;
    ItemId itemId;
    string name;
    bool dontDisplay;
  };

  const static map<ResourceId, ResourceInfo> resourceInfo;

  int numResource(ResourceId) const;
  int numResourcePlusDebt(ResourceId) const;
  bool hasResource(const CostInfo&) const;
  void takeResource(const CostInfo&);
  void returnResource(const CostInfo&);

  struct ItemFetchInfo;

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
  vector<pair<Item*, Position>> getTrapItems(TrapType, set<Position> = set<Position>()) const;

  void orderExecution(Creature*);
  void orderSacrifice(Creature*);
  void orderTorture(Creature*);
  void orderWhipping(Creature*);
  bool canWhip(Creature*) const;

  const map<UniqueEntity<Creature>::Id, string>& getMinionTaskStrings() const;

  void addTrap(Position, TrapType);
  void removeTrap(Position);
  void addConstruction(Position, SquareType, const CostInfo&, bool immediately, bool noCredit);
  void removeConstruction(Position);
  void destroySquare(Position);
  bool isPlannedTorch(Position) const;
  bool canPlaceTorch(Position) const;
  void removeTorch(Position);
  void addTorch(Position);
  void fetchAllItems(Position);
  void dig(Position);
  void cancelMarkedTask(Position);
  void cutTree(Position);
  double getDangerLevel() const;
  bool isMarked(Position) const;
  HighlightType getMarkHighlight(Position) const;
  void setPriorityTasks(Position);
  bool hasPriorityTasks(Position) const;

  bool hasTech(TechId id) const;
  void acquireTech(Technology*, bool free = false);
  vector<Technology*> getTechnologies() const;
  double getTechCost(Technology*);
  static vector<Spell*> getSpellLearning(const Technology*);
  vector<Spell*> getAllSpells() const;
  vector<Spell*> getAvailableSpells() const;
  TechId getNeededTech(Spell*) const;
  void addKnownTile(Position);

  vector<const Creature*> getKills() const;
  int getPoints() const;

  MinionEquipment& getMinionEquipment();
  const MinionEquipment& getMinionEquipment() const;

  vector<Position> getExtendedTiles(int maxRadius, int minRadius = 0) const;

  struct DormInfo;
  static const EnumMap<SpawnType, DormInfo>& getDormInfo();
  static optional<SquareType> getSecondarySquare(SquareType);

  struct MinionPaymentInfo;

  int getNextPayoutTime() const;
  int getSalary(const Creature*) const;
  int getNextSalaries() const;
  bool hasMinionDebt() const;

  int getPopulationSize() const;
  int getMaxPopulation() const;

  bool tryLockingDoor(Position);
  void orderConsumption(Creature* consumer, Creature* who);
  vector<Creature*>getConsumptionTargets(Creature* consumer);

  void addAssaultNotification(const Collective*, const vector<Creature*>&, const string& message);
  void removeAssaultNotification(const Collective*);

  void onKilled(Creature* victim, Creature* killer);
  void onAlarm(Position);
  void onTorture(const Creature* who, const Creature* torturer);
  void onSurrender(Creature* who);
  void onTrapTrigger(Position);
  void onTrapDisarm(const Creature*, Position);
  void onSquareDestroyed(Position);
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
  virtual void onAppliedItem(Position, Item* item) override;
  virtual void onAppliedItemCancel(Position) override;
  virtual void onPickedUp(Position, EntitySet<Item>) override;
  virtual void onCantPickItem(EntitySet<Item> items) override;
  virtual void onConstructed(Position, const SquareType&) override;
  virtual void onTorchBuilt(Position, Trigger*) override;
  virtual void onAppliedSquare(Position) override;
  virtual void onKillCancelled(Creature*) override;
  virtual void onBedCreated(Position, const SquareType& fromType, const SquareType& toType) override;
  virtual void onCopulated(Creature* who, Creature* with) override;
  virtual void onConsumed(Creature* consumer, Creature* who) override;
  virtual bool isConstructionReachable(Position) override;
  virtual void onWhippingDone(Creature* whipped, Position postPosition) override;

  private:
  friend class CollectiveBuilder;
  Collective(Level*, const CollectiveConfig&, Tribe*, EnumMap<ResourceId, int> credit, const string& name);
  void updateEfficiency(Position, SquareType);
  int getPaymentAmount(const Creature*) const;
  void makePayouts();
  void cashPayouts();
  void removeCreature(Creature*);

  const vector<ItemFetchInfo>& getFetchInfo() const;
  void fetchItems(Position, const ItemFetchInfo&);

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

  struct CurrentTaskInfo;
  map<UniqueEntity<Creature>::Id, CurrentTaskInfo> SERIAL(currentTasks);
  optional<Position> getTileToExplore(const Creature*, MinionTask) const;
  PTask getStandardTask(Creature* c);
  PTask getEquipmentTask(Creature* c);
  PTask getHealingTask(Creature* c);
  bool isTaskGood(const Creature*, MinionTask, bool ignoreTaskLock = false) const;
  PTask generateMinionTask(Creature*, MinionTask);
  void setRandomTask(const Creature*);

  void handleSurprise(Position);
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
  void considerBuildingBeds();
  bool considerNonSpawnImmigrant(const ImmigrantInfo&, vector<PCreature>);
  vector<Position> getSpawnPos(const vector<Creature*>&);
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
  unordered_map<SquareType, set<Position>> SERIAL(mySquares);
  unordered_map<SquareApplyType, set<Position>> SERIAL(mySquares2);
  map<Position, int> SERIAL(squareEfficiency);
  set<Position> SERIAL(allSquares);
  struct AlarmInfo : NamedTupleBase<double, Position> {
    NAMED_TUPLE_STUFF(AlarmInfo);
    NAME_ELEM(0, finishTime);
    NAME_ELEM(1, position);
  };
  optional<AlarmInfo> SERIAL(alarmInfo);
  MoveInfo getAlarmMove(Creature* c);
  HeapAllocated<ConstructionMap> SERIAL(constructions);
  EntitySet<Item> SERIAL(markedItems);
  set<Position> SERIAL(whippingPostsInUse);
  ItemPredicate unMarkedItems() const;
  enum class PrisonerState { SURRENDER, PRISON, EXECUTE, TORTURE, SACRIFICE };
  struct PrisonerInfo;
  PTask getPrisonerTask(Creature* prisoner);
  void clearPrisonerTask(Creature* prisoner);
  map<Creature*, PrisonerInfo> SERIAL(prisonerInfo);
  void updateConstructions();
  void delayDangerousTasks(const vector<Position>& enemyPos, double delayTime);
  bool isDelayed(Position);
  unordered_map<Position, double> SERIAL(delayedPos);
  vector<Position> getEnemyPositions() const;
  double manaRemainder = 0;
  double getKillManaScore(const Creature*) const;
  void addMana(double);
  vector<const Creature*> SERIAL(kills);
  int SERIAL(points) = 0;
  map<const Creature*, MinionPaymentInfo> SERIAL(minionPayment);
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
