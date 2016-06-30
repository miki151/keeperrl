#ifndef _CLOCK_H
#define _CLOCK_H

class Clock {
  public:
  int getMillis();
  void pause();
  void cont();
  bool isPaused();
  int getRealMillis();

  private:
  std::chrono::steady_clock::time_point pausedTime;
  optional<std::chrono::steady_clock::time_point> lastPause;
};

class Intervalometer {
  public:
  Intervalometer(int frequencyMillis);
  int getCount(int currentTimeMillis);

  private:
  int frequency;
  int lastUpdate = 0;
};

#endif
