#ifndef _MOVEMENT_TYPE
#define _MOVEMENT_TYPE

#include "util.h"
#include "tribe.h"

RICH_ENUM(MovementTrait,
  WALK,
  FLY,
  SWIM,
  WADE
);

class MovementType {
  public:
  MovementType(EnumSet<MovementTrait> = {});
  MovementType(MovementTrait);
  MovementType(TribeSet, EnumSet<MovementTrait>);
  bool hasTrait(MovementTrait) const;
  bool isCompatible(TribeId) const;
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
  optional<TribeSet> SERIAL(tribeSet);
};

namespace std {
  template <> struct hash<MovementType> {
    size_t operator()(const MovementType& t) const;
  };
}

#endif
