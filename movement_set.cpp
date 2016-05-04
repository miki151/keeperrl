#include "stdafx.h"
#include "movement_set.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementSet::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(onFire)
    & SVAR(sunlight)
    & SVAR(traits)
    & SVAR(forcibleTraits)
    & SVAR(tribeOverrides);
}

SERIALIZABLE(MovementSet);

bool MovementSet::canEnter(const MovementType& creature) const {
  if (!creature.isForced()) {
    if ((sunlight && creature.isSunlightVulnerable()) || (onFire && !creature.isFireResistant()))
      return false;
  }
  EnumSet<MovementTrait> rightTraits = (tribeOverrides && creature.getTribe() == tribeOverrides->first) ?
      tribeOverrides->second : traits;
  if (creature.isForced())
    rightTraits = rightTraits.sum(forcibleTraits);
  for (auto trait : rightTraits)
    if (creature.hasTrait(trait))
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

MovementSet& MovementSet::setSunlight(bool state) {
  sunlight = state;
  return *this;
}

MovementSet& MovementSet::addTrait(MovementTrait trait) {
  traits.insert(trait);
  return *this;
}

MovementSet& MovementSet::removeTrait(MovementTrait trait) {
  traits.erase(trait);
  return *this;
}

MovementSet& MovementSet::addTraitForTribe(TribeId tribe, MovementTrait trait) {
  if (!tribeOverrides)
    tribeOverrides = {tribe, {trait}};
  else {
    CHECK(tribeOverrides->first == tribe);
    tribeOverrides->second.insert(trait);
  }
  return *this;
}

MovementSet& MovementSet::removeTraitForTribe(TribeId tribe, MovementTrait trait) {
  if (tribeOverrides) {
    CHECK(tribeOverrides->first == tribe);
    tribeOverrides->second.erase(trait);
  }
  return *this;
}

MovementSet& MovementSet::addForcibleTrait(MovementTrait trait) {
  forcibleTraits.insert(trait);
  return *this;
}

void MovementSet::clear() {
  traits.clear();
  forcibleTraits.clear();
  tribeOverrides = none;
}
