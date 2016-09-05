#include "stdafx.h"
#include "movement_set.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementSet::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, onFire, covered, traits, forcibleTraits);
}

SERIALIZABLE(MovementSet);

bool MovementSet::canEnter(const MovementType& movementType) const {
  if (!movementType.isForced()) {
    if ((!covered && movementType.isSunlightVulnerable()) || (onFire && !movementType.isFireResistant()))
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

MovementSet& MovementSet::setOnFire(bool state) {
  onFire = state;
  return *this;
}

bool MovementSet::isOnFire() const {
  return onFire;
}

MovementSet& MovementSet::setCovered(bool state) {
  covered = state;
  return *this;
}

bool MovementSet::isCovered() const {
  return covered;
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

void MovementSet::clear() {
  traits.clear();
  forcibleTraits.clear();
}
