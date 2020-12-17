#include "stdafx.h"
#include "movement_info.h"

MovementInfo::MovementInfo(Vec2 dir, LocalTime begin, LocalTime end, int moveCnt, MovementInfo::Type t)
    : dirX(dir.x), dirY(dir.y), tBegin(begin.getDouble()), tEnd(end.getDouble()), type(t), moveCounter(moveCnt) {}

MovementInfo::MovementInfo() {
}

MovementInfo& MovementInfo::setDirection(Vec2 v) {
  dirX = v.x;
  dirY = v.y;
  return *this;
}

MovementInfo& MovementInfo::setType(MovementInfo::Type t) {
  type = t;
  return *this;
}

MovementInfo& MovementInfo::setMaxLength(TimeInterval i) {
  tEnd = min<float>(tEnd, tBegin + i.getDouble());
  return *this;
}

MovementInfo& MovementInfo::setVictim(UniqueEntity<Creature>::Id id) {
  victim = id.getGenericId();
  return *this;
}

MovementInfo& MovementInfo::setFX(optional<FXVariantName> v) {
  fx = v;
  return *this;
}

Vec2 MovementInfo::getDir() const {
  return Vec2(dirX, dirY);
}
