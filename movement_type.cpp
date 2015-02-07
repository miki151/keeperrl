#include "stdafx.h"
#include "movement_type.h"
#include "tribe.h"

template <class Archive> 
void MovementType::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(traits) & SVAR(tribe);
  CHECK_SERIAL;
}

SERIALIZABLE(MovementType);

MovementType::MovementType(initializer_list<MovementTrait> t) : traits(t), tribe(nullptr) {
}

MovementType::MovementType(MovementTrait t) : traits({t}), tribe(nullptr) {
}

MovementType::MovementType(const Tribe* tr, EnumSet<MovementTrait> t) : traits(t), tribe(tr) {
}

bool MovementType::hasTrait(MovementTrait t) const {
  return traits[t];
}

bool MovementType::canEnter(const MovementType& t) const {
  if (tribe && t.tribe && t.tribe != tribe)
    return false;
  for (auto trait : traits)
    if (t.hasTrait(trait))
      return true;
  return false;
}

bool MovementType::operator == (const MovementType& o) const {
  return traits == o.traits && tribe == o.tribe;
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
  return MovementType(nullptr, traits);
}
