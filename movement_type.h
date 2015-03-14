#ifndef _MOVEMENT_TYPE
#define _MOVEMENT_TYPE

#include "util.h"

RICH_ENUM(MovementTrait,
  WALK,
  FLY,
  SWIM,
  WADE,
  BY_FORCE,
  FIRE_RESISTANT,
  SUNLIGHT_VULNERABLE
);

class Tribe;

class MovementType {
  public:
  MovementType(initializer_list<MovementTrait> = {});
  MovementType(MovementTrait);
  MovementType(const Tribe*, EnumSet<MovementTrait> = {});
  bool hasTrait(MovementTrait) const;
  /** Returns if the argument can enter square defined by this. The relation is not symmetric.*/
  bool canEnter(const MovementType&) const;
  const Tribe* getTribe() const;
  void removeTrait(MovementTrait);
  void addTrait(MovementTrait);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  bool operator == (const MovementType&) const;
  const EnumSet<MovementTrait>& getTraits() const;

  MovementType getWithNoTribe() const;

  private:
  SERIAL_CHECKER;
  EnumSet<MovementTrait> SERIAL(traits);
  const Tribe* SERIAL(tribe);
};

namespace std {
  template <> struct hash<MovementType> {
    size_t operator()(const MovementType& t) const;
  };
}

#endif
