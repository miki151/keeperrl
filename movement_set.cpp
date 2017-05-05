#include "stdafx.h"
#include "movement_set.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementSet::serialize(Archive& ar, const unsigned int version) {
  ar(traits, forcibleTraits, onlyAllowed);
}

SERIALIZABLE(MovementSet);

bool MovementSet::canEnter(const MovementType& movementType, bool covered, bool onFire, const optional<TribeId>& forbidden) const {
  if (onlyAllowed && !movementType.isCompatible(*onlyAllowed))
    return false;
  if (!movementType.isForced()) {
    if ((!covered && movementType.isSunlightVulnerable()) ||
        (onFire && !movementType.isFireResistant()) ||
        (forbidden && movementType.isCompatible(*forbidden)))
      return false;
  }
  EnumSet<MovementTrait> rightTraits(traits);
  if (movementType.isForced())
    rightTraits = rightTraits.sum(forcibleTraits);
  for (auto trait : rightTraits)
    if (movementType.hasTrait(trait))
      return true;
  return false;
}

bool MovementSet::canEnter(const MovementType& t) const {
  return canEnter(t, true, false, none);
}

bool MovementSet::hasTrait(MovementTrait trait) const {
  return traits.contains(trait);
}

MovementSet& MovementSet::addTrait(MovementTrait trait) {
  traits.insert(trait);
  return *this;
}

MovementSet& MovementSet::removeTrait(MovementTrait trait) {
  traits.erase(trait);
  return *this;
}

MovementSet& MovementSet::addForcibleTrait(MovementTrait trait) {
  forcibleTraits.insert(trait);
  return *this;
}

MovementSet& MovementSet::setOnlyAllowed(TribeId id) {
  onlyAllowed = id;
  return *this;
}

void MovementSet::clear() {
  traits.clear();
  forcibleTraits.clear();
}
