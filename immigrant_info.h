#pragma once

#include "stdafx.h"
#include "util.h"
#include "cost_info.h"
#include "minion_trait.h"

using AttractionType = variant<FurnitureType, ItemIndex>;

struct AttractionInfo {
  AttractionInfo(int amountClaimed, vector<AttractionType>);
  AttractionInfo(int amountClaimed, AttractionType);

  SERIALIZATION_DECL(AttractionInfo)

  static string getAttractionName(const AttractionType&, int count);

  vector<AttractionType> SERIAL(types);
  int SERIAL(amountClaimed);
};

struct ExponentialCost {
  CostInfo SERIAL(base);
  int SERIAL(numToDoubleCost);
  int SERIAL(numFree);
  SERIALIZE_ALL(base, numToDoubleCost, numFree)
};

struct Pregnancy { SERIALIZE_EMPTY() };

using ImmigrantRequirement = variant<
    AttractionInfo,
    TechId,
    SunlightState,
    FurnitureType,
    CostInfo,
    ExponentialCost,
    Pregnancy
>;

struct OutsideTerritory { SERIALIZE_EMPTY() };
struct NearLeader { SERIALIZE_EMPTY() };

using SpawnLocation = variant<FurnitureType, OutsideTerritory, NearLeader, Pregnancy>;

class ImmigrantInfo {
  public:
  ImmigrantInfo(CreatureId, EnumSet<MinionTrait>);
  ImmigrantInfo(vector<CreatureId>, EnumSet<MinionTrait>);
  CreatureId getId(int numCreated) const;
  bool isAvailable(int numCreated) const;
  const SpawnLocation& getSpawnLocation() const;
  Range getGroupSize() const;
  int getInitialRecruitment() const;
  bool isAutoTeam() const;
  double getFrequency() const;
  bool isPersistent() const;
  const EnumSet<MinionTrait>& getTraits() const;
  optional<int> getLimit() const;

  ImmigrantInfo& addRequirement(ImmigrantRequirement);
  ImmigrantInfo& addPreliminaryRequirement(ImmigrantRequirement);
  ImmigrantInfo& setFrequency(double);
  ImmigrantInfo& setSpawnLocation(SpawnLocation);
  ImmigrantInfo& setInitialRecruitment(int);
  ImmigrantInfo& setGroupSize(Range);
  ImmigrantInfo& setAutoTeam();

  template <typename Visitor>
  void visitRequirements(const Visitor& visitor) const {
    for (auto& requirement : requirements)
      apply_visitor(requirement.type, visitor);
  }
  template <typename Visitor>
  void visitPreliminaryRequirements(const Visitor& visitor) const {
    for (auto& requirement : requirements)
      if (requirement.preliminary)
        apply_visitor(requirement.type, visitor);
  }

  SERIALIZATION_DECL(ImmigrantInfo)

  private:

  vector<CreatureId> SERIAL(ids);
  optional<double> SERIAL(frequency);
  struct RequirementInfo {
    ImmigrantRequirement SERIAL(type);
    bool SERIAL(preliminary); // if true, candidate immigrant won't be generated if this requirement is not met.
    SERIALIZE_ALL(type, preliminary)
  };
  vector<RequirementInfo> SERIAL(requirements);
  EnumSet<MinionTrait> SERIAL(traits);
  SpawnLocation SERIAL(spawnLocation) = OutsideTerritory{};
  Range SERIAL(groupSize) = Range(1, 2);
  int SERIAL(initialRecruitment) = 0;
  bool SERIAL(autoTeam) = false;
  bool SERIAL(consumeIds) = false;
};
