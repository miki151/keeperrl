#include "minion_trait.h"
#include "t_string.h"

optional<TStringId> getImmigrantDescription(MinionTrait trait) {
  switch (trait) {
    case MinionTrait::DOESNT_TRIGGER:
      return TStringId("DOES_NOT_TRIGGER_ENEMIES");
    case MinionTrait::INCREASE_POPULATION:
      return TStringId("WILL_BE_REPLACED");
    default:
      return none;
  }
}
