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


