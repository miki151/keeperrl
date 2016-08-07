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
#ifndef _COLLECTIVE_CONFIG_INFO_H
#define _COLLECTIVE_CONFIG_INFO_H

#include "enum_variant.h"
#include "square_type.h"
#include "util.h"
#include "minion_task.h"
#include "workshop_type.h"

enum class ItemClass;

class Game;
class Workshops;

enum class AttractionId {
  SQUARE,
  ITEM_INDEX,
};

class MinionAttraction : public EnumVariant<AttractionId, TYPES(SquareType, ItemIndex),
    ASSIGN(SquareType, AttractionId::SQUARE),
    ASSIGN(ItemIndex, AttractionId::ITEM_INDEX)> {
  using EnumVariant::EnumVariant;
};

namespace std {
  template <> struct hash<MinionAttraction> {
    size_t operator()(const MinionAttraction& t) const {
      return (size_t)t.getId();
    }
  };
}

struct AttractionInfo {
  AttractionInfo(MinionAttraction, double amountClaimed, double minAmount, bool mandatory = false);

  SERIALIZATION_DECL(AttractionInfo);

  MinionAttraction SERIAL(attraction);
  double SERIAL(amountClaimed);
  double SERIAL(minAmount);
  bool SERIAL(mandatory);
};

struct ImmigrantInfo {
  CreatureId SERIAL(id);
  double SERIAL(frequency);
  vector<AttractionInfo> SERIAL(attractions);
  EnumSet<MinionTrait> SERIAL(traits);
  bool SERIAL(spawnAtDorm);
  int SERIAL(salary);
  optional<TechId> SERIAL(techId);
  optional<SunlightState> SERIAL(limit);
  optional<Range> SERIAL(groupSize);
  bool SERIAL(autoTeam);
  bool SERIAL(ignoreSpawnType);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

struct BirthSpawn {
  CreatureId id;
  double frequency;
  optional<TechId> tech;
};

struct PopulationIncrease {
  SquareApplyType SERIAL(type);
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
  SquareType dormType;
  optional<SquareType> getBedType() const;
  optional<CollectiveWarning> warning;
};

struct ResourceInfo {
  vector<SquareType> storageType;
  optional<ItemIndex> itemIndex;
  ItemId itemId;
  string name;
  ViewId viewId;
  bool dontDisplay;
};

struct MinionTaskInfo {
  enum Type { APPLY_SQUARE, EXPLORE, COPULATE, CONSUME, EAT, SPIDER } type;
  MinionTaskInfo(vector<SquareType>, const string& description, optional<CollectiveWarning> = none, double cost = 0,
      bool centerOnly = false);
  MinionTaskInfo(Type, const string& description, optional<CollectiveWarning> = none);
  vector<SquareType> squares;
  string description;
  optional<CollectiveWarning> warning;
  double cost = 0;
  bool centerOnly = false;
};

struct WorkshopInfo {
  SquareId squareType;
  MinionTask minionTask;
  string taskName;
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
  static optional<WorkshopType> getWorkshopType(MinionTask);

  bool activeImmigrantion(const Game*) const;
  const EnumMap<SpawnType, DormInfo>& getDormInfo() const;
  static optional<SquareType> getSecondarySquare(SquareType);
  unordered_set<SquareType> getEfficiencySquares() const;
  vector<SquareType> getRoomsNeedingLight() const;
  int getTaskDuration(const Creature*, MinionTask) const;
  static const map<CollectiveResourceId, ResourceInfo>& getResourceInfo();
  MinionTaskInfo getTaskInfo(MinionTask) const;
  static const vector<SquareType>& getEquipmentStorage();
  static const vector<SquareType>& getResourceStorage();

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
};


#endif

