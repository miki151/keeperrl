#pragma once

#include "util.h"
#include "view_id.h"
#include "t_string.h"

struct CollectiveName {
  optional<TString> SERIAL(shortened);
  TString SERIAL(full);
  TString SERIAL(race);
  ViewIdList SERIAL(viewId);
  optional<string> SERIAL(location);
  SERIALIZE_ALL(shortened, full, race, viewId, location)
};

