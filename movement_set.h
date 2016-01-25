#ifndef _MOVEMENT_SET
#define _MOVEMENT_SET

class MovementType;
class Tribe;

#include "util.h"
#include "movement_type.h"

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

  void clear();

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


#endif
