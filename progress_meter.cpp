#include "stdafx.h"
#include "progress_meter.h"
#include "util.h"

ProgressMeter::ProgressMeter(double inc) : progress(0), increase(inc) {
}

double ProgressMeter::getProgress() const {
  return max(0.0, min(1.0, progress * increase));
}

void ProgressMeter::addProgress() {
  ++progress;
}

void ProgressMeter::reset() {
  progress = 0;
}

void ProgressMeter::setProgress(double p) {
  increase = 0.0001;
  progress = p / increase;
}
