#pragma once

#include "stdafx.h"

class Clock {
  public:
  Clock();
  milliseconds getMillis();
  void pause();
  void cont();
  bool isPaused();
  static milliseconds getRealMillis();

  private:
  steady_clock::time_point getCurrent();
  steady_clock::time_point pausedTime;
  optional<steady_clock::time_point> lastPause;
  steady_clock::time_point initTime;
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

  private:
  milliseconds frequency;
  optional<milliseconds> lastUpdate;
};

