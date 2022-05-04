#pragma once

#include "stdafx.h"
#include "util.h"
#include "cost_info.h"
#include "minion_trait.h"
#include "enemy_factory.h"
#include "sound.h"
#include "tutorial_state.h"
#include "furniture_type.h"
#include "tech_id.h"
#include "pretty_archive.h"
#include "sunlight_info.h"

class ContentFactory;
struct SpecialTraitInfo;
class SpecialTrait;

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
  vector<Collective*> findEnemy(WGame) const;
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

struct ImmigrantFlag {
  string SERIAL(value);
  SERIALIZE_ALL(value)
};

#define IMMIGRANT_REQUIREMENT_LIST\
  X(AttractionInfo, 0)\
  X(TechId, 1)\
  X(SunlightState, 2)\
  X(FurnitureType, 3)\
  X(CostInfo, 4)\
  X(ExponentialCost, 5)\
  X(Pregnancy, 6)\
  X(RecruitmentInfo, 7)\
  X(TutorialRequirement, 8)\
  X(MinTurnRequirement, 9)\
  X(NegateRequirement, 10)\
  X(ImmigrantFlag, 11)

#define VARIANT_TYPES_LIST IMMIGRANT_REQUIREMENT_LIST
#define VARIANT_NAME ImmigrantRequirement

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

struct OutsideTerritory { SERIALIZE_EMPTY() };
struct InsideTerritory { SERIALIZE_EMPTY() };
struct NearLeader { SERIALIZE_EMPTY() };

#define VARIANT_TYPES_LIST\
  X(FurnitureType, 0)\
  X(OutsideTerritory, 1)\
  X(InsideTerritory, 2)\
  X(NearLeader, 3)\
  X(Pregnancy, 4)

#define VARIANT_NAME SpawnLocation

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME


class ImmigrantInfo {
  public:
  ImmigrantInfo(CreatureId, EnumSet<MinionTrait>);
  ImmigrantInfo(vector<CreatureId>, EnumSet<MinionTrait>);
  STRUCT_DECLARATIONS(ImmigrantInfo)
  CreatureId getId(int numCreated) const;
  CreatureId getNonRandomId(int numCreated) const;
  bool isAvailable(int numCreated) const;
  const SpawnLocation& getSpawnLocation() const;
  Range getGroupSize() const;
  int getInitialRecruitment() const;
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

  ImmigrantInfo& addRequirement(ImmigrantRequirement);

  template <typename Visitor>
  void visitRequirements(const Visitor& visitor) const {
    for (auto& requirement : requirements) {
      requirement.type.template visit<void>(visitor);
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
  bool SERIAL(consumeIds) = false;
  optional<Keybinding> SERIAL(keybinding);
  optional<Sound> SERIAL(sound);
  bool SERIAL(noAuto) = false;
  bool SERIAL(invisible) = false;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  bool SERIAL(hiddenInHelp) = false;
  vector<SpecialTraitInfo> SERIAL(specialTraits);
};

static_assert(std::is_nothrow_move_constructible<ImmigrantInfo>::value, "T should be noexcept MoveConstructible");
