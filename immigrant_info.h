#pragma once

#include "stdafx.h"
#include "util.h"
#include "cost_info.h"
#include "minion_trait.h"
#include "enemy_factory.h"
#include "sound.h"
#include "special_trait.h"
#include "tutorial_state.h"
#include "furniture_type.h"
#include "tech_id.h"

class ContentFactory;

MAKE_VARIANT2(AttractionType, FurnitureType, ItemIndex);

struct AttractionInfo {
  AttractionInfo(int amountClaimed, vector<AttractionType>);
  AttractionInfo(int amountClaimed, AttractionType);

  SERIALIZATION_DECL(AttractionInfo)

  static string getAttractionName(const ContentFactory*, const AttractionType&, int count);

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
  vector<EnemyId> SERIAL(enemyId);
  int SERIAL(minPopulation);
  MinionTrait SERIAL(trait);
  WCollective findEnemy(WGame) const;
  vector<Creature*> getAvailableRecruits(WGame, CreatureId) const;
  vector<Creature*> getAllRecruits(WGame, CreatureId) const;
  SERIALIZE_ALL(enemyId, minPopulation, trait)
};

struct TutorialRequirement {
  TutorialState SERIAL(state);
  SERIALIZE_ALL(state)
};

struct MinTurnRequirement {
  GlobalTime SERIAL(turn);
  SERIALIZE_ALL(turn)
};

class ImmigrantRequirement;

struct NegateRequirement {
  HeapAllocated<ImmigrantRequirement> SERIAL(r);
  SERIALIZE_ALL(r)
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
    MinTurnRequirement,
    NegateRequirement
);

struct OutsideTerritory { SERIALIZE_EMPTY() };
struct InsideTerritory { SERIALIZE_EMPTY() };
struct NearLeader { SERIALIZE_EMPTY() };

MAKE_VARIANT2(SpawnLocation, FurnitureType, OutsideTerritory, InsideTerritory, NearLeader, Pregnancy);

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
  void visitRequirements(const Visitor& visitor) const {
    for (auto& requirement : requirements) {
      requirement.type.visit(visitor);
    }
  }

  SERIALIZATION_DECL(ImmigrantInfo)

  struct RequirementInfo {
    double SERIAL(candidateProb); // chance of candidate immigrant still generated if this requirement is not met.
    ImmigrantRequirement SERIAL(type);
    SERIALIZE_ALL(NAMED(candidateProb), NAMED(type))
  };
  vector<RequirementInfo> SERIAL(requirements);
  bool SERIAL(stripEquipment) = true;

  private:

  vector<CreatureId> SERIAL(ids);
  optional<double> SERIAL(frequency);
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
