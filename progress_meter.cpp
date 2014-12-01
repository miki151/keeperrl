#include "stdafx.h"
#include "progress_meter.h"
#include "util.h"

ProgressMeter::ProgressMeter(double inc) : progress(0), increase(inc) {
}

double ProgressMeter::getProgress() const {
  int p = progress;
  Debug() << "Progress " << p;
  return min(1.0, progress * increase);
}

void ProgressMeter::addProgress() {
  ++progress;
}

