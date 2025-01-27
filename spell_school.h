#pragma once

#include "stdafx.h"
#include "spell.h"
#include "attr_type.h"

struct SpellSchool {
  AttrType SERIAL(expType);
  optional<TString> SERIAL(name);
  vector<pair<SpellId, int>> SERIAL(spells);
  SERIALIZATION_DECL(SpellSchool)
  void serialize(PrettyInputArchive& ar1, const unsigned int);
};
