#ifndef _MOVEMENT_TYPE
#define _MOVEMENT_TYPE

#include "util.h"

RICH_ENUM(MovementTrait,
  WALK,
  FLY,
  SWIM,
  WADE
);

class Tribe;

class MovementType {
  public:
  MovementType(EnumSet<MovementTrait> = {});
  MovementType(MovementTrait);
  MovementType(const Tribe*, EnumSet<MovementTrait> = {});
  bool hasTrait(MovementTrait) const;
  const Tribe* getTribe() const;
  MovementType& removeTrait(MovementTrait);
  MovementType& addTrait(MovementTrait);
  MovementType& setSunlightVulnerable(bool = true);
  MovementType& setFireResistant(bool = true);
  MovementType& setForced(bool = true);
  
  bool isSunlightVulnerable() const;
  bool isFireResistant() const;
  bool isForced() const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  bool operator == (const MovementType&) const;
  const EnumSet<MovementTrait>& getTraits() const;

  private:
  bool SERIAL(sunlightVulnerable) = false;
  bool SERIAL(fireResistant) = false;
  bool SERIAL(forced) = false;
  EnumSet<MovementTrait> SERIAL(traits);
  const Tribe* SERIAL(tribe);
};

class MovementSet {
  public:
  /** Returns if the argument can enter square defined by this.*/
  bool canEnter(const MovementType&) const;

  MovementSet& setOnFire(bool);
  bool isOnFire() const;
  MovementSet& setSunlight(bool = true);
  
  MovementSet& addTrait(MovementTrait);
  MovementSet& removeTrait(MovementTrait);
  MovementSet& addTraitForTribe(const Tribe*, MovementTrait);
  MovementSet& removeTraitForTribe(const Tribe*, MovementTrait);
  MovementSet& addForcibleTrait(MovementTrait);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  private:
  bool SERIAL(onFire) = false;
  bool SERIAL(sunlight) = false;
  typedef EnumSet<MovementTrait> TribeSet;
  TribeSet SERIAL(traits);
  TribeSet SERIAL(forcibleTraits);
  optional<pair<const Tribe*, TribeSet>> SERIAL(tribeOverrides);
};

namespace std {
  template <> struct hash<MovementType> {
    size_t operator()(const MovementType& t) const;
  };
}

#endif
