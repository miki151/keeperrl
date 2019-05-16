#pragma once

#include "util.h"

RICH_ENUM(
    EffectAIIntent,
    WANTED,
    NONE,
    UNWANTED
);

inline EffectAIIntent reverse(EffectAIIntent e) {
  switch (e) {
    case EffectAIIntent::WANTED: return EffectAIIntent::UNWANTED;
    case EffectAIIntent::UNWANTED: return EffectAIIntent::WANTED;
    case EffectAIIntent::NONE: return e;
  }
}
