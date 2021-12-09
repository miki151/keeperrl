#pragma once

#include "stdafx.h"
#include "util.h"
#include "effect.h"
#include "view_id.h"
#include "item_prefix.h"

struct AutomatonPart {
  string SERIAL(minionGroup);
  int SERIAL(layer);
  ViewId SERIAL(installedId);
  SERIALIZE_ALL(layer, minionGroup, installedId)
};
