#pragma once

#include "stdafx.h"
#include "util.h"
#include "view_id.h"
#include "fx_info.h"
#include "effect.h"
#include "t_string.h"

struct AttrInfo {
  TString SERIAL(name);
  ViewId SERIAL(viewId);
  TString SERIAL(adjective);
  int SERIAL(absorptionCap);
  int SERIAL(modifierVariation) = 1;
  int SERIAL(wishedItemIncrease) = 5;
  bool SERIAL(isAttackAttr) = false;
  optional<FXInfo> SERIAL(meleeFX);
  TString SERIAL(help);
  optional<TString> SERIAL(bodyPartInjury);
  optional<Effect> SERIAL(onAttackedEffect);
  SERIALIZE_ALL(NAMED(bodyPartInjury), NAMED(name), NAMED(viewId), NAMED(adjective), NAMED(absorptionCap), OPTION(modifierVariation), OPTION(wishedItemIncrease), OPTION(isAttackAttr), NAMED(meleeFX), NAMED(help), NAMED(onAttackedEffect))
};