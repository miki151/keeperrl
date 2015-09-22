#include "stdafx.h"
#include "collective_attack.h"
#include "collective.h"
#include "creature.h"

template <class Archive>
void CollectiveAttack::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(attacker) & SVAR(ransom) & SVAR(creatures);
}

SERIALIZABLE(CollectiveAttack);
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveAttack);

CollectiveAttack::CollectiveAttack(Collective* att, const vector<Creature*>& c, optional<int> r)
    : ransom(r), creatures(c), attacker(att) {}


Collective* CollectiveAttack::getAttacker() const {
  return attacker;
}

const vector<Creature*>& CollectiveAttack::getCreatures() const {
  return creatures;
}

optional<int> CollectiveAttack::getRansom() const {
  return ransom;
}

bool CollectiveAttack::operator == (const CollectiveAttack& o) const {
  return attacker == o.attacker && ransom == o.ransom && creatures == o.creatures;
}
