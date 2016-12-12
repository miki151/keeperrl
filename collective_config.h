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
#include "square_type.h"
#include "util.h"
#include "minion_trait.h"
#include "workshop_type.h"
#include "cost_info.h"
#include "position.h"

enum class ItemClass;

class Game;
class Workshops;

using AttractionType = variant<FurnitureType, ItemIndex>;

struct AttractionInfo {
  AttractionInfo(int amountClaimed, vector<AttractionType>);
  AttractionInfo(int amountClaimed, AttractionType);

  SERIALIZATION_DECL(AttractionInfo)

  vector<AttractionType> SERIAL(types);
  int SERIAL(amountClaimed);
};

using ImmigrantRequirement = variant<AttractionInfo, TechId, SunlightState, CostInfo, FurnitureType>;

struct ImmigrantInfo {
  CreatureId SERIAL(id);
  double SERIAL(frequency);
  struct RequirementInfo {
    ImmigrantRequirement SERIAL(type);
    bool SERIAL(preliminary); // if true, candidate immigrant won't be generated if this requirement is not met.
    SERIALIZE_ALL(type, preliminary)
  };
  vector<RequirementInfo> SERIAL(requirements);
  void addRequirement(ImmigrantRequirement);
  void addPreliminaryRequirement(ImmigrantRequirement);
  EnumSet<MinionTrait> SERIAL(traits);
  bool SERIAL(spawnAtDorm);
  optional<Range> SERIAL(groupSize);
  bool SERIAL(autoTeam);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

struct BirthSpawn {
  CreatureId id;
  double frequency;
  optional<TechId> tech;
};

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

typedef function<const set<Position>&(const Collective*)> StorageDestinationFun;

struct ResourceInfo {
  StorageDestinationFun storageDestination;
  optional<ItemIndex> itemIndex;
  ItemId itemId;
  string name;
  ViewId viewId;
  bool dontDisplay;
};

typedef function<bool(const Collective*, const Item*)> CollectiveItemPredicate;

struct ItemFetchInfo {
  ItemIndex index;
  CollectiveItemPredicate predicate;
  StorageDestinationFun destinationFun;
  bool oneAtATime;
  CollectiveWarning warning;
};

struct MinionTaskInfo {
  enum Type { FURNITURE, EXPLORE, COPULATE, EAT, SPIDER } type;
  MinionTaskInfo();
  MinionTaskInfo(FurnitureType, const string& description);
  typedef function<bool(const Creature*, FurnitureType)> UsagePredicate;
  typedef function<bool(const Collective*, FurnitureType)> ActivePredicate;
  MinionTaskInfo(UsagePredicate, const string& description);
  MinionTaskInfo(UsagePredicate, ActivePredicate, const string& description);
  MinionTaskInfo(Type, const string& description, optional<CollectiveWarning> = none);
  UsagePredicate furniturePredicate;
  ActivePredicate activePredicate = [](const Collective*, FurnitureType) { return true; };
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
  static CollectiveConfig keeper(double immigrantFrequency, int payoutTime, double payoutMultiplier,
      int maxPopulation, vector<PopulationIncrease>, vector<ImmigrantInfo>);
  static CollectiveConfig withImmigrants(double immigrantFrequency, int maxPopulation, vector<ImmigrantInfo>);
  static CollectiveConfig noImmigrants();

  CollectiveConfig& allowRecruiting(int minPopulation);
  CollectiveConfig& setLeaderAsFighter();
  CollectiveConfig& setGhostSpawns(double prob, int number);
  CollectiveConfig& setGuardian(GuardianInfo);

  bool isLeaderFighter() const;
  bool getManageEquipment() const;
  bool getWorkerFollowLeader() const;
  double getImmigrantFrequency() const;
  int getPayoutTime() const;
  double getPayoutMultiplier() const;
  bool getStripSpawns() const;
  bool getFetchItems() const;
  bool getEnemyPositions() const;
  bool getWarnings() const;
  bool getConstructions() const;
  bool bedsLimitImmigration() const;
  int getMaxPopulation() const;
  int getNumGhostSpawns() const;
  double getGhostProb() const;
  optional<int> getRecruitingMinPopulation() const;
  bool sleepOnlyAtNight() const;
  const vector<ImmigrantInfo>& getImmigrantInfo() const;
  const vector<PopulationIncrease>& getPopulationIncreases() const;
  const optional<GuardianInfo>& getGuardianInfo() const;
  vector<BirthSpawn> getBirthSpawns() const;
  unique_ptr<Workshops> getWorkshops() const;
  static const WorkshopInfo& getWorkshopInfo(WorkshopType);
  static optional<WorkshopType> getWorkshopType(FurnitureType);

  bool hasImmigrantion(bool currentlyActiveModel) const;
  const EnumMap<SpawnType, DormInfo>& getDormInfo() const;
  const vector<FurnitureType>& getRoomsNeedingLight() const;
  static const ResourceInfo& getResourceInfo(CollectiveResourceId);
  static const vector<ItemFetchInfo>& getFetchInfo();
  static optional<int> getTrainingMaxLevelIncrease(FurnitureType);
  static const MinionTaskInfo& getTaskInfo(MinionTask);
  static const vector<FloorInfo>& getFloors();
  static double getEfficiencyBonus(FurnitureType);
  static bool canBuildOutsideTerritory(FurnitureType);
  static int getManaForConquering(VillainType);

  SERIALIZATION_DECL(CollectiveConfig);

  private:
  enum CollectiveType { KEEPER, VILLAGE };
  CollectiveConfig(double immigrantFrequency, int payoutTime, double payoutMultiplier,
      vector<ImmigrantInfo>, CollectiveType, int maxPopulation, vector<PopulationIncrease>);

  double SERIAL(immigrantFrequency);
  int SERIAL(payoutTime);
  double SERIAL(payoutMultiplier);
  int SERIAL(maxPopulation);
  vector<PopulationIncrease> SERIAL(populationIncreases);
  vector<ImmigrantInfo> SERIAL(immigrantInfo);
  CollectiveType SERIAL(type);
  optional<int> SERIAL(recruitingMinPopulation);
  bool SERIAL(leaderAsFighter) = false;
  int SERIAL(spawnGhosts) = 0;
  double SERIAL(ghostProb) = 0;
  optional<GuardianInfo> SERIAL(guardianInfo);
  void addBedRequirementToImmigrants();
};
