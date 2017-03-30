#include "stdafx.h"
#include "collective_attack.h"
#include "collective.h"
#include "creature.h"
#include "collective_name.h"

SERIALIZE_DEF(CollectiveAttack, attacker, ransom, creatures, attackerName)
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveAttack);

CollectiveAttack::CollectiveAttack(WCollective att, const vector<WCreature>& c, optional<int> r)
  : ransom(r), creatures(c), attacker(att), attackerName(attacker->getName().getFull()) {}

CollectiveAttack::CollectiveAttack(const string& name, const vector<WCreature>& c) : creatures(c), attackerName(name) {

}


WCollective CollectiveAttack::getAttacker() const {
  return attacker;
}

const string& CollectiveAttack::getAttackerName() const {
  return attackerName;
}

const vector<WCreature>& CollectiveAttack::getCreatures() const {
  return creatures;
}

optional<int> CollectiveAttack::getRansom() const {
  return ransom;
}

bool CollectiveAttack::operator == (const CollectiveAttack& o) const {
  return attacker == o.attacker && ransom == o.ransom && creatures == o.creatures && attackerName == o.attackerName;
}
