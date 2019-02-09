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

  FXName name = FXName(0);
  Color color = Color::WHITE;
  float strength = 0.0f;
  FXStackId stackId = FXStackId::generic;
};

FXInfo getFXInfo(FXVariantName);
optional<FXInfo> getOverlayFXInfo(ViewId);

optional<FXInfo> destroyFXInfo(FurnitureType);
optional<FXInfo> tryDestroyFXInfo(FurnitureType);
optional<FXInfo> walkIntoFXInfo(FurnitureType);
optional<FXInfo> walkOverFXInfo(FurnitureType);
optional<FXVariantName> getFurnitureUsageFX(FurnitureType);

struct FXSpawnInfo {
  FXSpawnInfo(FXInfo info, Vec2 pos, Vec2 offset = {})
      : position(pos), targetOffset(offset), info(info) { }

  Vec2 position;
  Vec2 targetOffset;
  FXInfo info;
};
