#include "minion_trait.h"

const string getName(const MinionTrait trait) {
  switch(trait) {
    case MinionTrait::LEADER:
      return "leader";
    case MinionTrait::FIGHTER:
      return "fighter";
    case MinionTrait::PRISONER:
      return "prisoner";
    case MinionTrait::NO_EQUIPMENT:
      return "no equipment";
    case MinionTrait::NO_LIMIT:
      return "no limit";
    case MinionTrait::FARM_ANIMAL:
      return "farm animal";
    case MinionTrait::SUMMONED:
      return "summoned";
    case MinionTrait::NO_AUTO_EQUIPMENT:
      return "no autoequipment";
    case MinionTrait::DOESNT_TRIGGER:
      return "doesn't trigger";
    case MinionTrait::INCREASE_POPULATION:
      return "increase population";
    case MinionTrait::AUTOMATON:
      return "automaton";
  }
  return "";
}

const string getTraitName(const MinionTrait trait) {
  return getName(trait);
}

extern const char* /*can be null*/ getImmigrantDescription(MinionTrait trait) {
  switch (trait) {
    case MinionTrait::DOESNT_TRIGGER:
      return "Does not trigger enemies when it attacks";
    case MinionTrait::INCREASE_POPULATION:
      return "Increases population limit by 1";
    default:
      return nullptr;
  }
}
