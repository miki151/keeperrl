#include "stdafx.h"
#include "clock.h"
#include "debug.h"

Clock::Clock() {
  initTime = steady_clock::now();
}

milliseconds Clock::getMillis() {
  if (lastPause)
    return duration_cast<milliseconds>(*lastPause - pausedTime);
  else
    return duration_cast<milliseconds>(getCurrent() - pausedTime);
}

void Clock::pause() {
  if (!lastPause)
    lastPause = getCurrent();
}

void Clock::cont() {
  if (lastPause) {
    pausedTime += getCurrent() - *lastPause;
    lastPause = none;
  }
}

bool Clock::isPaused() {
  return !!lastPause;
}

milliseconds Clock::getRealMillis() {
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
}

steady_clock::time_point Clock::getCurrent() {
  return steady_clock::time_point(steady_clock::now() - initTime);
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


ScopeTimer::ScopeTimer(const char* msg) : message(msg) {
}

ScopeTimer::~ScopeTimer() {
  INFO << " " << clock.getMillis() << message;
}
