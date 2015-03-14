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

static vector<MovementTrait> necessaryTraits { MovementTrait::FIRE_RESISTANT };
static vector<MovementTrait> reverseTraits { MovementTrait::SUNLIGHT_VULNERABLE };

static bool isReverseTrait(MovementTrait trait) {
  return trait == MovementTrait::SUNLIGHT_VULNERABLE;
}

bool MovementType::canEnter(const MovementType& t) const {
  if (tribe && t.tribe && t.tribe != tribe)
    return false;
  if (!t.hasTrait(MovementTrait::BY_FORCE)) {
    for (auto trait : necessaryTraits)
      if (hasTrait(trait) && !t.hasTrait(trait))
        return false;
    for (auto trait : reverseTraits)
      if (t.hasTrait(trait) && !hasTrait(trait))
        return false;
  }
  for (auto trait : traits)
    if (t.hasTrait(trait) && !isReverseTrait(trait))
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

void MovementType::removeTrait(MovementTrait trait) {
  traits.erase(trait);
}

void MovementType::addTrait(MovementTrait trait) {
  traits.insert(trait);
}

