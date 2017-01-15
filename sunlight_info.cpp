#include "stdafx.h"
#include "sunlight_info.h"


const char* SunlightInfo::getText() const {
  return getText(state);
}

const char* SunlightInfo::getText(SunlightState state) {
  switch (state) {
    case SunlightState::NIGHT: return "night";
    case SunlightState::DAY: return "day";
  }
}

const double dayLength = 1500;
const double nightLength = 1500;
const double duskLength  = 180;

void SunlightInfo::update(double currentTime) {
  double d = 0;
  while (1) {
    d += dayLength;
    if (d > currentTime) {
      lightAmount = 1;
      timeRemaining = d - currentTime;
      state = SunlightState::DAY;
      break;
    }
    d += duskLength;
    if (d > currentTime) {
      lightAmount = (d - currentTime) / duskLength;
      timeRemaining = d + nightLength - duskLength - currentTime;
      state = SunlightState::NIGHT;
      break;
    }
    d += nightLength - 2 * duskLength;
    if (d > currentTime) {
      lightAmount = 0;
      timeRemaining = d + duskLength - currentTime;
      state = SunlightState::NIGHT;
      break;
    }
    d += duskLength;
    if (d > currentTime) {
      lightAmount = 1 - (d - currentTime) / duskLength;
      timeRemaining = d - currentTime;
      state = SunlightState::NIGHT;
      break;
    }
  }
}

SunlightState SunlightInfo::getState() const {
  return state;
}

double SunlightInfo::getLightAmount() const {
  return lightAmount;
}

double SunlightInfo::getTimeRemaining() const {
  return timeRemaining;
}

