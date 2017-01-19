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

#include "move_info.h"
#include "task_callback.h"
#include "resource_id.h"
#include "event_listener.h"
#include "entity_map.h"
#include "minion_trait.h"
#include "spawn_type.h"

class CollectiveAttack;
class Creature;
class CollectiveControl;
class Tribe;
class TribeId;
class Level;
class Trigger;
struct ImmigrantInfo;
struct AttractionInfo;
class MinionEquipment;
class TaskMap;
class KnownTiles;
class CollectiveTeams;
class ConstructionMap;
class Technology;
class CollectiveConfig;
class CostInfo;
struct TriggerInfo;
class Territory;
class CollectiveName;
template <typename T>
class EventProxy;
class Workshops;
class TileEfficiency;
class Zones;
struct ItemFetchInfo;
class CollectiveWarnings;
class Immigration;

class Collective : public TaskCallback {
  public:
  void addCreature(Creature*, EnumSet<MinionTrait>);
  void addCreature(PCreature, Position, EnumSet<MinionTrait>);
  MoveInfo getMove(Creature*);
  void setControl(PCollectiveControl);
  void tick();
  void update(bool currentlyActive);
  TribeId getTribeId() const;
  Tribe* getTribe() const;
  Level* getLevel() const;
  Model* getModel() const;
  Game* getGame() const;
  void addNewCreatureMessage(const vector<Creature*>&);
  void setTask(const Creature*, PTask);
  bool hasTask(const Creature*) const;
  void cancelTask(const Creature*);
  void banishCreature(Creature*);
  bool wasBanished(const Creature*) const;
  void setVillainType(VillainType);
  void setEnemyId(EnemyId);
  optional<VillainType> getVillainType() const;
  optional<EnemyId> getEnemyId() const;
  CollectiveControl* getControl() const;
  double getLocalTime() const;
  double getGlobalTime() const;

  typedef CollectiveResourceId ResourceId;

  SERIALIZATION_DECL(Collective);

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

  bool canPillage() const;
  bool hasTradeItems() const;
  vector<Item*> getTradeItems() const;
  PItem buyItem(Item*);
  vector<TriggerInfo> getTriggers(const Collective* against) const;

  double getEfficiency(const Creature*) const;
  const Creature* getLeader() const;
  Creature* getLeader();
  bool hasLeader() const;
  void clearLeader();

  const Territory& getTerritory() const;
  bool canClaimSquare(Position pos) const;
  void claimSquare(Position);
  const KnownTiles& getKnownTiles() const;
  const TileEfficiency& getTileEfficiency() const;
  void limitKnownTilesToModel();
  CollectiveWarnings& getWarnings();
  const CollectiveConfig& getConfig() const;

  bool usesEquipment(const Creature*) const;

  virtual ~Collective();

  int numResource(ResourceId) const;
  int numResourcePlusDebt(ResourceId) const;
  bool hasResource(const CostInfo&) const;
  void takeResource(const CostInfo&);
  void returnResource(const CostInfo&);

  const ConstructionMap& getConstructions() const;

  void setMinionTask(const Creature* c, MinionTask task);
  optional<MinionTask> getMinionTask(const Creature*) const;
  bool isMinionTaskPossible(Creature* c, MinionTask task);

  vector<Item*> getAllItems(bool includeMinions = true) const;
  vector<Item*> getAllItems(ItemPredicate predicate, bool includeMinions = true) const;
  vector<Item*> getAllItems(ItemIndex, bool includeMinions = true) const;

  vector<pair<Item*, Position>> getTrapItems(TrapType, const vector<Position>&) const;

  void orderExecution(Creature*);

  void addTrap(Position, TrapType);
  void removeTrap(Position);
  bool canAddFurniture(Position, FurnitureType) const;
  void addFurniture(Position, FurnitureType, const CostInfo&, bool noCredit);
  void removeFurniture(Position, FurnitureLayer);
  void destroySquare(Position, FurnitureLayer);
  bool isPlannedTorch(Position) const;
  bool canPlaceTorch(Position) const;
  void removeTorch(Position);
  void addTorch(Position);
  Zones& getZones();
  const Zones& getZones() const;
  void cancelMarkedTask(Position);
  void orderDestruction(Position pos, const DestroyAction&);
  double getDangerLevel() const;
  bool isMarked(Position) const;
  HighlightType getMarkHighlight(Position) const;
  void setPriorityTasks(Position);
  bool hasPriorityTasks(Position) const;

  bool hasTech(TechId id) const;
  void acquireTech(Technology*);
  vector<Technology*> getTechnologies() const;
  double getTechCost(Technology*);
  bool addKnownTile(Position);

  const EntitySet<Creature>& getKills() const;
  int getPoints() const;

  MinionEquipment& getMinionEquipment();
  const MinionEquipment& getMinionEquipment() const;
  optional<FurnitureType> getMissingTrainingDummy(const Creature*) const;

  Workshops& getWorkshops();
  const Workshops& getWorkshops() const;

  Immigration& getImmigration();
  const Immigration& getImmigration() const;

  int getPopulationSize() const;
  int getMaxPopulation() const;

  void orderConsumption(Creature* consumer, Creature* who);
  vector<Creature*> getConsumptionTargets(Creature* consumer) const;
  void addAttack(const CollectiveAttack&);
  void onRansomPaid();

  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  void freeTeamMembers(TeamId);

  const CollectiveName& getName() const;
  const TaskMap& getTaskMap() const;
  void updateResourceProduction();
  bool isItemMarked(const Item*) const;
  int getNumItems(ItemIndex, bool includeMinions = true) const;
  optional<set<Position>> getStorageFor(const Item*) const;

  void addKnownVillain(const Collective*);
  bool isKnownVillain(const Collective*) const;
  void addKnownVillainLocation(const Collective*);
  bool isKnownVillainLocation(const Collective*) const;

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  protected:
  // From Task::Callback
  virtual void onAppliedItem(Position, Item* item) override;
  virtual void onAppliedItemCancel(Position) override;
  virtual void onTaskPickedUp(Position, EntitySet<Item>) override;
  virtual void onCantPickItem(EntitySet<Item> items) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;
  virtual void onTorchBuilt(Position, Trigger*) override;
  virtual void onAppliedSquare(Creature*, Position) override;
  virtual void onKillCancelled(Creature*) override;
  virtual void onCopulated(Creature* who, Creature* with) override;
  virtual bool isConstructionReachable(Position) override;

  private:
  HeapAllocated<EventProxy<Collective>> SERIAL(eventProxy);
  friend EventProxy<Collective>;
  void onEvent(const GameEvent&);

  friend class CollectiveBuilder;
  Collective(Level*, const CollectiveConfig&, TribeId, EnumMap<ResourceId, int> credit, const CollectiveName&);
  void addCreatureInTerritory(PCreature, EnumSet<MinionTrait>);
  void removeCreature(Creature*);
  void onMinionKilled(Creature* victim, Creature* killer);
  void onKilledSomeone(Creature* victim, Creature* killer);

  void fetchItems(Position, const ItemFetchInfo&);

  void addMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForBanishing(const Creature*);

  bool isItemNeeded(const Item*) const;
  void addProducesMessage(const Creature*, const vector<PItem>&);
  int getDebt(ResourceId id) const;

  HeapAllocated<MinionEquipment> SERIAL(minionEquipment);
  EnumMap<ResourceId, int> SERIAL(credit);
  HeapAllocated<TaskMap> SERIAL(taskMap);
  vector<TechId> SERIAL(technologies);
  void markItem(const Item*);
  void unmarkItem(UniqueEntity<Item>::Id);

  HeapAllocated<KnownTiles> SERIAL(knownTiles);

  struct CurrentTaskInfo {
    MinionTask SERIAL(task);
    // If none then it's a one-time task. Upon allocating the task, the variable is set to a negative value,
    // so the job immediately times out after finishing the task.
    optional<double> SERIAL(finishTime);

    SERIALIZE_ALL(task, finishTime)
  };

  EntityMap<Creature, CurrentTaskInfo> SERIAL(currentTasks);
  optional<Position> getTileToExplore(const Creature*, MinionTask) const;
  PTask getStandardTask(Creature* c);
  PTask getEquipmentTask(Creature* c);
  void considerHealingTask(Creature* c);
  bool isTaskGood(const Creature*, MinionTask, bool ignoreTaskLock = false) const;
  void setRandomTask(const Creature*);

  void handleSurprise(Position);
  MoveInfo getDropItems(Creature*);
  MoveInfo getWorkerMove(Creature*);
  MoveInfo getTeamMemberMove(Creature*);
  int getTaskDuration(const Creature*, MinionTask) const;
  void decayMorale();
  vector<Creature*> SERIAL(creatures);
  Creature* SERIAL(leader) = nullptr;
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  EnumMap<SpawnType, vector<Creature*>> SERIAL(bySpawnType);
  PCollectiveControl SERIAL(control);
  HeapAllocated<TribeId> SERIAL(tribe);
  Level* SERIAL(level) = nullptr;
  HeapAllocated<Territory> SERIAL(territory);
  struct AlarmInfo {
    double SERIAL(finishTime);
    Position SERIAL(position);
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar & SVAR(finishTime) & SVAR(position);
    }
  };
  optional<AlarmInfo> SERIAL(alarmInfo);
  MoveInfo getAlarmMove(Creature* c);
  HeapAllocated<ConstructionMap> SERIAL(constructions);
  EntitySet<Item> SERIAL(markedItems);
  EntitySet<Creature> SERIAL(surrendering);
  void updateConstructions();
  void handleTrapPlacementAndProduction();
  void scheduleAutoProduction(function<bool (const Item*)> itemPredicate, int count);
  void delayDangerousTasks(const vector<Position>& enemyPos, double delayTime);
  bool isDelayed(Position);
  unordered_map<Position, double, CustomHash<Position>> SERIAL(delayedPos);
  vector<Position> getEnemyPositions() const;
  double manaRemainder = 0;
  double getKillManaScore(const Creature*) const;
  void addMana(double);
  EntitySet<Creature> SERIAL(kills);
  int SERIAL(points) = 0;
  HeapAllocated<CollectiveTeams> SERIAL(teams);
  HeapAllocated<CollectiveName> SERIAL(name);
  HeapAllocated<CollectiveConfig> SERIAL(config);
  EntitySet<Creature> SERIAL(banished);
  optional<VillainType> SERIAL(villainType);
  optional<EnemyId> SERIAL(enemyId);
  unique_ptr<Workshops> SERIAL(workshops);
  HeapAllocated<Zones> SERIAL(zones);
  HeapAllocated<TileEfficiency> SERIAL(tileEfficiency);
  HeapAllocated<CollectiveWarnings> SERIAL(warnings);
  HeapAllocated<Immigration> SERIAL(immigration);
  mutable optional<double> dangerLevelCache;
  unordered_set<const Collective*> SERIAL(knownVillains);
  unordered_set<const Collective*> SERIAL(knownVillainLocations);
};
