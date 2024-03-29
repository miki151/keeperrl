#pragma once

#include "stdafx.h"
#include "util.h"
#include <chrono>

class ScrollPosition {
  public:
  ScrollPosition();
  explicit ScrollPosition(double pos);
  void set(double, milliseconds);
  void setRelative(double, milliseconds);
  void setRatio(double, milliseconds);
  void reset(double);
  void setBounds(double min, double max, int yBegin);
  void add(double, milliseconds);
  double get(milliseconds, double min, double max, int yBegin);
  double get(milliseconds);
  double getRatio(milliseconds);
  bool isScrolling(milliseconds);
  void reset();

  private:
  double start = 0;
  double target = 0;
  milliseconds targetTime = milliseconds{0};
  struct KnownBounds {
    double min;
    double max;
    int yBegin;
  };
  optional<KnownBounds> knownBounds;
};
