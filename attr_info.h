#pragma once

#include "stdafx.h"
#include "util.h"
#include "view_id.h"
#include "fx_info.h"

struct AttrInfo {
  string SERIAL(name);
  ViewId SERIAL(viewId);
  string SERIAL(adjective);
  int SERIAL(absorptionCap);
  int SERIAL(modifierVariation) = 1;
  int SERIAL(wishedItemIncrease) = 5;
  bool SERIAL(isAttackAttr) = false;
  optional<FXInfo> SERIAL(meleeFX);
  string SERIAL(help);
  optional<string> SERIAL(bodyPartInjury);
  SERIALIZE_ALL(NAMED(bodyPartInjury), NAMED(name), NAMED(viewId), NAMED(adjective), NAMED(absorptionCap), OPTION(modifierVariation), OPTION(wishedItemIncrease), OPTION(isAttackAttr), NAMED(meleeFX), NAMED(help))
};