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
  MovementSet& setCovered(bool);
  bool isCovered() const;
  
  MovementSet& addTrait(MovementTrait);
  MovementSet& removeTrait(MovementTrait);
  MovementSet& addTraitForTribe(TribeId, MovementTrait);
  MovementSet& removeTraitForTribe(TribeId, MovementTrait);
  MovementSet& addForcibleTrait(MovementTrait);

  void clear();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  private:
  bool SERIAL(onFire) = false;
  bool SERIAL(covered) = false;
  EnumSet<MovementTrait> SERIAL(traits);
  EnumSet<MovementTrait> SERIAL(forcibleTraits);
  optional<pair<TribeId, EnumSet<MovementTrait>>> SERIAL(tribeOverrides);
};


#endif
