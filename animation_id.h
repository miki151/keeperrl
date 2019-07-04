#pragma once

#include "util.h"

RICH_ENUM(AnimationId,
  DEATH,
  ATTACK
);

extern milliseconds getDuration(AnimationId);
extern const char* getFileName(AnimationId);
extern int getNumFrames(AnimationId);
