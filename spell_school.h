#pragma once

#include "stdafx.h"
#include "spell.h"

struct SpellSchool {
  vector<pair<string, int>> SERIAL(spells);
  SERIALIZE_ALL(spells)
};
