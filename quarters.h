#pragma once

#include "stdafx.h"
#include "util.h"
#include "entity_map.h"
#include "zones.h"
#include "unique_entity.h"
#include "view_id.h"

class Quarters {
  public:
  void assign(optional<int> index, UniqueEntity<Creature>::Id);
  optional<int> getAssigned(UniqueEntity<Creature>::Id) const;
  struct QuartersInfo {
    ZoneId zone;
    ViewId viewId;
  };
  static const vector<QuartersInfo>& getAllQuarters();
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EntityMap<Creature, int> SERIAL(assignment);
};
