#include "minion_trait.h"

extern const char* /*can be null*/ getImmigrantDescription(MinionTrait trait) {
  switch (trait) {
    case MinionTrait::DOESNT_TRIGGER:
      return "Does not trigger enemies when it attacks";
    default:
      return nullptr;
  }
}
