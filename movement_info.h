#pragma once

#include "stdafx.h"
#include "util.h"
#include "game_time.h"
#include "unique_entity.h"

struct MovementInfo {
  enum Type { MOVE, ATTACK, WORK };
  MovementInfo(Vec2 direction, LocalTime tBegin, LocalTime tEnd, int moveCounter, Type);
  MovementInfo();
  MovementInfo& setDirection(Vec2);
  MovementInfo& setType(Type);
  MovementInfo& setMaxLength(TimeInterval);
  MovementInfo& setVictim(UniqueEntity<Creature>::Id);
  MovementInfo& setFX(optional<FXVariantName>);
  Vec2 direction;
  double tBegin;
  double tEnd;
  Type type = MOVE;
  int moveCounter = 0;
  optional<UniqueEntity<Creature>::Id> victim;
  optional<FXVariantName> fx;
};
