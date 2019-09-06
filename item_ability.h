#pragma once

#include "spell.h"
#include "game_time.h"

struct ItemAbility {
  Spell SERIAL(spell);
  optional<GlobalTime> SERIAL(timeout);
  SERIALIZE_ALL(spell, timeout)
};
