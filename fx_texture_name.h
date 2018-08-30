#pragma once

#include "util.h"

namespace fx {
enum class TextureName;
}

RICH_ENUM(fx::TextureName,
  CIRCULAR,
  FLAKES_BORDERS,
  CLOUDS_SOFT,
  CLOUDS_SOFT_BORDERS,
  CLOUDS_ADD,
  TORUS,
  MAGIC_MISSILE,
  FLAMES,
  FLAMES_BLURRED,
  SPECIAL
);
