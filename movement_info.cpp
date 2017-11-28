#include "stdafx.h"
#include "movement_info.h"

MovementInfo::MovementInfo(Vec2 dir, MovementInfo::TimeInfo info, LocalTime end, MovementInfo::Type t)
    : direction(dir), tBegin(info.begin), tEnd(end), type(t), moveCounter(info.moveCounter) {}

MovementInfo::MovementInfo() {
}
