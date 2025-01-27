#pragma once

#include "util.h"

RICH_ENUM(SunlightState,
  DAY,
  NIGHT
);

#include "game_time.h"

class TStringId;

class SunlightInfo {
  public:
  TStringId getText() const;
  static TStringId getText(SunlightState);
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

