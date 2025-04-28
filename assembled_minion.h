#pragma once

#include "util.h"
#include "minion_trait.h"
#include "effect.h"
#include "creature_id.h"

struct AssembledMinion {
  void assemble(Collective*, Item*, Position) const;
  CreatureId SERIAL(creature);
  EnumSet<MinionTrait> SERIAL(traits);
  vector<Effect> SERIAL(effects);
  double SERIAL(scale) = 1.0;
  SERIALIZE_ALL(NAMED(creature), NAMED(traits), OPTION(effects), SKIP(scale))
};
