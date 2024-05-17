#pragma once

#include "stdafx.h"
#include "util.h"
#include "tribe.h"
#include "random_layout_id.h"

struct KeeperBaseInfo {
  optional<RandomLayoutId> SERIAL(layout);
  Vec2 SERIAL(size);
  bool SERIAL(insideMountain) = false;

  template <typename Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar(size, layout);
    if (version == 1)
      ar(insideMountain);
  }

  void serialize(PrettyInputArchive&, unsigned int);
};

CEREAL_CLASS_VERSION(KeeperBaseInfo, 1)

