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
class Workshops;
class TileEfficiency;
class Zones;
struct ItemFetchInfo;
class CollectiveWarnings;
class Immigration;

class Collective : public TaskCallback, public UniqueEntity<Collective>, public EventListener<Collective> {
  public:
  static PCollective create(WLevel, TribeId, const CollectiveName&);
  void init(CollectiveConfig&&, Immigration&&);
  void addCreature(WCreature, EnumSet<MinionTrait>);
  void addCreature(PCreature, Position, EnumSet<MinionTrait>);
  MoveInfo getMove(WCreature);
  void setControl(PCollectiveControl);
  void tick();
  void update(bool currentlyActive);
  TribeId getTribeId() const;
  Tribe* getTribe() const;
  WLevel getLevel() const;
  WModel getModel() const;
  WGame getGame() const;
  void addNewCreatureMessage(const vector<WCreature>&);
  void setTask(WCreature, PTask);
  bool hasTask(WConstCreature) const;
  void cancelTask(WConstCreature);
  void banishCreature(WCreature);
  bool wasBanished(WConstCreature) const;
  void setVillainType(VillainType);
  void setEnemyId(EnemyId);
  optional<VillainType> getVillainType() const;
  optional<EnemyId> getEnemyId() const;
  WCollectiveControl getControl() const;
  double getLocalTime() const;
  double getGlobalTime() const;

  typedef CollectiveResourceId ResourceId;

  SERIALIZATION_DECL(Collective);

  const vector<WCreature>& getCreatures() const;
  bool isConquered() const;

  const vector<WCreature>& getCreatures(SpawnType) const;
  const vector<WCreature>& getCreatures(MinionTrait) const;
  vector<WCreature> getCreaturesAnyOf(EnumSet<MinionTrait>) const;
  vector<WCreature> getCreatures(EnumSet<MinionTrait> with, EnumSet<MinionTrait> without = {}) const;
  bool hasTrait(WConstCreature, MinionTrait) const;
  bool hasAnyTrait(WConstCreature, EnumSet<MinionTrait>) const;
  void setTrait(WCreature c, MinionTrait);
  void removeTrait(WCreature c, MinionTrait);

  bool canPillage() const;
  bool hasTradeItems() const;
  vector<WItem> getTradeItems() const;
  PItem buyItem(WItem);
  vector<TriggerInfo> getTriggers(WConstCollective against) const;

  double getEfficiency(WConstCreature) const;
  WConstCreature getLeader() const;
  WCreature getLeader();
  bool hasLeader() const;
  void clearLeader();

  const Territory& getTerritory() const;
  Territory& getTerritory();
  bool canClaimSquare(Position pos) const;
  void claimSquare(Position);
  const KnownTiles& getKnownTiles() const;
  const TileEfficiency& getTileEfficiency() const;
  void retire();
  CollectiveWarnings& getWarnings();
  const CollectiveConfig& getConfig() const;

  bool usesEquipment(WConstCreature) const;

  virtual ~Collective();

  int numResource(ResourceId) const;
  int numResourcePlusDebt(ResourceId) const;
  bool hasResource(const CostInfo&) const;
  void takeResource(const CostInfo&);
  void returnResource(const CostInfo&);

  const ConstructionMap& getConstructions() const;

  void setMinionTask(WConstCreature c, MinionTask task);
  optional<MinionTask> getMinionTask(WConstCreature) const;
  bool isMinionTaskPossible(WCreature c, MinionTask task);

  vector<WItem> getAllItems(bool includeMinions = true) const;
  vector<WItem> getAllItems(ItemPredicate predicate, bool includeMinions = true) const;
  vector<WItem> getAllItems(ItemIndex, bool includeMinions = true) const;

  vector<pair<WItem, Position>> getTrapItems(TrapType, const vector<Position>&) const;

  void orderExecution(WCreature);

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
  optional<FurnitureType> getMissingTrainingDummy(WConstCreature) const;

  Workshops& getWorkshops();
  const Workshops& getWorkshops() const;

  Immigration& getImmigration();
  const Immigration& getImmigration() const;

  int getPopulationSize() const;
  int getMaxPopulation() const;

  void orderConsumption(WCreature consumer, WCreature who);
  vector<WCreature> getConsumptionTargets(WCreature consumer) const;
  void addAttack(const CollectiveAttack&);
  void onRansomPaid();

  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  void freeTeamMembers(TeamId);

  const CollectiveName& getName() const;
  const TaskMap& getTaskMap() const;
  void updateResourceProduction();
  bool isItemMarked(const WItem) const;
  int getNumItems(ItemIndex, bool includeMinions = true) const;
  optional<set<Position>> getStorageFor(const WItem) const;

  void addKnownVillain(WConstCollective);
  bool isKnownVillain(WConstCollective) const;
  void addKnownVillainLocation(WConstCollective);
  bool isKnownVillainLocation(WConstCollective) const;

  void onEvent(const GameEvent&);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  private:
  struct Private {};

  public:
  Collective(Private, WLevel, TribeId, const CollectiveName&);

  protected:
  // From Task::Callback
  virtual void onAppliedItem(Position, WItem item) override;
  virtual void onAppliedItemCancel(Position) override;
  virtual void onTaskPickedUp(Position, EntitySet<Item>) override;
  virtual void onCantPickItem(EntitySet<Item> items) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;
  virtual void onTorchBuilt(Position, WTrigger) override;
  virtual void onAppliedSquare(WCreature, Position) override;
  virtual void onKillCancelled(WCreature) override;
  virtual void onCopulated(WCreature who, WCreature with) override;
  virtual bool isConstructionReachable(Position) override;

  private:
  void addCreatureInTerritory(PCreature, EnumSet<MinionTrait>);
  void removeCreature(WCreature);
  void onMinionKilled(WCreature victim, WCreature killer);
  void onKilledSomeone(WCreature victim, WCreature killer);

  void fetchItems(Position, const ItemFetchInfo&);

  void addMoraleForKill(WConstCreature killer, WConstCreature victim);
  void decreaseMoraleForKill(WConstCreature killer, WConstCreature victim);
  void decreaseMoraleForBanishing(WConstCreature);

  bool isItemNeeded(const WItem) const;
  void addProducesMessage(WConstCreature, const vector<PItem>&);
  int getDebt(ResourceId id) const;

  HeapAllocated<MinionEquipment> SERIAL(minionEquipment);
  EnumMap<ResourceId, int> SERIAL(credit);
  HeapAllocated<TaskMap> SERIAL(taskMap);
  vector<TechId> SERIAL(technologies);
  void markItem(const WItem);
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
  optional<Position> getTileToExplore(WConstCreature, MinionTask) const;
  PTask getStandardTask(WCreature c);
  PTask getEquipmentTask(WCreature c);
  void considerHealingTask(WCreature c);
  bool isTaskGood(WConstCreature, MinionTask, bool ignoreTaskLock = false) const;
  void setRandomTask(WConstCreature);

  void handleSurprise(Position);
  MoveInfo getDropItems(WCreature);
  MoveInfo getWorkerMove(WCreature);
  MoveInfo getTeamMemberMove(WCreature);
  int getTaskDuration(WConstCreature, MinionTask) const;
  void decayMorale();
  vector<WCreature> SERIAL(creatures);
  WCreature SERIAL(leader) = nullptr;
  EnumMap<MinionTrait, vector<WCreature>> SERIAL(byTrait);
  EnumMap<SpawnType, vector<WCreature>> SERIAL(bySpawnType);
  PCollectiveControl SERIAL(control);
  HeapAllocated<TribeId> SERIAL(tribe);
  WLevel SERIAL(level) = nullptr;
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
  MoveInfo getAlarmMove(WCreature c);
  HeapAllocated<ConstructionMap> SERIAL(constructions);
  EntitySet<Item> SERIAL(markedItems);
  EntitySet<Creature> SERIAL(surrendering);
  void updateConstructions();
  void handleTrapPlacementAndProduction();
  void scheduleAutoProduction(function<bool (const WItem)> itemPredicate, int count);
  void delayDangerousTasks(const vector<Position>& enemyPos, double delayTime);
  bool isDelayed(Position);
  unordered_map<Position, double, CustomHash<Position>> SERIAL(delayedPos);
  vector<Position> getEnemyPositions() const;
  double manaRemainder = 0;
  double getKillManaScore(WConstCreature) const;
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
  EntitySet<Collective> SERIAL(knownVillains);
  EntitySet<Collective> SERIAL(knownVillainLocations);
};
