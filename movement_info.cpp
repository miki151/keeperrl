#include "stdafx.h"
#include "movement_info.h"

MovementInfo::MovementInfo(Vec2 dir, LocalTime begin, LocalTime end, int moveCnt, MovementInfo::Type t)
    : direction(dir), tBegin(begin.getDouble()), tEnd(end.getDouble()), type(t), moveCounter(moveCnt) {}

MovementInfo::MovementInfo() {
}

MovementInfo& MovementInfo::setDirection(Vec2 v) {
  direction = v;
  return *this;
}

MovementInfo&MovementInfo::setType(MovementInfo::Type t) {
  type = t;
  return *this;
}

MovementInfo&MovementInfo::setMaxLength(TimeInterval i) {
  tEnd = min(tEnd, tBegin + i.getDouble());
  return *this;
}

MovementInfo& MovementInfo::setVictim(UniqueEntity<Creature>::Id id) {
  victim = id;
  return *this;
}
