#pragma once

#include "stdafx.h"
#include "view_id.h"
#include "t_string.h"

struct ScriptedHelpInfo {
  string SERIAL(scriptedId);
  optional<ViewId> SERIAL(viewId);
  optional<TString> SERIAL(title);
  SERIALIZE_ALL(NAMED(scriptedId), NAMED(viewId), NAMED(title))
};