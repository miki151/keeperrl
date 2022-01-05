#pragma once

#include "stdafx.h"
#include "view_id.h"

struct ScriptedHelpInfo {
  string SERIAL(scriptedId);
  optional<ViewId> SERIAL(viewId);
  optional<string> SERIAL(title);
  SERIALIZE_ALL(NAMED(scriptedId), NAMED(viewId), NAMED(title))
};