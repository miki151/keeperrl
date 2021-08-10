#pragma once

#include "spell.h"
#include "game_time.h"
#include "unique_entity.h"

struct ItemAbility {
  Spell SERIAL(spell);
  optional<GlobalTime> SERIAL(timeout);
  // Keep the item id's generic ID as a way to detect that the item was 
  // retired and we need to reset the timeout.
  GenericId SERIAL(itemId);
  SERIALIZE_ALL(spell, timeout, itemId)
};
