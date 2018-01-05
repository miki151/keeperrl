#pragma once

#include "stdafx.h"
#include "util.h"
#include "game_time.h"

struct MovementInfo {
  enum Type { MOVE, ATTACK };
  MovementInfo(Vec2 direction, LocalTime tBegin, LocalTime tEnd, int moveCounter, Type);
  MovementInfo();
  MovementInfo& setDirection(Vec2);
  MovementInfo& setType(Type);
  MovementInfo& setMaxLength(TimeInterval);
  Vec2 direction;
  double tBegin;
  double tEnd;
  Type type = MOVE;
  int moveCounter = 0;
};
