#include "stdafx.h"
#include "collective_attack.h"
#include "collective.h"
#include "creature.h"
#include "collective_name.h"
#include "task.h"
#include "task_map.h"
#include "view_object.h"

SERIALIZE_DEF(CollectiveAttack, attacker, ransom, creatures, attackerName, attackTasks, attackerViewId)
SERIALIZATION_CONSTRUCTOR_IMPL(CollectiveAttack);

static string generateAttackerName(WConstCollective attacker) {
  if (auto& name = attacker->getName())
    return name->full;
  else
    return "an unnamed attacker";
}

CollectiveAttack::CollectiveAttack(vector<WConstTask> attackTasks, WCollective att, const vector<Creature*>& c, optional<int> r)
    : ransom(r), creatures(c), attacker(att), attackerName(generateAttackerName(att)),
      attackerViewId(att->getLeaderOrOtherMinion()->getViewObject().id()),
      attackTasks(attackTasks.transform([](auto elem) { return elem->getThis(); })) {}

CollectiveAttack::CollectiveAttack(vector<WConstTask> attackTasks, const string& name, ViewId id, const vector<Creature*>& c)
    : creatures(c), attackerName(name), attackerViewId(id),
      attackTasks(attackTasks.transform([](auto elem) { return elem->getThis(); })) {}


WCollective CollectiveAttack::getAttacker() const {
  return attacker;
}

const string& CollectiveAttack::getAttackerName() const {
  return attackerName;
}

ViewId CollectiveAttack::getAttackerViewId() const {
  return attackerViewId;
}

const vector<Creature*>& CollectiveAttack::getCreatures() const {
  return creatures;
}

optional<int> CollectiveAttack::getRansom() const {
  return ransom;
}

bool CollectiveAttack::isOngoing() const {
  for (auto& task : attackTasks)
    if (!!task && attacker->getTaskMap().getOwner(task.get()))
      return true;
  return false;
}

bool CollectiveAttack::operator == (const CollectiveAttack& o) const {
  return attacker == o.attacker && ransom == o.ransom && creatures == o.creatures && attackerName == o.attackerName;
}
