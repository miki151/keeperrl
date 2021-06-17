#pragma once

class MovementType;
class Tribe;

#include "util.h"
#include "movement_type.h"

class MovementSet {
  public:
  MovementSet(TribeId);
  bool canEnter(const MovementType&, bool covered, bool onFire, const optional<TribeId>& forbidden) const;
  bool canEnter(const MovementType&) const;

  bool hasTrait(MovementTrait) const;
  bool blocksPrisoners() const;

  MovementSet& addTrait(MovementTrait);
  MovementSet& removeTrait(MovementTrait);
  MovementSet& addForcibleTrait(MovementTrait);
  MovementSet& setBlockingEnemies();
  TribeId getTribe() const;
  void setTribe(TribeId);

  MovementSet& clearTraits();

  SERIALIZATION_DECL(MovementSet)
  
  private:
  EnumSet<MovementTrait> SERIAL(traits);
  EnumSet<MovementTrait> SERIAL(forcibleTraits);
  bool SERIAL(blockingEnemies) = false;
  bool SERIAL(blockingPrisoners) = false;
  TribeId SERIAL(tribe);
};
