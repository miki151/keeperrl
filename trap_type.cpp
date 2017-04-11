#include "trap_type.h"
#include "effect_type.h"
#include "lasting_effect.h"
#include "effect.h"
#include "view_id.h"

string getTrapName(TrapType type) {
  switch (type) {
    case TrapType::ALARM:
      return "alarm";
    case TrapType::BOULDER:
      return "boulder";
    case TrapType::POISON_GAS:
      return "poison gas";
    case TrapType::SURPRISE:
      return "surprise";
    case TrapType::TERROR:
      return "panic";
    case TrapType::WEB:
      return "web";
  }
}
