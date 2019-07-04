#include "minion_trait.h"

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
