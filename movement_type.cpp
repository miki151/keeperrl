#include "stdafx.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementType::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(traits)
    & SVAR(tribeSet)
    & SVAR(sunlightVulnerable)
    & SVAR(fireResistant)
    & SVAR(forced);
}

SERIALIZABLE(MovementType);

MovementType::MovementType(EnumSet<MovementTrait> t) : traits(t) {
}

MovementType::MovementType(MovementTrait t) : traits({t}) {
}

MovementType::MovementType(TribeSet tr, EnumSet<MovementTrait> t) : traits(t), tribeSet(tr) {
}

bool MovementType::hasTrait(MovementTrait t) const {
  return traits.contains(t);
}

bool MovementType::operator == (const MovementType& o) const {
  return traits == o.traits && tribeSet == o.tribeSet && sunlightVulnerable == o.sunlightVulnerable &&
      fireResistant == o.fireResistant && forced == o.forced;
}

const EnumSet<MovementTrait>& MovementType::getTraits() const {
  return traits;
}

namespace std {
  size_t hash<MovementType>::operator()(const MovementType& t) const {
      return combineHash(t.getTraits());
  }
}

bool MovementType::isCompatible(TribeId id) const {
  return !tribeSet || tribeSet->contains(id);
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


