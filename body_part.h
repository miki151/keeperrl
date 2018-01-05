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

extern const char* getName(BodyPart);
