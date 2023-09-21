#pragma once

#include "stdafx.h"
#include "spell.h"

struct SpellSchool {
  ExperienceType SERIAL(expType);
  optional<string> SERIAL(name);
  vector<pair<SpellId, int>> SERIAL(spells);
  SERIALIZATION_DECL(SpellSchool)
  void serialize(PrettyInputArchive& ar1, const unsigned int);
};
