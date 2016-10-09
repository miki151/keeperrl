#include "stdafx.h"
#include "clock.h"

milliseconds Clock::getMillis() {
  if (lastPause)
    return duration_cast<milliseconds>(*lastPause - pausedTime);
  else
    return duration_cast<milliseconds>(steady_clock::now() - pausedTime);
}

void Clock::pause() {
  if (!lastPause)
    lastPause = steady_clock::now();
}

void Clock::cont() {
  if (lastPause) {
    pausedTime += steady_clock::now() - *lastPause;
    lastPause = none;
  }
}

bool Clock::isPaused() {
  return !!lastPause;
}

milliseconds Clock::getRealMillis() {
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
}

Intervalometer::Intervalometer(milliseconds f) : frequency(f) {
}

int Intervalometer::getCount(milliseconds mill) {
  if (!lastUpdate)
    lastUpdate = mill - frequency;
  if (mill >= *lastUpdate + frequency) {
    int diff = (mill - *lastUpdate) / frequency;
    *lastUpdate += diff * frequency;
    return diff;
  }
  return 0;
}

