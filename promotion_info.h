#pragma once

#include "effect.h"
#include "view_id.h"
#include "t_string.h"

struct PromotionInfo {
  Effect SERIAL(applied);
  TString SERIAL(name);
  ViewId SERIAL(viewId);
  optional<TStringId> SERIAL(message);
  template <typename Archive>
  void serialize(Archive&, unsigned int);
};

CEREAL_CLASS_VERSION(PromotionInfo, 1);