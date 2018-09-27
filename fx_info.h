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
FXInfo getFXInfo(WorkshopType);
optional<FXInfo> getOverlayFXInfo(ViewId);

optional<FXInfo> destroyFXInfo(FurnitureType);
optional<FXInfo> tryDestroyFXInfo(FurnitureType);
optional<FXInfo> walkIntoFXInfo(FurnitureType);
optional<FXInfo> walkOverFXInfo(FurnitureType);

struct FXSpawnInfo {
  FXSpawnInfo(FXInfo info, Position pos, GenericId gid, Vec2 offset = {})
      : position(pos), genericId(gid), targetOffset(offset), info(info) { }
  FXSpawnInfo(FXInfo info, Position pos, Vec2 offset = {})
      : position(pos), genericId(0), targetOffset(offset), info(info) { }

  Position position;
  GenericId genericId;
  Vec2 targetOffset;
  FXInfo info;
};
