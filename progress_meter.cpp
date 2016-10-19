#include "stdafx.h"
#include "progress_meter.h"
#include "util.h"

ProgressMeter::ProgressMeter(float inc) : progress(0), increase(inc) {
}

double ProgressMeter::getProgress() const {
  return max(0.0f, min(1.0f, progress * increase));
}

void ProgressMeter::addProgress(int num) {
  progress += num;
}

void ProgressMeter::reset() {
  progress = 0;
}

void ProgressMeter::setProgress(float p) {
  increase = 0.0001;
  progress = p / increase;
}
