#pragma once

#include "util.h"

RICH_ENUM(SunlightState,
  DAY,
  NIGHT
);

#include "game_time.h"

class SunlightInfo {
  public:
  const char* getText() const;
  static const char* getText(SunlightState);
  void update(GlobalTime currentTime);
  SunlightState getState() const;
  double getLightAmount() const;
  TimeInterval getTimeRemaining() const;
  TimeInterval getTimeSinceDawn() const;

  private:
  double lightAmount;
  TimeInterval timeRemaining;
  SunlightState state;
};

