#pragma once

#include "stdafx.h"
#include "spell.h"

struct SpellSchool {
  ExperienceType SERIAL(expType);
  vector<pair<SpellId, int>> SERIAL(spells);
  SERIALIZE_ALL(expType, spells)
};
