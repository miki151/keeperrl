#pragma once

#include "stdafx.h"
#include "util.h"
#include "cost_info.h"
#include "minion_trait.h"
#include "enemy_factory.h"
#include "sound.h"
#include "special_trait.h"
#include "tutorial_state.h"

MAKE_VARIANT2(AttractionType, FurnitureType, ItemIndex);

struct AttractionInfo {
  AttractionInfo(int amountClaimed, vector<AttractionType>);
  AttractionInfo(int amountClaimed, AttractionType);

  SERIALIZATION_DECL(AttractionInfo)

  static string getAttractionName(const AttractionType&, int count);

  int SERIAL(amountClaimed);
  vector<AttractionType> SERIAL(types);
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
  TutorialState SERIAL(state);
  SERIALIZE_ALL(state);
};

struct MinTurnRequirement {
  GlobalTime SERIAL(turn);
  SERIALIZE_ALL(turn)
};

MAKE_VARIANT2(ImmigrantRequirement,
    AttractionInfo,
    TechId,
    SunlightState,
    FurnitureType,
    CostInfo,
    ExponentialCost,
    Pregnancy,
    RecruitmentInfo,
    TutorialRequirement,
    MinTurnRequirement
);

struct OutsideTerritory { SERIALIZE_EMPTY() };
struct NearLeader { SERIALIZE_EMPTY() };

MAKE_VARIANT2(SpawnLocation, FurnitureType, OutsideTerritory, NearLeader, Pregnancy);

class ImmigrantInfo {
  public:
  ImmigrantInfo(CreatureId, EnumSet<MinionTrait>);
  ImmigrantInfo(vector<CreatureId>, EnumSet<MinionTrait>);
  CreatureId getId(int numCreated) const;
  CreatureId getNonRandomId(int numCreated) const;
  bool isAvailable(int numCreated) const;
  const SpawnLocation& getSpawnLocation() const;
  Range getGroupSize() const;
  int getInitialRecruitment() const;
  bool isAutoTeam() const;
  double getFrequency() const;
  bool isPersistent() const;
  bool isInvisible() const;
  const EnumSet<MinionTrait>& getTraits() const;
  optional<int> getLimit() const;
  optional<Keybinding> getKeybinding() const;
  optional<Sound> getSound() const;
  bool isNoAuto() const;
  optional<TutorialHighlight> getTutorialHighlight() const;
  bool isHiddenInHelp() const;

  struct SpecialTraitInfo {
    double SERIAL(prob);
    vector<SpecialTrait> SERIAL(traits);
    SERIALIZE_ALL(NAMED(prob), NAMED(traits))
  };

  const vector<SpecialTraitInfo>& getSpecialTraits() const;

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
  ImmigrantInfo& setInvisible();
  ImmigrantInfo& setLimit(int);
  ImmigrantInfo& setTutorialHighlight(TutorialHighlight);
  ImmigrantInfo& setHiddenInHelp();
  ImmigrantInfo& addSpecialTrait(double chance, SpecialTrait);
  ImmigrantInfo& addSpecialTrait(double chance, vector<SpecialTrait>);
  ImmigrantInfo& addOneOrMoreTraits(double chance, vector<LastingEffect>);

  template <typename Visitor>
  struct RequirementVisitor {
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
      requirement.type.visit(v);
    }
  }

  template <typename Visitor>
  void visitRequirements(const Visitor& visitor) const {
    for (auto& requirement : requirements) {
      requirement.type.visit(visitor);
    }
  }

  SERIALIZATION_DECL(ImmigrantInfo)

  private:

  vector<CreatureId> SERIAL(ids);
  optional<double> SERIAL(frequency);
  struct RequirementInfo {
    double SERIAL(candidateProb); // chance of candidate immigrant still generated if this requirement is not met.
    ImmigrantRequirement SERIAL(type);
    SERIALIZE_ALL(NAMED(candidateProb), NAMED(type))
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
  bool SERIAL(invisible) = false;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  bool SERIAL(hiddenInHelp) = false;
  vector<SpecialTraitInfo> SERIAL(specialTraits);
};
