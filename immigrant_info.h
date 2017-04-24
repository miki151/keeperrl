#pragma once

#include "stdafx.h"
#include "util.h"
#include "cost_info.h"
#include "minion_trait.h"
#include "enemy_factory.h"
#include "sound.h"

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

class Collective;
class Game;

struct RecruitmentInfo {
  EnumSet<EnemyId> SERIAL(enemyId);
  int SERIAL(minPopulation);
  MinionTrait SERIAL(trait);
  WCollective findEnemy(WGame) const;
  vector<WCreature> getAvailableRecruits(WGame, CreatureId) const;
  vector<WCreature> getAllRecruits(WGame, CreatureId) const;
  SERIALIZE_ALL(enemyId, minPopulation, trait)
};

struct TutorialRequirement {
  STutorial SERIAL(tutorial);
  template <class Archive>
  void serialize(Archive&, const unsigned);
};

using ImmigrantRequirement = variant<
    AttractionInfo,
    TechId,
    SunlightState,
    FurnitureType,
    CostInfo,
    ExponentialCost,
    Pregnancy,
    RecruitmentInfo,
    TutorialRequirement
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
  optional<Keybinding> getKeybinding() const;
  optional<Sound> getSound() const;
  bool isNoAuto() const;
  optional<TutorialHighlight> getTutorialHighlight() const;
  bool isHiddenInHelp() const;

  ImmigrantInfo& addRequirement(double candidateProb, ImmigrantRequirement);
  ImmigrantInfo& addRequirement(ImmigrantRequirement);
  ImmigrantInfo& setFrequency(double);
  ImmigrantInfo& setSpawnLocation(SpawnLocation);
  ImmigrantInfo& setInitialRecruitment(int);
  ImmigrantInfo& setGroupSize(Range);
  ImmigrantInfo& setAutoTeam();
  ImmigrantInfo& setKeybinding(Keybinding);
  ImmigrantInfo& setSound(Sound);
  ImmigrantInfo& setNoAuto();
  ImmigrantInfo& setLimit(int);
  ImmigrantInfo& setTutorialHighlight(TutorialHighlight);
  ImmigrantInfo& setHiddenInHelp();

  template <typename Visitor>
  struct RequirementVisitor : public boost::static_visitor<void> {
    RequirementVisitor(const Visitor& v, double p) : visitor(v), prob(p) {}
    const Visitor& visitor;
    double prob;
    template <typename Req>
    void operator()(const Req& r) const {
      visitor(r, prob);
    }
  };

  template <typename Visitor>
  void visitRequirementsAndProb(const Visitor& visitor) const {
    for (auto& requirement : requirements) {
      RequirementVisitor<Visitor> v {visitor, requirement.candidateProb};
      applyVisitor(requirement.type, v);
    }
  }

  template <typename Visitor>
  void visitRequirements(const Visitor& visitor) const {
    for (auto& requirement : requirements) {
      applyVisitor(requirement.type, visitor);
    }
  }

  SERIALIZATION_DECL(ImmigrantInfo)

  private:

  vector<CreatureId> SERIAL(ids);
  optional<double> SERIAL(frequency);
  struct RequirementInfo {
    ImmigrantRequirement SERIAL(type);
    double SERIAL(candidateProb); // chance of candidate immigrant still generated if this requirement is not met.
    SERIALIZE_ALL(type, candidateProb)
  };
  vector<RequirementInfo> SERIAL(requirements);
  EnumSet<MinionTrait> SERIAL(traits);
  SpawnLocation SERIAL(spawnLocation) = OutsideTerritory{};
  Range SERIAL(groupSize) = Range(1, 2);
  int SERIAL(initialRecruitment) = 0;
  bool SERIAL(autoTeam) = false;
  bool SERIAL(consumeIds) = false;
  optional<Keybinding> SERIAL(keybinding);
  optional<Sound> SERIAL(sound);
  bool SERIAL(noAuto) = false;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  bool SERIAL(hiddenInHelp) = false;
};
