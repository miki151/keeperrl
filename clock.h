#pragma once

class Clock {
  public:
  milliseconds getMillis();
  void pause();
  void cont();
  bool isPaused();
  milliseconds getRealMillis();

  private:
  steady_clock::time_point pausedTime;
  optional<steady_clock::time_point> lastPause;
};

class Intervalometer {
  public:
  Intervalometer(milliseconds frequency);
  int getCount(milliseconds currentTime);

  private:
  milliseconds frequency;
  optional<milliseconds> lastUpdate;
};

