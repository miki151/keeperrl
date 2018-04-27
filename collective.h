
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
class TileEfficiency;
class Zones;
struct ItemFetchInfo;
class CollectiveWarnings;
class Immigration;
class Quarters;
class PositionMatching;

class Collective : public TaskCallback, public UniqueEntity<Collective>, public EventListener<Collective> {
  public:
  static PCollective create(WLevel, TribeId, const optional<CollectiveName>&, bool discoverable);
  void init(CollectiveConfig&&, Immigration&&);
  void acquireInitialTech();
  void addCreature(WCreature, EnumSet<MinionTrait>);
  void addCreature(PCreature, Position, EnumSet<MinionTrait>);
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
  bool isDiscoverable() const;
  void setEnemyId(EnemyId);
  VillainType getVillainType() const;
  optional<EnemyId> getEnemyId() const;
  WCollectiveControl getControl() const;
  LocalTime getLocalTime() const;
  GlobalTime getGlobalTime() const;

  typedef CollectiveResourceId ResourceId;

  SERIALIZATION_DECL(Collective)

  const vector<WCreature>& getCreatures() const;
  bool isConquered() const;

  const vector<WCreature>& getCreatures(MinionTrait) const;
  bool hasTrait(WConstCreature, MinionTrait) const;
  void setTrait(WCreature c, MinionTrait);
  void removeTrait(WCreature c, MinionTrait);

  bool canPillage() const;
  bool hasTradeItems() const;
  vector<WItem> getTradeItems() const;
  PItem buyItem(WItem);
  vector<TriggerInfo> getTriggers(WConstCollective against) const;

  double getEfficiency(WConstCreature) const;
  WCreature getLeader() const;

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

  void setMinionActivity(WCreature c, MinionActivity task);
  bool isActivityGood(WCreature, MinionActivity, bool ignoreTaskLock = false);

  vector<WItem> getAllItems(bool includeMinions = true) const;
  vector<WItem> getAllItems(ItemPredicate predicate, bool includeMinions = true) const;
  vector<WItem> getAllItems(ItemIndex, bool includeMinions = true) const;

  vector<pair<WItem, Position>> getTrapItems(TrapType, const vector<Position>&) const;

  void addTrap(Position, TrapType);
  void removeTrap(Position);
  bool canAddFurniture(Position, FurnitureType) const;
  void addFurniture(Position, FurnitureType, const CostInfo&, bool noCredit);
  void removeFurniture(Position, FurnitureLayer);
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
  double getDangerLevel() const;
  bool isMarked(Position) const;
  HighlightType getMarkHighlight(Position) const;
  void setPriorityTasks(Position);
  bool hasPriorityTasks(Position) const;

  bool hasTech(TechId id) const;
  void acquireTech(Technology*);
  vector<Technology*> getTechnologies() const;
  bool addKnownTile(Position);

  const EntitySet<Creature>& getKills() const;
  int getPoints() const;

  MinionEquipment& getMinionEquipment();
  const MinionEquipment& getMinionEquipment() const;
  optional<FurnitureType> getMissingTrainingFurniture(WConstCreature, ExperienceType) const;

  Workshops& getWorkshops();
  const Workshops& getWorkshops() const;

  Immigration& getImmigration();
  const Immigration& getImmigration() const;

  int getPopulationSize() const;
  int getMaxPopulation() const;

  vector<WCreature> getConsumptionTargets(WCreature consumer) const;
  void addAttack(const CollectiveAttack&);
  void onRansomPaid();
  void onExternalEnemyKilled(const string& name);

  CollectiveTeams& getTeams();
  const CollectiveTeams& getTeams() const;
  void freeTeamMembers(TeamId);

  const optional<CollectiveName>& getName() const;
  const TaskMap& getTaskMap() const;
  TaskMap& getTaskMap();
  void updateResourceProduction();
  bool isItemMarked(WConstItem) const;
  int getNumItems(ItemIndex, bool includeMinions = true) const;
  optional<PositionSet> getStorageFor(WConstItem) const;

  void addKnownVillain(WConstCollective);
  bool isKnownVillain(WConstCollective) const;
  void addKnownVillainLocation(WConstCollective);
  bool isKnownVillainLocation(WConstCollective) const;

  void onEvent(const GameEvent&);
  void onPositionDiscovered(Position);

  struct CurrentActivity {
    MinionActivity SERIAL(task);
    LocalTime SERIAL(finishTime);
    SERIALIZE_ALL(task, finishTime)
  };

  CurrentActivity getCurrentActivity(WConstCreature) const;
  struct AlarmInfo {
    GlobalTime SERIAL(finishTime);
    Position SERIAL(position);
    SERIALIZE_ALL(finishTime, position)
  };
  const optional<AlarmInfo>& getAlarmInfo() const;

  double getRebellionProbability() const;

  private:
  struct Private {};

  public:
  Collective(Private, WLevel, TribeId, const optional<CollectiveName>&);

  protected:
  // From Task::Callback
  virtual void onAppliedItem(Position, WItem item) override;
  virtual void onAppliedItemCancel(Position) override;
  virtual void onConstructed(Position, FurnitureType) override;
  virtual void onDestructed(Position, FurnitureType, const DestroyAction&) override;
  virtual void onAppliedSquare(WCreature, Position) override;
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

  bool isItemNeeded(WConstItem) const;
  void addProducesMessage(WConstCreature, const vector<PItem>&);
  int getDebt(ResourceId id) const;

  HeapAllocated<MinionEquipment> SERIAL(minionEquipment);
  EnumMap<ResourceId, int> SERIAL(credit);
  HeapAllocated<TaskMap> SERIAL(taskMap);
  vector<TechId> SERIAL(technologies);
  void markItem(WConstItem, WConstTask);
  void unmarkItem(UniqueEntity<Item>::Id);

  HeapAllocated<KnownTiles> SERIAL(knownTiles);

  EntityMap<Creature, CurrentActivity> SERIAL(currentActivity);
  optional<Position> getTileToExplore(WConstCreature, MinionActivity) const;

  void handleSurprise(Position);
  int getTaskDuration(WConstCreature, MinionActivity) const;
  void decayMorale();
  vector<WCreature> SERIAL(creatures);
  EnumMap<MinionTrait, vector<WCreature>> SERIAL(byTrait);
  PCollectiveControl SERIAL(control);
  HeapAllocated<TribeId> SERIAL(tribe);
  WLevel SERIAL(level) = nullptr;
  HeapAllocated<Territory> SERIAL(territory);
  optional<AlarmInfo> SERIAL(alarmInfo);
  HeapAllocated<ConstructionMap> SERIAL(constructions);
  EntityMap<Item, WConstTask> SERIAL(markedItems);
  void updateConstructions();
  void handleTrapPlacementAndProduction();
  void scheduleAutoProduction(function<bool (WConstItem)> itemPredicate, int count);
  void delayDangerousTasks(const vector<Position>& enemyPos, LocalTime delayTime);
  bool isDelayed(Position);
  unordered_map<Position, LocalTime, CustomHash<Position>> SERIAL(delayedPos);
  vector<Position> getEnemyPositions() const;
  double manaRemainder = 0;
  double getKillManaScore(WConstCreature) const;
  void addMana(double);
  EntitySet<Creature> SERIAL(kills);
  int SERIAL(points) = 0;
  HeapAllocated<CollectiveTeams> SERIAL(teams);
  HeapAllocated<optional<CollectiveName>> SERIAL(name);
  HeapAllocated<CollectiveConfig> SERIAL(config);
  EntitySet<Creature> SERIAL(banished);
  VillainType SERIAL(villainType);
  optional<EnemyId> SERIAL(enemyId);
  unique_ptr<Workshops> SERIAL(workshops);
  HeapAllocated<Zones> SERIAL(zones);
  HeapAllocated<TileEfficiency> SERIAL(tileEfficiency);
  HeapAllocated<CollectiveWarnings> SERIAL(warnings);
  PImmigration SERIAL(immigration);
  mutable optional<double> dangerLevelCache;
  EntitySet<Collective> SERIAL(knownVillains);
  EntitySet<Collective> SERIAL(knownVillainLocations);
  set<EnemyId> SERIAL(conqueredVillains); // OBSOLETE
  void setDiscoverable();
  bool SERIAL(discoverable) = false;
  void considerTransferingLostMinions();
  void considerRebellion();
  void updateCreatureStatus(WCreature);
  HeapAllocated<Quarters> SERIAL(quarters);
  PPositionMatching SERIAL(positionMatching);
};
