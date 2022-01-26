#include "stdafx.h"
#include "movement_set.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementSet::serialize(Archive& ar, const unsigned int version) {
  ar(OPTION(traits), OPTION(forcibleTraits), OPTION(blockingPrisoners), OPTION(blockingFarmAnimals), OPTION(blockingEnemies), SKIP(tribe));
}

SERIALIZABLE(MovementSet);
SERIALIZATION_CONSTRUCTOR_IMPL(MovementSet)

MovementSet::MovementSet(TribeId id) : tribe(id) {
}

bool MovementSet::canEnter(const MovementType& movementType, bool covered, bool onFire,
    const optional<TribeId>& forbidden) const {
  if (blockingEnemies && !movementType.isCompatible(tribe))
    return false;
  if (!movementType.isForced()) {
    if ((blockingPrisoners && movementType.isPrisoner()) ||
        (blockingFarmAnimals && movementType.isFarmAnimal()) ||
        (!covered && movementType.isSunlightVulnerable()) ||
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

bool MovementSet::blocksPrisoners() const {
  return blockingPrisoners;
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

MovementSet& MovementSet::setBlockingEnemies() {
  blockingEnemies = true;
  return *this;
}

TribeId MovementSet::getTribe() const {
  return tribe;
}

void MovementSet::setTribe(TribeId id) {
  tribe = id;
}

MovementSet& MovementSet::clearTraits() {
  traits.clear();
  forcibleTraits.clear();
  blockingEnemies = false;
  return *this;
}

#include "pretty_archive.h"
template void MovementSet::serialize(PrettyInputArchive&, unsigned);
