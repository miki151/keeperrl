#include "stdafx.h"
#include "clock.h"
#include "debug.h"

Clock::Clock(bool neverPause) : neverPause(neverPause) {
  initTime = steady_clock::now();
}

milliseconds Clock::getMillis() {
  if (lastPause)
    return duration_cast<milliseconds>(*lastPause - pausedTime);
  else
    return duration_cast<milliseconds>(getCurrent() - pausedTime);
}

void Clock::pause() {
  if (!lastPause && !neverPause)
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

microseconds Clock::getRealMicros() {
  return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
}

bool Clock::isChristmas() {
  time_t t = time(NULL);
  int day = localtime(&t)->tm_mday;
  int month = localtime(&t)->tm_mon + 1;
  return month == 12 && (day >= 19 && day <= 26);
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

void Intervalometer::clear() {
  lastUpdate = none;
}


ScopeTimer::ScopeTimer(const char* msg) : message(msg) {
}

ScopeTimer::~ScopeTimer() {
  INFO << " " << clock.getMillis() << message;
}
