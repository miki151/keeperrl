#pragma once

#include "stdafx.h"
#include "extern/optional.h"

class Clock {
  public:
  Clock(bool neverPause = false);
  milliseconds getMillis();
  void pause();
  void cont();
  bool isPaused();
  static milliseconds getRealMillis();
  static microseconds getRealMicros();
  static int getCurrentMonth();
  static int getCurrentDayOfMonth();

  private:
  steady_clock::time_point getCurrent();
  steady_clock::time_point pausedTime;
  optional<steady_clock::time_point> lastPause;
  steady_clock::time_point initTime;
  bool neverPause = false;
};

class ScopeTimer {
  public:
  ScopeTimer(const char* msg);
  ~ScopeTimer();

  private:
  const char* message;
  Clock clock;
};

class Intervalometer {
  public:
  Intervalometer(milliseconds frequency);
  int getCount(milliseconds currentTime);
  void clear();

  private:
  milliseconds frequency;
  optional<milliseconds> lastUpdate;
};

