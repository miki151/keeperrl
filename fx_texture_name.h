#pragma once

#include "util.h"

namespace fx {
enum class TextureName;
}

RICH_ENUM(fx::TextureName,
  CIRCULAR,
  CIRCULAR_STRONG,
  FLAKES_BORDERS,
  CLOUDS_SOFT,
  CLOUDS_SOFT_BORDERS,
  CLOUDS_ADD,
  TORUS,
  TORUS_BOTTOM,
  TORUS_BOTTOM_BLURRED,
  MAGIC_MISSILE,
  FLAMES,
  FLAMES_BLURRED,
  SPECIAL
);
