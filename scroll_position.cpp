#include "stdafx.h"
#include "scroll_position.h"
#include "clock.h"

const static milliseconds scrollTime {100};

ScrollPosition::ScrollPosition() {
}

ScrollPosition::ScrollPosition(double pos) : start(pos), target(pos) {
}

void ScrollPosition::set(double val, milliseconds currentTime) {
  start = get(currentTime);
  targetTime = currentTime + scrollTime;
  target = val;
}

void ScrollPosition::reset(double val) {
  start = target = val;
  targetTime = milliseconds{0};
}

void ScrollPosition::setBounds(double minV, double maxV) {
  start = max(minV, min(maxV, start));
  target = max(minV, min(maxV, target));
}

void ScrollPosition::add(double val, milliseconds currentTime) {
  set(target + val, currentTime);
}

double ScrollPosition::get(milliseconds currentTime) const {
  if (currentTime > targetTime)
    return target;
  else
    return target - (targetTime - currentTime).count() * (target - start) / scrollTime.count();
}

void ScrollPosition::reset() {
  target = start = 0;
  targetTime = milliseconds{0};
}
