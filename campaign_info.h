#pragma once

#include "stdafx.h"
#include "util.h"

struct CampaignInfo {
  Vec2 SERIAL(size);
  int SERIAL(maxMainVillains);
  int SERIAL(maxLesserVillains);
  int SERIAL(maxAllies);
  int SERIAL(influenceSize);
  SERIALIZE_ALL(NAMED(size), NAMED(maxMainVillains), NAMED(maxLesserVillains), NAMED(maxAllies), NAMED(influenceSize))
};
