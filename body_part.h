#pragma once

#include "util.h"

RICH_ENUM(BodyPart,
  HEAD,
  TORSO,
  ARM,
  WING,
  LEG,
  BACK
);

class TString;
extern TString getName(BodyPart);
extern TString getPlural(BodyPart);
extern TString getPluralText(BodyPart, int num);
