#pragma once

#include "util.h"
#include "clock.h"

class FpsCounter {
  public:
  FpsCounter(int numLatencyFrames);
  void addTick();
  int getFps();
  int getMaxLatency();

  private:
  int lastFps = 0;
  int curSec = -1;
  int curFps = 0;
  Clock clock;
  const int numLatencyFrames;
  multiset<milliseconds> latencies;
  queue<milliseconds> latencyQueue;
  optional<milliseconds> lastUpdate;
  void updateLatencies(milliseconds);
};

