#pragma once

class MovementType;
class Tribe;

#include "util.h"
#include "movement_type.h"

class MovementSet {
  public:
  bool canEnter(const MovementType&, bool covered, bool onFire, const optional<TribeId>& forbidden) const;
  bool canEnter(const MovementType&) const;

  bool hasTrait(MovementTrait) const;

  MovementSet& addTrait(MovementTrait);
  MovementSet& removeTrait(MovementTrait);
  MovementSet& addForcibleTrait(MovementTrait);
  MovementSet& setOnlyAllowed(TribeId);

  void clear();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  private:
  EnumSet<MovementTrait> SERIAL(traits);
  EnumSet<MovementTrait> SERIAL(forcibleTraits);
  optional<TribeId> SERIAL(onlyAllowed);
};
