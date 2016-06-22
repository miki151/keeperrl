#include "stdafx.h"
#include "clock.h"

using namespace std::chrono;

int Clock::getMillis() {
  if (lastPause)
    return duration_cast<milliseconds>(*lastPause - pausedTime).count();
  else
    return duration_cast<milliseconds>(steady_clock::now() - pausedTime).count();
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

int Clock::getRealMillis() {
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

Intervalometer::Intervalometer(int f) : frequency(f) {
}

int Intervalometer::getCount(int mill) {
  if (mill >= lastUpdate + frequency) {
    int diff = (mill - lastUpdate) / frequency;
    lastUpdate += diff * frequency;
    return diff;
  }
  return 0;
}

