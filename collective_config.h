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

#include "enum_variant.h"
#include "util.h"
#include "minion_trait.h"
#include "workshop_type.h"
#include "cost_info.h"
#include "position.h"

enum class ItemClass;

class Game;
class Workshops;
class ImmigrantInfo;
class Technology;

struct ResourceInfo;
struct ItemFetchInfo;

struct PopulationIncrease {
  FurnitureType SERIAL(type);
  double SERIAL(increasePerSquare);
  int SERIAL(maxIncrease);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

struct GuardianInfo {
  CreatureId SERIAL(creature);
  double SERIAL(probability);
  int SERIAL(minEnemies);
  int SERIAL(minVictims);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

struct DormInfo {
  FurnitureType bedType;
  optional<CollectiveWarning> warning;
};

struct MinionTaskInfo {
  enum Type { FURNITURE, EXPLORE, COPULATE, EAT, SPIDER, WORKER, ARCHERY } type;
  MinionTaskInfo();
  MinionTaskInfo(FurnitureType, const string& description);
  typedef function<bool(WConstCollective, WConstCreature, FurnitureType)> UsagePredicate;
  typedef function<bool(WConstCollective, FurnitureType)> ActivePredicate;
  MinionTaskInfo(UsagePredicate, const string& description);
  MinionTaskInfo(UsagePredicate, ActivePredicate, const string& description);
  MinionTaskInfo(Type, const string& description, optional<CollectiveWarning> = none);
  UsagePredicate furniturePredicate = [](WConstCollective, WConstCreature, FurnitureType) { return true; };
  ActivePredicate activePredicate = [](WConstCollective, FurnitureType) { return true; };
  string description;
  optional<CollectiveWarning> warning;
};

struct WorkshopInfo {
  FurnitureType furniture;
  string taskName;
  SkillId skill;
};

struct FloorInfo {
  FurnitureType type;
  CostInfo cost;
  string name;
  double efficiencyBonus;
};

class CollectiveConfig {
  public:
  static CollectiveConfig keeper(int immigrantInterval, int maxPopulation, bool regenerateMana,
      vector<PopulationIncrease>, const vector<ImmigrantInfo>&);
  static CollectiveConfig withImmigrants(int immigrantInterval, int maxPopulation, const vector<ImmigrantInfo>&);
  static CollectiveConfig noImmigrants();

  CollectiveConfig& setLeaderAsFighter();
  CollectiveConfig& setGhostSpawns(double prob, int number);
  CollectiveConfig& setGuardian(GuardianInfo);

  bool isLeaderFighter() const;
  bool getManageEquipment() const;
  bool getFollowLeaderIfNoTerritory() const;
  int getImmigrantInterval() const;
  bool getStripSpawns() const;
  bool getFetchItems() const;
  bool getEnemyPositions() const;
  bool getWarnings() const;
  bool getConstructions() const;
  bool bedsLimitImmigration() const;
  int getMaxPopulation() const;
  int getNumGhostSpawns() const;
  int getImmigrantTimeout() const;
  double getGhostProb() const;
  bool hasVillainSleepingTask() const;
  bool getRegenerateMana() const;
  const vector<ImmigrantInfo>& getImmigrantInfo() const;
  const vector<PopulationIncrease>& getPopulationIncreases() const;
  const optional<GuardianInfo>& getGuardianInfo() const;
  unique_ptr<Workshops> getWorkshops() const;
  vector<Technology*> getInitialTech() const;

  static const WorkshopInfo& getWorkshopInfo(WorkshopType);
  static optional<WorkshopType> getWorkshopType(FurnitureType);

  map<CollectiveResourceId, int> getStartingResource() const;

  bool hasImmigrantion(bool currentlyActiveModel) const;
  const EnumMap<SpawnType, DormInfo>& getDormInfo() const;
  const vector<FurnitureType>& getRoomsNeedingLight() const;
  static const ResourceInfo& getResourceInfo(CollectiveResourceId);
  static const vector<ItemFetchInfo>& getFetchInfo();
  static optional<int> getTrainingMaxLevel(ExperienceType, FurnitureType);
  static const vector<FurnitureType>& getTrainingFurniture(ExperienceType);
  static const MinionTaskInfo& getTaskInfo(MinionTask);
  static const vector<FloorInfo>& getFloors();
  static double getEfficiencyBonus(FurnitureType);
  static bool canBuildOutsideTerritory(FurnitureType);
  static int getManaForConquering(const optional<VillainType>&);

  SERIALIZATION_DECL(CollectiveConfig)
  CollectiveConfig(const CollectiveConfig&);
  ~CollectiveConfig();

  private:
  enum CollectiveType { KEEPER, VILLAGE };
  CollectiveConfig(int immigrantInterval, const vector<ImmigrantInfo>&, CollectiveType, int maxPopulation,
      vector<PopulationIncrease>);

  int SERIAL(immigrantInterval);
  int SERIAL(maxPopulation);
  vector<PopulationIncrease> SERIAL(populationIncreases);
  vector<ImmigrantInfo> SERIAL(immigrantInfo);
  CollectiveType SERIAL(type);
  bool SERIAL(leaderAsFighter) = false;
  int SERIAL(spawnGhosts) = 0;
  double SERIAL(ghostProb) = 0;
  optional<GuardianInfo> SERIAL(guardianInfo);
  void addBedRequirementToImmigrants();
  bool SERIAL(regenerateMana) = false;
};
