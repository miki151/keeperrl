#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "monster_ai.h"
#include "task.h"
#include "event.h"
#include "minion_equipment.h"
#include "spell_info.h"
#include "task_map.h"
#include "minion_attraction.h"
#include "sectors.h"
#include "minion_task.h"
#include "gender.h"
#include "item.h"

class Creature;
class CollectiveControl;
class Tribe;
class Deity;
class Level;
class Trigger;

RICH_ENUM(SpawnType,
  HUMANOID,
  UNDEAD,
  BEAST,
  DEMON
);

struct CollectiveConfig;

RICH_ENUM(CollectiveConfigId,
  KEEPER,
  VILLAGE
);

RICH_ENUM(CollectiveWarning,
    DIGGING,
    STORAGE,
    LIBRARY,
    BEDS,
    TRAINING,
    WORKSHOP,
    NO_WEAPONS,
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

class Collective : public Task::Callback {
  public:
  Collective(Level*, CollectiveConfigId, Tribe*);
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

  typedef CollectiveWarning Warning;
  typedef CollectiveResourceId ResourceId;
  virtual void onAppliedItem(Vec2 pos, Item* item) override;
  virtual void onAppliedItemCancel(Vec2 pos) override;
  virtual void onPickedUp(Vec2 pos, EntitySet<Item>) override;
  virtual void onCantPickItem(EntitySet<Item> items) override;
  virtual void onConstructed(Vec2, SquareType) override;
  virtual void onTorchBuilt(Vec2, Trigger*) override;
  virtual void onAppliedSquare(Vec2 pos) override;
  virtual void onKillCancelled(Creature*) override;
  virtual void onBedCreated(Vec2, SquareType fromType, SquareType toType) override;
  virtual void onCopulated(Creature* who, Creature* with);
  virtual void onConsumed(Creature* consumer, Creature* who);

  SERIALIZATION_DECL(Collective);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;

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

  ~Collective();

  void setWarning(Warning, bool state = true);
  bool isWarning(Warning) const;

  struct CostInfo : public NamedTupleBase<ResourceId, int> {
    NAMED_TUPLE_STUFF(CostInfo);
    NAME_ELEM(0, id);
    NAME_ELEM(1, value);
  };

  struct ResourceInfo {
    vector<SquareType> storageType;
    ItemPredicate predicate;
    ItemId itemId;
    string name;
    bool dontDisplay;
  };

  struct MinionTaskInfo {
    enum Type { APPLY_SQUARE, EXPLORE, COPULATE, CONSUME } type;
    MinionTaskInfo(vector<SquareType>, const string& description, Optional<Warning> = Nothing(), double cost = 0,
        bool centerOnly = false);
    MinionTaskInfo(Type, const string&);
    vector<SquareType> squares;
    string description;
    Optional<Warning> warning;
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
    ItemPredicate predicate;
    vector<SquareType> destination;
    bool oneAtATime;
    vector<SquareType> additionalPos;
    Warning warning;
  };

  const vector<ItemFetchInfo>& getFetchInfo() const;
  void fetchItems(Vec2 pos, const ItemFetchInfo&, bool ignoreDelayed = false);

  struct ConstructionInfo : public NamedTupleBase<CostInfo, bool, double, SquareType, UniqueEntity<Task>::Id> {
    NAMED_TUPLE_STUFF(ConstructionInfo);
    NAME_ELEM(0, cost);
    NAME_ELEM(1, built);
    NAME_ELEM(2, marked);
    NAME_ELEM(3, type);
    NAME_ELEM(4, task);
  };
  const map<Vec2, ConstructionInfo>& getConstructions() const;

  void setMinionTask(Creature* c, MinionTask task);

  set<TrapType> getNeededTraps() const;

  vector<Item*> getAllItems(ItemPredicate predicate, bool includeMinions = true) const;

  static vector<SquareType> getEquipmentStorageSquares();
  vector<pair<Item*, Vec2>> getTrapItems(TrapType, set<Vec2> = set<Vec2>()) const;

  void orderExecution(Creature*);
  void orderSacrifice(Creature*);
  void orderTorture(Creature*);

  const map<UniqueEntity<Creature>::Id, string>& getMinionTaskStrings() const;

  struct TrapInfo : public NamedTupleBase<TrapType, bool, double> {
    NAMED_TUPLE_STUFF(TrapInfo);
    NAME_ELEM(0, type);
    NAME_ELEM(1, armed);
    NAME_ELEM(2, marked);
  };
  const map<Vec2, TrapInfo>& getTraps() const;
  void addTrap(Vec2, TrapType);
  void removeTrap(Vec2);
  void addConstruction(Vec2, SquareType, CostInfo, bool immediately, bool noCredit);
  void removeConstruction(Vec2);
  void destroySquare(Vec2);
  struct TorchInfo : public NamedTupleBase<bool, double, UniqueEntity<Task>::Id, Dir, Trigger*> {
    NAMED_TUPLE_STUFF(TorchInfo);
    NAME_ELEM(0, built);
    NAME_ELEM(1, marked);
    NAME_ELEM(2, task);
    NAME_ELEM(3, attachmentDir);
    NAME_ELEM(4, trigger);
  };
  const map<Vec2, TorchInfo>& getTorches() const;
  bool isPlannedTorch(Vec2) const;
  bool canPlaceTorch(Vec2) const;
  void removeTorch(Vec2);
  void addTorch(Vec2);
  void fetchAllItems(Vec2);
  void dig(Vec2);
  void dontDig(Vec2);
  void cutTree(Vec2);
  double getDangerLevel(bool includeExecutions = true) const;
  bool isMarkedToDig(Vec2) const;
  void setPriorityTasks(Vec2);

  bool hasTech(TechId id) const;
  void acquireTech(Technology*, bool free = false);
  const vector<Technology*>& getTechnologies() const;
  double getTechCost(Technology*);
  static vector<SpellInfo> getSpellLearning(const Technology*);
  vector<SpellId> getAllSpells() const;
  vector<SpellId> getAvailableSpells() const;
  TechId getNeededTech(SpellId) const;
  bool isInCombat(const Creature*) const;
  void addKnownTile(Vec2 pos);

  vector<const Creature*> getKills() const;
  int getPoints() const;

  MinionEquipment& getMinionEquipment();
  const MinionEquipment& getMinionEquipment() const;

  vector<Vec2> getExtendedTiles(int maxRadius, int minRadius = 0) const;

  struct ImmigrantInfo;

  struct DormInfo;
  static const EnumMap<SpawnType, DormInfo>& getDormInfo();
  static Optional<SquareType> getSecondarySquare(SquareType);
  static Optional<Vec2> chooseBedPos(const set<Vec2>& lair, const set<Vec2>& beds);

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

  void updateSectors(Vec2);
  void orderConsumption(Creature* consumer, Creature* who);
  vector<Creature*>getConsumptionTargets(Creature* consumer);

  void addAssaultNotification(const Creature*, const VillageControl*);
  void removeAssaultNotification(const Creature*, const VillageControl*);

  bool isInTeam(TeamId, const Creature*) const;
  bool isTeamActive(TeamId) const;
  void addToTeam(TeamId, Creature*);
  void removeFromTeam(TeamId, Creature*);
  void setTeamLeader(TeamId, Creature*);
  void activateTeam(TeamId);
  void deactivateTeam(TeamId);
  TeamId createTeam();
  void removeFromAllTeams(Creature*);
  const Creature* getTeamLeader(TeamId) const;
  Creature* getTeamLeader(TeamId);
  const vector<Creature*>& getTeam(TeamId) const;
  vector<TeamId> getTeams(const Creature*) const;
  vector<TeamId> getTeams() const;
  vector<TeamId> getActiveTeams(const Creature*) const;
  vector<TeamId> getActiveTeams() const;
  void cancelTeam(TeamId);
  bool teamExists(TeamId) const;

  template <class Archive>
  static void registerTypes(Archive& ar);

  private:
  void updateEfficiency(Vec2, SquareType);
  int getPaymentAmount(const Creature*) const;
  void makePayouts();
  void cashPayouts();

  void addMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForKill(const Creature* killer, const Creature* victim);

  double getAttractionValue(MinionAttraction);
  double getImmigrantChance(const ImmigrantInfo&);

  bool isItemNeeded(const Item*) const;
  void addProducesMessage(const Creature*, const vector<PItem>&);

  SERIAL_CHECKER;
  REGISTER_HANDLER(CombatEvent, const Creature*);
  REGISTER_HANDLER(KillEvent, const Creature* victim, const Creature* killer);
  REGISTER_HANDLER(WorshipEvent, Creature* who, const Deity*, WorshipType);
  REGISTER_HANDLER(AttackEvent, Creature* victim, Creature* attacker);
  REGISTER_HANDLER(SquareReplacedEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(AlarmEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(SurrenderEvent, Creature* who, const Creature* to);
  REGISTER_HANDLER(TrapTriggerEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(TrapDisarmEvent, const Level*, const Creature*, Vec2 pos);
  REGISTER_HANDLER(EquipEvent, const Creature*, const Item*);
  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items);
  REGISTER_HANDLER(TortureEvent, Creature* who, const Creature* torturer);

  CollectiveConfigId SERIAL(configId);
  const CollectiveConfig& getConfig() const;
  MinionEquipment SERIAL(minionEquipment);
  EnumMap<ResourceId, int> SERIAL(credit);
  TaskMap<CostInfo> SERIAL(taskMap);
  vector<Technology*> SERIAL(technologies);
  int SERIAL2(numFreeTech, 0);
  bool isItemMarked(const Item*) const;
  void markItem(const Item*);
  void unmarkItem(UniqueEntity<Item>::Id);

  Table<bool> SERIAL(knownTiles);
  set<Vec2> SERIAL(borderTiles);

  struct CurrentTaskInfo : NamedTupleBase<MinionTask, double> {
    NAMED_TUPLE_STUFF(CurrentTaskInfo);
    NAME_ELEM(0, task);
    NAME_ELEM(1, finishTime);
  };
  map<UniqueEntity<Creature>::Id, CurrentTaskInfo> SERIAL(currentTasks);
  PTask getStandardTask(Creature* c);
  PTask getEquipmentTask(Creature* c);
  PTask getHealingTask(Creature* c);
  MinionTask chooseRandomFreeTask(const Creature*);

  void handleSurprise(Vec2 pos);
  EnumSet<Warning> warnings;
  MoveInfo getDropItems(Creature*);
  MoveInfo getWorkerMove(Creature*);
  MoveInfo getTeamMemberMove(Creature*);
  bool usesEquipment(const Creature* c) const;
  void autoEquipment(Creature* creature, bool replace);
  Item* getWorstItem(vector<Item*> items) const;
  int getTaskDuration(Creature*, MinionTask) const;
  map<UniqueEntity<Creature>::Id, string> SERIAL(minionTaskStrings);
  double getStanding(EpithetId id) const;
  void onEpithetWorship(Creature*, WorshipType, EpithetId);
  void considerHealingLeader();
  bool considerImmigrant(const ImmigrantInfo&);
  void considerImmigration();
  void considerBirths();
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  EnumMap<SpawnType, vector<Creature*>> SERIAL(bySpawnType);
  PCollectiveControl SERIAL(control);
  Tribe* SERIAL2(tribe, nullptr);
  map<const Deity*, double> SERIAL(deityStanding);
  Level* SERIAL2(level, nullptr);
  unordered_map<SquareType, set<Vec2>> SERIAL(mySquares);
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
  map<Vec2, ConstructionInfo> SERIAL(constructions);
  EntitySet<Item> SERIAL(markedItems);
  ItemPredicate unMarkedItems(ItemClass) const;
  map<Vec2, TrapInfo> SERIAL(traps);
  map<Vec2, TorchInfo> SERIAL(torches);
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
  unordered_map<const Creature*, double> SERIAL(lastCombat);
  vector<const Creature*> SERIAL(kills);
  int SERIAL2(points, 0);
  unordered_map<const Creature*, MinionPaymentInfo> SERIAL(minionPayment);
  int SERIAL(nextPayoutTime);
  struct AttractionInfo;
  unordered_map<const Creature*, vector<AttractionInfo>> SERIAL(minionAttraction);
  double getAttractionOccupation(MinionAttraction);
  unique_ptr<Sectors> SERIAL(sectors);
  unique_ptr<Sectors> SERIAL(flyingSectors);
  Creature* getCopulationTarget(Creature* succubus);
  Creature* getConsumptionTarget(Creature* consumer);
  deque<Creature*> SERIAL(pregnancies);
  mutable vector<ItemFetchInfo> itemFetchInfo;
  struct TeamInfo : public NamedTupleBase<vector<Creature*>, bool> {
    NAMED_TUPLE_STUFF(TeamInfo);
    NAME_ELEM(0, creatures);
    NAME_ELEM(1, active);
  };
  map<TeamId, TeamInfo> SERIAL(teamInfo);
  set<const Location*> SERIAL(knownLocations);
};

#endif
