#pragma once

#include "util.h"
#include "position.h"
#include "color.h"
#include "fx_name.h"

RICH_ENUM(FXStackId, debuff, buff, generic);

struct FXInfo {
  FXInfo(FXName, Color = Color::WHITE, float strength = 0.0, FXStackId = FXStackId::generic);

  bool empty() const;

  FXInfo& setReversed();

  SERIALIZATION_CONSTRUCTOR(FXInfo)
  FXName SERIAL(name) = FXName(0);
  Color SERIAL(color) = Color::WHITE;
  float SERIAL(strength) = 0.0f;
  FXStackId SERIAL(stackId) = FXStackId::generic;
  bool SERIAL(reversed) = false;
  SERIALIZE_ALL(NAMED(name), OPTION(color), OPTION(strength), OPTION(stackId), OPTION(reversed))
};

FXInfo getFXInfo(FXVariantName);

struct FXSpawnInfo {
  FXSpawnInfo(FXInfo, Vec2 pos, Vec2 offset = {});

  Vec2 position;
  Vec2 targetOffset;
  FXInfo info;
};
