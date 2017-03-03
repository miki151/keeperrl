#include "stdafx.h"
#include "fps_counter.h"

FpsCounter::FpsCounter(int n) : numLatencyFrames(n) {
}

void FpsCounter::addTick() {
  auto curTime = clock.getMillis();
  updateLatencies(curTime);
  int thisSec = curTime.count() / 1000;
  if (thisSec == curSec)
    ++curFps;
  else {
    lastFps = curFps;
    curSec = thisSec;
    curFps = 0;
  }
}

void FpsCounter::updateLatencies(milliseconds curTime) {
  if (lastUpdate) {
    auto thisLatency = curTime - *lastUpdate;
    latencyQueue.push(thisLatency);
    latencies.insert(thisLatency);
    if (latencyQueue.size() > numLatencyFrames) {
      latencies.erase(latencies.find(latencyQueue.front()));
      latencyQueue.pop();
    }
  }
  lastUpdate = curTime;
}

int FpsCounter::getFps() {
  return lastFps;
}

int FpsCounter::getMaxLatency() {
  return (--latencies.end())->count();
}
