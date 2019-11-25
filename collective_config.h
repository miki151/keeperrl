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
#include "game_time.h"
#include "furniture_type.h"
#include "conquer_condition.h"
#include "creature_id.h"

enum class ItemClass;

class Game;
class Workshops;
class Technology;
class ImmigrantInfo;

struct ResourceInfo;
struct ItemFetchInfo;
class ContentFactory;

struct GuardianInfo {
  CreatureId SERIAL(creature);
  double SERIAL(probability);
  int SERIAL(minEnemies);
  int SERIAL(minVictims);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

struct MinionActivityInfo {
  enum Type { FURNITURE, EXPLORE, COPULATE, EAT, SPIDER, WORKER, ARCHERY, IDLE, MINION_ABUSE, GUARD } type;
  MinionActivityInfo();
  MinionActivityInfo(FurnitureType, const string& description);
  MinionActivityInfo(FurnitureUsageType, const string& description);
  typedef function<bool(const ContentFactory*, WConstCollective, const Creature*, FurnitureType)> UsagePredicate;
  MinionActivityInfo(UsagePredicate, const string& description);
  MinionActivityInfo(Type, const string& description);
  UsagePredicate furniturePredicate = [](const ContentFactory*, WConstCollective, const Creature*, FurnitureType) { return true; };
  string description;
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
  static CollectiveConfig keeper(TimeInterval immigrantInterval, int maxPopulation, ConquerCondition);
  static CollectiveConfig noImmigrants();

  bool isLeaderFighter() const;
  bool getManageEquipment() const;
  bool getFollowLeaderIfNoTerritory() const;
  bool stayInTerritory() const;
  TimeInterval getImmigrantInterval() const;
  bool getStripSpawns() const;
  bool getEnemyPositions() const;
  bool getWarnings() const;
  bool getConstructions() const;
  int getMaxPopulation() const;
  int getNumGhostSpawns() const;
  TimeInterval getImmigrantTimeout() const;
  double getGhostProb() const;
  bool hasVillainSleepingTask() const;
  bool allowHealingTaskOutsideTerritory() const;
  const optional<GuardianInfo>& getGuardianInfo() const;
  bool isConquered(const Collective*) const;
  CollectiveConfig& setConquerCondition(ConquerCondition);

  static const WorkshopInfo& getWorkshopInfo(WorkshopType);
  static optional<WorkshopType> getWorkshopType(FurnitureType);

  static void addBedRequirementToImmigrants(vector<ImmigrantInfo>&, ContentFactory*);

  map<CollectiveResourceId, int> getStartingResource() const;

  bool hasImmigrantion(bool currentlyActiveModel) const;
  static const ResourceInfo& getResourceInfo(CollectiveResourceId);
  const vector<ItemFetchInfo>& getFetchInfo() const;
  static const MinionActivityInfo& getActivityInfo(MinionActivity);

  SERIALIZATION_DECL(CollectiveConfig)
  CollectiveConfig(const CollectiveConfig&);
  ~CollectiveConfig();

  private:
  enum CollectiveType { KEEPER, VILLAGE };
  CollectiveConfig(TimeInterval immigrantInterval, CollectiveType, int maxPopulation, ConquerCondition);

  TimeInterval SERIAL(immigrantInterval);
  int SERIAL(maxPopulation) = 10000;
  CollectiveType SERIAL(type) = CollectiveType::VILLAGE;
  bool SERIAL(leaderAsFighter) = false;
  int SERIAL(spawnGhosts) = 0;
  double SERIAL(ghostProb) = 0;
  optional<GuardianInfo> SERIAL(guardianInfo);
  ConquerCondition SERIAL(conquerCondition) = ConquerCondition::KILL_FIGHTERS_AND_LEADER;
};
