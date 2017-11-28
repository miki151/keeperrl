#pragma once

#include "stdafx.h"
#include "util.h"
#include "game_time.h"

struct MovementInfo {
  enum Type { MOVE, ATTACK };
  struct TimeInfo {
    LocalTime begin;
    int moveCounter = 0;
  };
  MovementInfo(Vec2 direction, TimeInfo, LocalTime tEnd, Type);
  MovementInfo();
  Vec2 direction;
  LocalTime tBegin;
  LocalTime tEnd;
  Type type = MOVE;
  int moveCounter = 0;
};
