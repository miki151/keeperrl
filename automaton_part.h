#pragma once

#include "stdafx.h"
#include "util.h"
#include "effect.h"

struct AutomatonPart {
  AutomatonSlot SERIAL(slot);
  vector<Effect> SERIAL(effect);
  bool isAvailable(const Creature*) const;
  void apply(Creature*) const;
  template <class Archive>
  void serialize(Archive& ar1, const unsigned int);
};
