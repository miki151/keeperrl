#include "stdafx.h"
#include "progress.h"

static atomic<bool> interrupted { false };

void Progress::interrupt() {
  interrupted = true;
}

void Progress::checkIfInterrupted() {
  if (interrupted.exchange(false))
    throw InterruptedException();
}
