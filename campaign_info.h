#pragma once

#include "stdafx.h"
#include "util.h"

struct CampaignInfo {
  Vec2 SERIAL(size);
  int SERIAL(maxMainVillains);
  int SERIAL(maxLesserVillains);
  int SERIAL(maxAllies);
  int SERIAL(initialRadius);
  int SERIAL(mapZoom) = 2;
  int SERIAL(minimapZoom) = 2;
  SERIALIZE_ALL(NAMED(size), NAMED(maxMainVillains), NAMED(maxLesserVillains), NAMED(maxAllies), NAMED(initialRadius), OPTION(mapZoom), OPTION(minimapZoom))
};
