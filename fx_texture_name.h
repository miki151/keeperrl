#pragma once

#include "util.h"

namespace fx {
enum class TextureName;
}

RICH_ENUM(fx::TextureName,
  CIRCULAR,
  CIRCULAR_STRONG,
  FLAKES_BORDERS,
  WATER_DROPS,
  CLOUDS_SOFT,
  CLOUDS_SOFT_BORDERS,
  CLOUDS_ADD,
  TORUS,
  TORUS_BOTTOM,
  TORUS_BOTTOM_BLURRED,
  TELEPORT,
  TELEPORT_BLEND,
  BEAMS,
  SPARKS,
  SPARKS_LIGHT,
  MISSILE_CORE,
  FLAMES,
  FLAMES_BLURRED,
  BLAST,
  SPECIAL
);
