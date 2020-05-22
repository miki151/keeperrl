
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
#include "dungeon_level.h"
#include "enemy_id.h"

class CollectiveAttack;
class Creature;
class CollectiveControl;
class Tribe;
class TribeId;
class Level;
class Trigger;
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
struct CollectiveName;
class Workshops;
class Zones;
struct ItemFetchInfo;
class CollectiveWarnings;
class Immigration;
class Quarters;
class PositionMatching;
class MinionActivities;
class ResourceInfo;

class Collective : public TaskCallback, public UniqueEntity<Collective>, public EventListener<Collective> {
  public:
  static PCollective create(WModel model, TribeId, const optional<CollectiveName>&, bool discoverable,
      const ContentFactory*);
  void init(CollectiveConfig);
  void setWorkshops(unique_ptr<Workshops>);
  void setImmigration(PImmigration);
  void addCreature(Creature*, EnumSet<MinionTrait>);
  Creature* addCreature(PCreature, Position, EnumSet<MinionTrait>);
  void setPopulationGroup(const vector<Creature*>&);
  void setControl(PCollectiveControl);
  void makeConqueredRetired(Collective* conqueror);
  void tick();
  void update(bool currentlyActive);
  TribeId getTribeId() const;
  Tribe* getTribe() const;
  WModel getModel() const;
  WGame getGame() const;
  typedef CollectiveResourceId ResourceId;
  const ResourceInfo& getResourceInfo(ResourceId) const;
  void addNewCreatureMessage(const vector<Creature*>&);
  void setTask(Creature*, PTask);
  bool hasTask(const Creature*) const;
  void freeFromTask(const Creature*);
  void banishCreature(Creature*);
  void removeCreature(Creature*);
  bool wasBanished(const Creature*) const;
  void setVillainType(VillainType);
  bool isDiscoverable() const;
  void setEnemyId(EnemyId);
  VillainType getVillainType() const;
  optional<EnemyId> getEnemyId() const;
  CollectiveControl* getControl() const;
  LocalTime getLocalTime() const;
  GlobalTime getGlobalTime() const;
  const PositionSet& getStoragePositions(StorageId) const;


  SERIALIZATION_DECL(Collective)

  const vector<Creature*>& getCreatures() const;
  bool isConquered() const;

  const vector<Creature*>& getCreatures(MinionTrait) const;
  bool hasTrait(const Creature*, MinionTrait) const;
  void setTrait(Creature* c, MinionTrait);
  void removeTrait(Creature* c, MinionTrait);
  EnumSet<MinionTrait> getAllTraits(Creature*) const;

  bool hasTradeItems() const;
  vector<Item*> getTradeItems() const;
  PItem buyItem(Item*);
  vector<TriggerInfo> getTriggers(const Collective* against) const;

  double getEfficiency(const Creature*) const;
  const vector<Creature*>& getLeaders() const;
  bool needsToBeKilledToConquer(const Creature*) const;

  const Territory& getTerritory() const;
  Territory& getTerritory();
  bool canClaimSquare(Position pos) const;
  void claimSquare(Position);
  const KnownTiles& getKnownTiles() const;
  void retire();
  CollectiveWarnings& getWarnings();
  const CollectiveConfig& getConfig() const;

  bool usesEquipment(const Creature*) const;

  virtual ~Collective() override;

  int numResource(ResourceId) const;
  int numResourcePlusDebt(ResourceId) const;
  bool hasResource(const CostInfo&) const;
  void takeResource(const CostInfo&);
  void returnResource(const CostInfo&);

  const ConstructionMap& getConstructions() const;
  ConstructionMap& getConstructions();

  void setMinionActivity(Creature* c, MinionActivity task);
  bool isActivityGood(Creature*, MinionActivity, bool ignoreTaskLock = false);
  bool isActivityGoodAssumingHaveTasks(Creature*, MinionActivity, bool ignoreTaskLock = false);

  using GroupLockedActivities = EnumSet<MinionActivity>;
  GroupLockedActivities& getGroupLockedActivities(const string& group);
  const GroupLockedActivities& getGroupLockedActivities(const string& group) const;
  GroupLockedActivities getGroupLockedActivities(const Creature*) const;

  vector<Item*> getAllItems(bool includeMinions = true) const;
  vector<Item*> getAllItems(ItemIndex, bool includeMinions = true) const;
  vector<pair<Position, vector<Item*>>> getStoredItems(ItemIndex, StorageId) const;
  struct TrapItemInfo;
  vector<TrapItemInfo> getTrapItems(const vector<Position>&) const;

  void addTrap(Position, FurnitureType);
  void removeTrap(Position);
  bool canAddFurniture(Position, FurnitureType) const;
  void addFurniture(Position, FurnitureType, const CostInfo&, bool noCredit);
  void removeUnbuiltFurniture(Position, FurnitureLayer);
  void destroyOrder(Position, FurnitureLayer);
  bool isPlannedTorch(Position) const;
  bool canPlaceTorch(Position) const;
  void removeTorch(Position);
  void addTorch(Position);
  Zones& getZones();
  const Zones& getZones() const;
  Quarters& getQuarters();
  const Quarters& getQuarters() const;
  void cancelMarkedTask(Position);
  void orderDestruction(Position pos, const DestroyAction&);
  void installBodyPart(Item*, Creature* target);
  double getDangerLevel() const;
  void setPriorityTasks(Position);
  bool hasPriorityTasks(Position) const;

  void acquireTech(TechId, bool throughLevelling);
  const Technology& getTechnology() const;
  void setTechnology(Technology);
  bool addKnownTile(Position);

  const EntitySet<Creature>& getKills() const;
  int getPoints() const;

  MinionEquipment& getMinionEquipment();
  const MinionEquipment& getMinionEquipment() const;
  optional<FurnitureType> getMissingTrainingFurniture(const Creature*, ExperienceType) const;

  Workshops& getWorkshops();
  const Workshops& getWorkshops() const;

  Immigration& getImmigration();
  const Immigration& getImmigration() const;
  const MinionActivities& getMinionActivities() const;
  int getPopulationSize() const;
  int getMaxPopulation() const;
  const DungeonLevel& getDungeonLevel() const;
  DungeonLevel& getDungeonLevel();

  vector<Creature*> getConsumptionTargets(Creature* consumer) const;
  void onRansomPaid();
  void onExternalEnemyKilled(const string& name);
  void addRecordedEvent(string);
  const unordered_set<string>& getRecordedEvents() const;

  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  void freeTeamMembers(const vector<Creature*>& members);

  const heap_optional<CollectiveName>& getName() const;
  const TaskMap& getTaskMap() const;
  TaskMap& getTaskMap();
  WConstTask getItemTask(const Item*) const;
  int getNumItems(ItemIndex, bool includeMinions = true) const;
  const PositionSet& getStorageForPillagedItem(const Item*) const;

  void addKnownVillain(const Collective*);
  bool isKnownVillain(const Collective*) const;
  void addKnownVillainLocation(const Collective*);
  bool isKnownVillainLocation(const Collective*) const;

  void onEvent(const GameEvent&);

  struct CurrentActivity {
    MinionActivity SERIAL(activity);
    LocalTime SERIAL(finishTime);
    SERIALIZE_ALL(activity, finishTime)
  };

  CurrentActivity getCurrentActivity(const Creature*) const;
  struct AlarmInfo {
    GlobalTime SERIAL(finishTime);
    Position SERIAL(position);
    SERIALIZE_ALL(finishTime, position)
  };
  string getMinionGroupName(const Creature*) const;
  vector<string> getAutomatonGroupNames(const Creature*) const;
  const optional<AlarmInfo>& getAlarmInfo() const;

  double getRebellionProbability() const;

  // From Task::Callback
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;

  private:
  struct Private {};

  public:
  Collective(Private, WModel, TribeId, const optional<CollectiveName>&, const ContentFactory*);

  protected:
  // From Task::Callback
  virtual void onAppliedItem(Position, Item* item) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onAppliedSquare(Creature*, pair<Position, FurnitureLayer>) override;
  virtual void onCopulated(Creature* who, Creature* with) override;
  virtual bool isConstructionReachable(Position) override;
  virtual bool containsCreature(Creature*) override;

  private:
  void onMinionKilled(Creature* victim, Creature* killer);
  void onKilledSomeone(Creature* victim, Creature* killer);

  void fetchItems(Position, const ItemFetchInfo&);

  void addMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForKill(const Creature* killer, const Creature* victim);
  void decreaseMoraleForBanishing(const Creature*);

  bool isItemNeeded(const Item*) const;
  void addProducesMessage(const Creature*, const vector<PItem>&, const char* verb = "produces");
  int getDebt(ResourceId id) const;

  PPositionMatching SERIAL(positionMatching);
  HeapAllocated<MinionEquipment> SERIAL(minionEquipment);
  unordered_map<ResourceId, int, CustomHash<ResourceId>> SERIAL(credit);
  HeapAllocated<TaskMap> SERIAL(taskMap);
  HeapAllocated<Technology> SERIAL(technology);
  void markItem(const Item*, WConstTask);
  void unmarkItem(UniqueEntity<Item>::Id);

  HeapAllocated<KnownTiles> SERIAL(knownTiles);

  EntityMap<Creature, CurrentActivity> SERIAL(currentActivity);
  optional<Position> getTileToExplore(const Creature*, MinionActivity) const;

  void handleSurprise(Position);
  int getTaskDuration(const Creature*, MinionActivity) const;
  vector<Creature*> SERIAL(creatures);
  vector<vector<Creature*>> SERIAL(populationGroups);
  EnumMap<MinionTrait, vector<Creature*>> SERIAL(byTrait);
  PCollectiveControl SERIAL(control);
  HeapAllocated<TribeId> SERIAL(tribe);
  WModel SERIAL(model) = nullptr;
  HeapAllocated<Territory> SERIAL(territory);
  optional<AlarmInfo> SERIAL(alarmInfo);
  HeapAllocated<ConstructionMap> SERIAL(constructions);
  EntityMap<Item, WeakPointer<const Task>> SERIAL(markedItems);
  void updateConstructions();
  void handleTrapPlacementAndProduction();
  void scheduleAutoProduction(function<bool (const Item*)> itemPredicate, int count);
  void delayDangerousTasks(const vector<Position>& enemyPos, LocalTime delayTime);
  bool isDelayed(Position);
  unordered_map<Position, LocalTime, CustomHash<Position>> SERIAL(delayedPos);
  vector<Position> getEnemyPositions() const;
  EntitySet<Creature> SERIAL(kills);
  int SERIAL(points) = 0;
  HeapAllocated<CollectiveTeams> SERIAL(teams);
  heap_optional<CollectiveName> SERIAL(name);
  HeapAllocated<CollectiveConfig> SERIAL(config);
  EntitySet<Creature> SERIAL(banished);
  VillainType SERIAL(villainType);
  optional<EnemyId> SERIAL(enemyId);
  unique_ptr<Workshops> SERIAL(workshops);
  HeapAllocated<Zones> SERIAL(zones);
  HeapAllocated<CollectiveWarnings> SERIAL(warnings);
  PImmigration SERIAL(immigration);
  mutable optional<double> dangerLevelCache;
  EntitySet<Collective> SERIAL(knownVillains);
  EntitySet<Collective> SERIAL(knownVillainLocations);
  bool SERIAL(discoverable) = false;
  void considerRebellion();
  void updateCreatureStatus(Creature*);
  HeapAllocated<Quarters> SERIAL(quarters);
  int SERIAL(populationIncrease) = 0;
  DungeonLevel SERIAL(dungeonLevel);
  bool SERIAL(hadALeader) = false;
  vector<Item*> getAllItemsImpl(optional<ItemIndex>, bool includeMinions) const;
  // Remove after alpha 27
  void updateBorderTiles();
  bool updatedBorderTiles = false;
  HeapAllocated<MinionActivities> SERIAL(minionActivities);
  unordered_set<string> SERIAL(recordedEvents);
  unordered_set<string> SERIAL(allRecordedEvents);
  map<string, GroupLockedActivities> SERIAL(groupLockedAcitivities);
  bool SERIAL(attackedByPlayer) = false;
  void updateGuardTasks();
  void updateAutomatonPartsTasks();
  void updateAutomatonEngines();
  bool creatureConsideredPlayer(Creature*) const;
  void summonDemon(Creature* summoner);
};
