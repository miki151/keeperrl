#include "stdafx.h"
#include "scroll_position.h"
#include "clock.h"

const static milliseconds scrollTime {100};

ScrollPosition::ScrollPosition() {
}

ScrollPosition::ScrollPosition(double pos) : start(pos), target(pos) {
}

void ScrollPosition::setRelative(double val, milliseconds currentTime) {
  if (knownBounds)
    set(target + val - knownBounds->min  - knownBounds->yBegin, currentTime);
  else
    set(val, currentTime);
}

void ScrollPosition::set(double val, milliseconds currentTime) {
  start = get(currentTime);
  targetTime = currentTime + scrollTime;
  target = val;
}

void ScrollPosition::setRatio(double val, milliseconds currentTime) {
  if (knownBounds) {
    val = max(0.0, min(1.0, val));
    start = get(currentTime);
    targetTime = milliseconds(0);//currentTime + scrollTime;
    target = knownBounds->min * (val - 1) + knownBounds->max * val;
  }
}

double ScrollPosition::getRatio(milliseconds currentTime) {
  if (knownBounds)
    return (get(currentTime) - knownBounds->min) / (knownBounds->max - knownBounds->min);
  return 0;
}

bool ScrollPosition::isScrolling(milliseconds currentTime) {
  return currentTime < targetTime;
}

void ScrollPosition::reset(double val) {
  start = target = val;
  targetTime = milliseconds{0};
}

void ScrollPosition::setBounds(double minV, double maxV, int yBegin) {
  start = max(minV, min(maxV, start));
  target = max(minV, min(maxV, target));
  knownBounds = KnownBounds {minV, maxV, yBegin};
}

void ScrollPosition::add(double val, milliseconds currentTime) {
  set(target + val, currentTime);
}

double ScrollPosition::get(milliseconds currentTime) {
  if (currentTime > targetTime)
    return target;
  else
    return target - (targetTime - currentTime).count() * (target - start) / scrollTime.count();
}

double ScrollPosition::get(milliseconds currentTime, double min, double max, int yBegin) {
  setBounds(min, max, yBegin);
  return get(currentTime);
}

void ScrollPosition::reset() {
  target = start = 0;
  targetTime = milliseconds{0};
}
