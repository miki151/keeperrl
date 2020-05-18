#pragma once

#include "stdafx.h"
#include "util.h"
#include "effect.h"
#include "view_id.h"
#include "item_prefix.h"

struct AutomatonPart {
  string SERIAL(minionGroup);
  Effect SERIAL(effect);
  optional<ViewId> SERIAL(installedId);
  ViewId SERIAL(viewId);
  string SERIAL(name);
  optional<int> SERIAL(layer);
  string SERIAL(automatonType);
  struct PrefixInfo {
    ItemPrefix SERIAL(prefix);
    string SERIAL(name);
    SERIALIZE_ALL(prefix, name)
  };
  vector<PrefixInfo> SERIAL(prefixes);
  optional<ItemClass> SERIAL(prefixType);
  bool isAvailable(const Creature*, int numAssigned = 0) const;
  template <class Archive>
  void serialize(Archive& ar1, const unsigned int);
};
