#include "stdafx.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementType::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(traits)
    & SVAR(tribe)
    & SVAR(sunlightVulnerable)
    & SVAR(fireResistant)
    & SVAR(forced);
}

SERIALIZABLE(MovementType);

MovementType::MovementType(EnumSet<MovementTrait> t) : traits(t), tribe(nullptr) {
}

MovementType::MovementType(MovementTrait t) : traits({t}), tribe(nullptr) {
}

MovementType::MovementType(const Tribe* tr, EnumSet<MovementTrait> t) : traits(t), tribe(tr) {
}

bool MovementType::hasTrait(MovementTrait t) const {
  return traits[t];
}

bool MovementType::operator == (const MovementType& o) const {
  return traits == o.traits && tribe == o.tribe && sunlightVulnerable == o.sunlightVulnerable &&
      fireResistant == o.fireResistant && forced == o.forced;
}

const EnumSet<MovementTrait>& MovementType::getTraits() const {
  return traits;
}

namespace std {
  size_t hash<MovementType>::operator()(const MovementType& t) const {
      return hash<EnumSet<MovementTrait>>()(t.getTraits());
  }
}

const Tribe* MovementType::getTribe() const {
  return tribe;
}

MovementType MovementType::getWithNoTribe() const {
  MovementType ret(*this);
  ret.tribe = nullptr;
  return ret;
} 

MovementType& MovementType::removeTrait(MovementTrait trait) {
  traits.erase(trait);
  return *this;
}

MovementType& MovementType::addTrait(MovementTrait trait) {
  traits.insert(trait);
  return *this;
}

MovementType& MovementType::setSunlightVulnerable(bool state) {
  sunlightVulnerable = state;
  return *this;
}

MovementType& MovementType::setFireResistant(bool state) {
  fireResistant = state;
  return *this;
}

MovementType& MovementType::setForced(bool state) {
  forced = state;
  return *this;
}

bool MovementType::isSunlightVulnerable() const {
  return sunlightVulnerable;
}

bool MovementType::isFireResistant() const {
  return fireResistant;
}

bool MovementType::isForced() const {
  return forced;
}

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
  EnumSet<MovementTrait> rightTraits = (tribeOverrides && tribeOverrides->first == creature.getTribe()) ?
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
  traits[trait] = true;
  return *this;
}

MovementSet& MovementSet::removeTrait(MovementTrait trait) {
  traits[trait] = false;
  return *this;
}

MovementSet& MovementSet::addTraitForTribe(const Tribe* tribe, MovementTrait trait) {
  if (!tribeOverrides)
    tribeOverrides = {tribe, {trait}};
  else {
    CHECK(tribeOverrides->first == tribe);
    tribeOverrides->second[trait] = true;
  }
  return *this;
}

MovementSet& MovementSet::removeTraitForTribe(const Tribe* tribe, MovementTrait trait) {
  if (tribeOverrides) {
    CHECK(tribeOverrides->first == tribe);
    tribeOverrides->second[trait] = false;
  }
  return *this;
}

MovementSet& MovementSet::addForcibleTrait(MovementTrait trait) {
  forcibleTraits[trait] = true;
  return *this;
}

