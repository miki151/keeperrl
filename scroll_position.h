#pragma once

#include "stdafx.h"

class ScrollPosition {
  public:
  ScrollPosition();
  ScrollPosition(double pos);
  void set(double, milliseconds);
  void reset(double);
  void setBounds(double min, double max);
  void add(double, milliseconds);
  double get(milliseconds) const;
  void reset();

  private:
  double start = 0;
  double target = 0;
  milliseconds targetTime = milliseconds{0};
};
