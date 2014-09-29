#include "stdafx.h"
#include "clock.h"

#include <SFML/Graphics.hpp>

static sf::Clock sfClock;

int Clock::getMillis() {
  if (lastPause > -1)
    return lastPause - pausedTime;
  else
    return sfClock.getElapsedTime().asMilliseconds() - pausedTime;
}

void Clock::setMillis(int time) {
  if (lastPause > -1)
    pausedTime = lastPause - time;
  else
    pausedTime = sfClock.getElapsedTime().asMilliseconds() - time;
}

void Clock::pause() {
  if (lastPause < 0)
    lastPause = sfClock.getElapsedTime().asMilliseconds();
}

void Clock::cont() {
  if (lastPause > -1) {
    pausedTime += sfClock.getElapsedTime().asMilliseconds() - lastPause;
    lastPause = -1;
  }
}

bool Clock::isPaused() {
  return lastPause > -1;
}

int Clock::getRealMillis() {
  return sfClock.getElapsedTime().asMilliseconds();
}

Intervalometer::Intervalometer(int f) : frequency(f) {
}

int Intervalometer::getCount() {
  int mill = Clock::get().getMillis();
  if (mill >= lastUpdate + frequency) {
    int diff = (mill - lastUpdate) / frequency;
    lastUpdate += diff * frequency;
    return diff;
  }
  return 0;
}

