#pragma once

#include "lasting_effect.h"
#include "attr_type.h"
#include "effect.h"
#include "msg_type.h"
#include "fx_variant_name.h"

struct BuffInfo {
  string SERIAL(name);
  optional<pair<MsgType, string>> SERIAL(addedMessage);
  optional<pair<MsgType, string>> SERIAL(removedMessage);
  optional<Effect> SERIAL(startEffect);
  optional<Effect> SERIAL(tickEffect);
  optional<Effect> SERIAL(endEffect);
  string SERIAL(description);
  string SERIAL(adjective);
  bool SERIAL(consideredBad) = false;
  bool SERIAL(combatConsumable) = false;
  bool SERIAL(stacks) = false;
  int SERIAL(price) = 50;
  Color SERIAL(color);
  optional<string> SERIAL(hatedGroupName);
  double SERIAL(defenseMultiplier) = 1.0;
  optional<AttrType> SERIAL(defenseMultiplierAttr);
  FXVariantName SERIAL(fx) = FXVariantName::BUFF_RED;
  SERIALIZE_ALL(OPTION(combatConsumable), OPTION(fx), OPTION(defenseMultiplier), OPTION(defenseMultiplierAttr), NAMED(hatedGroupName), NAMED(name), OPTION(addedMessage), OPTION(removedMessage), NAMED(startEffect), NAMED(tickEffect), NAMED(endEffect), OPTION(stacks), OPTION(consideredBad), NAMED(description), OPTION(price), NAMED(color), NAMED(adjective))
};