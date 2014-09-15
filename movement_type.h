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
  MovementType(initializer_list<MovementTrait> = {});
  MovementType(MovementTrait);
  MovementType(const Tribe*, EnumSet<MovementTrait> = {});
  bool hasTrait(MovementTrait) const;
  /** Returns if the argument can enter square define by this. The relation is not symmetric.*/
  bool canEnter(const MovementType&) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  SERIAL_CHECKER;
  EnumSet<MovementTrait> SERIAL(traits);
  const Tribe* SERIAL(tribe);
};

#endif
