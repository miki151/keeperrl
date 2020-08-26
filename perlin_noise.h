#pragma once

#include "util.h"


struct NoiseInit {
  int topLeft;
  int topRight;
  int bottomRight;
  int bottomLeft;
  int middle;
};

Table<double> genNoiseMap(RandomGen& random, Rectangle area, NoiseInit, double varianceMult);
