#include "stdafx.h"
#include "quarters.h"
#include "view_id.h"


void Quarters::assign(optional<int> index, UniqueEntity<Creature>::Id id) {
  if (index)
    assignment.set(id, *index);
  else
    assignment.erase(id);
}

optional<int> Quarters::getAssigned(UniqueEntity<Creature>::Id id) const {
  return assignment.getMaybe(id);
}

vector<Quarters::QuartersInfo> Quarters::getAllQuarters() {
  static vector<Quarters::QuartersInfo> ret {
    { ZoneId::QUARTERS1, ViewId("quarters", Color(255, 20, 147)) },
    { ZoneId::QUARTERS2, ViewId("quarters", Color(0, 191, 255)) },
    { ZoneId::QUARTERS3, ViewId("quarters", Color(255, 165, 0)) },
  };
  return ret;
}

SERIALIZE_DEF(Quarters, assignment)
