#include "stdafx.h"
#include "timer_var.h"
#include "serialization.h"

template <class Archive> 
void TimerVar::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(timeout);
}

SERIALIZABLE(TimerVar);

void TimerVar::set(double t) {
  timeout = t;
}

void TimerVar::unset() {
  timeout = -1;
}

bool TimerVar::isFinished(double currentTime) {
  if (timeout >= 0 && currentTime >= timeout) {
    timeout = -1;
    return true;
  }
  return false;
}

TimerVar::operator bool() const {
  return timeout >= 0;
}

