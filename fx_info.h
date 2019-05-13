#pragma once

#include "util.h"
#include "position.h"
#include "color.h"

RICH_ENUM(FXStackId, debuff, buff, generic);

struct FXInfo {
  FXInfo(FXName name, Color color = Color::WHITE, float strength = 0.0, FXStackId sid = FXStackId::generic)
      : name(name), color(color), strength(strength), stackId(sid) { }

  bool empty() const {
    return name == FXName(0);
  }

  SERIALIZATION_CONSTRUCTOR(FXInfo)
  FXName SERIAL(name) = FXName(0);
  Color SERIAL(color) = Color::WHITE;
  float SERIAL(strength) = 0.0f;
  FXStackId SERIAL(stackId) = FXStackId::generic;
  SERIALIZE_ALL(NAMED(name), OPTION(color), OPTION(strength), OPTION(stackId))
};

FXInfo getFXInfo(FXVariantName);
optional<FXInfo> getOverlayFXInfo(ViewId);

struct FXSpawnInfo {
  FXSpawnInfo(FXInfo info, Vec2 pos, Vec2 offset = {})
      : position(pos), targetOffset(offset), info(info) { }

  Vec2 position;
  Vec2 targetOffset;
  FXInfo info;
};
