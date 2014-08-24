#ifndef _COLLECTIVE_H
#define _COLLECTIVE_H

#include "monster_ai.h"
#include "task.h"
#include "event.h"
#include "minion_equipment.h"
#include "spell_info.h"
#include "task_map.h"

class Creature;
class CollectiveControl;
class Tribe;
class Deity;
class Level;

RICH_ENUM(MinionTrait,
  LEADER,
  FIGHTER,
  WORKER,
  PRISONER,
  NO_EQUIPMENT,
  SPAWN_HUMANOID,
  SPAWN_GOLEM,
  SPAWN_UNDEAD,
  SPAWN_BEAST,
);

RICH_ENUM(MinionTask,
  SLEEP,
  GRAVE,
  TRAIN,
  WORKSHOP,
  STUDY,
  RESEARCH,
  LABORATORY,
  PRISON,
  TORTURE,
  SACRIFICE,
  EXECUTE,
  WORSHIP,
  LAIR,
  EXPLORE,
);

RICH_ENUM(CollectiveConfig,
  MANAGE_EQUIPMENT,
  WORKER_FOLLOW_LEADER,
);

class Collective : public Task::Callback {
  public:
  Collective(Tribe*);
  void addCreature(Creature*, EnumSet<MinionTrait>);
  void addCreature(PCreature, Vec2, EnumSet<MinionTrait>);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void setLevel(Level*);
  void tick(double time);
  const Tribe* getTribe() const;
  Tribe* getTribe();
  double getStanding(const Deity*) const;
  double getWarLevel() const;
  Level* getLevel();
  const Level* getLevel() const;
  double getTime() const;
  void update(Creature*);
  void setConfig(CollectiveConfig);

  virtual void onAppliedItem(Vec2 pos, Item* item) override;
  virtual void onAppliedItemCancel(Vec2 pos) override;
  virtual void onPickedUp(Vec2 pos, EntitySet<Item>) override;
  virtual void onCantPickItem(EntitySet<Item> items) override;
  virtual void onConstructed(Vec2, SquareType) override;
  virtual void onAppliedSquare(Vec2 pos) override;
  virtual void onKillCancelled(Creature*) override;

  SERIALIZATION_DECL(Collective);

  vector<Creature*>& getCreatures();
  const vector<Creature*>& getCreatures() const;

  vector<Creature*>& getCreatures(MinionTrait);
  vector<Creature*> getCreaturesAnyOf(EnumSet<MinionTrait>) const;
  const vector<Creature*>& getCreatures(MinionTrait) const;
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
  void changeSquareType(Vec2 pos, SquareType from, SquareType to);
  bool containsSquare(Vec2 pos) const;
  bool isKnownSquare(Vec2 pos) const;
  bool underAttack() const;

  void updateEfficiency(Vec2, SquareType);
  double getEfficiency(Vec2) const;
  bool hasEfficiency(Vec2) const;

  bool isGuardPost(Vec2) const;
  void addGuardPost(Vec2);
  void removeGuardPost(Vec2);
  void freeFromGuardPost(const Creature*);

  ~Collective();

  enum class Warning;
  void setWarning(Warning, bool state = true);
  bool isWarning(Warning) const;

  enum class ResourceId {
    GOLD,
    WOOD,
    IRON,
    STONE,
    MANA,
    PRISONER_HEAD,
  };

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
    Optional<Warning> warning;
    bool dontDisplay;
  };

  struct MinionTaskInfo {
    enum Type { APPLY_SQUARE, EXPLORE } type;
    MinionTaskInfo(vector<SquareType>, const string& description, Optional<Warning> = Nothing(),
        bool centerOnly = false);
    MinionTaskInfo(Type, const string&);
    vector<SquareType> squares;
    string description;
    Optional<Warning> warning;
    bool centerOnly;
  };

  map<MinionTask, MinionTaskInfo> getTaskInfo() const;

  const static map<ResourceId, ResourceInfo> resourceInfo;


  int numResource(ResourceId) const;
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

  vector<ItemFetchInfo> getFetchInfo() const;
  void fetchItems(Vec2 pos, ItemFetchInfo, bool ignoreDelayed = false);

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
  vector<pair<Item*, Vec2>> getTrapItems(TrapType, set<Vec2> = {}) const;

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
  double getTechCost();
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

  private:
  SERIAL_CHECKER;

  REGISTER_HANDLER(CombatEvent, const Creature*);
  REGISTER_HANDLER(KillEvent, const Creature* victim, const Creature* killer);
  REGISTER_HANDLER(WorshipEvent, Creature* who, const Deity*, WorshipType);
  REGISTER_HANDLER(AttackEvent, Creature* victim, Creature* attacker);
  REGISTER_HANDLER(SquareReplacedEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(AlarmEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(SurrenderEvent, Creature* who, const Creature* to);
  REGISTER_HANDLER(TrapTriggerEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(TrapDisarmEvent, const Level*, Vec2 pos);
  REGISTER_HANDLER(EquipEvent, const Creature*, const Item*);
  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items);
  REGISTER_HANDLER(TortureEvent, Creature* who, const Creature* torturer);

  MinionEquipment SERIAL(minionEquipment);
  map<ResourceId, int> SERIAL(credit);
  EnumSet<CollectiveConfig> SERIAL(config);
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

  void handleSurprise(Vec2 pos);
  EnumSet<Warning> warnings;
  MoveInfo getDropItems(Creature *c);
  MoveInfo getWorkerMove(Creature* c);
  bool usesEquipment(const Creature* c) const;
  void autoEquipment(Creature* creature, bool replace);
  Item* getWorstItem(vector<Item*> items) const;
  int getTaskDuration(Creature*, MinionTask) const;
  map<UniqueEntity<Creature>::Id, string> SERIAL(minionTaskStrings);
  double getStanding(EpithetId id) const;
  void onEpithetWorship(Creature*, WorshipType, EpithetId);
  void considerHealingLeader();
  vector<Creature*> SERIAL(creatures);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
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
  void addMana(double);
  unordered_map<const Creature*, double> SERIAL(lastCombat);
  vector<const Creature*> SERIAL(kills);
  int SERIAL2(points, 0);
};

RICH_ENUM(Collective::Warning,
    DIGGING,
    STORAGE,WOOD,
    IRON,
    STONE,
    GOLD,
    LIBRARY,
    MINIONS,
    BEDS,
    TRAINING,
    WORKSHOP,
    LABORATORY,
    NO_WEAPONS,
    GRAVES,
    CHESTS,
    NO_PRISON,
    LARGER_PRISON,
    TORTURE_ROOM,
    ALTAR,
    MORE_CHESTS,
    MANA
);


#endif
