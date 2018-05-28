#include "stdafx.h"
#include "collective_attack.h"
#include "collective.h"
#include "creature.h"
#include "collective_name.h"
#include "task.h"

SERIALIZE_DEF(CollectiveAttack, attacker, ransom, creatures, attackerName, attackTasks)
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveAttack);

static string generateAttackerName(WConstCollective attacker) {
  if (auto& name = attacker->getName())
    return name->full;
  else
    return "an unnamed attacker";
}

CollectiveAttack::CollectiveAttack(vector<WConstTask> attackTasks, WCollective att, const vector<WCreature>& c, optional<int> r)
    : ransom(r), creatures(c), attacker(att), attackerName(generateAttackerName(att)), attackTasks(attackTasks) {}

CollectiveAttack::CollectiveAttack(vector<WConstTask> attackTasks, const string& name, const vector<WCreature>& c)
    : creatures(c), attackerName(name), attackTasks(attackTasks) {
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

bool CollectiveAttack::isOngoing() const {
  for (auto& task : attackTasks)
    if (!!task)
      return true;
  return false;
}

bool CollectiveAttack::operator == (const CollectiveAttack& o) const {
  return attacker == o.attacker && ransom == o.ransom && creatures == o.creatures && attackerName == o.attackerName;
}
