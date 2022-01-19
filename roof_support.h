#pragma once

#include "util.h"

class RoofSupport {
  public:
  RoofSupport(Rectangle bounds);
  void add(Vec2);
  void remove(Vec2);
  bool isRoof(Vec2) const;
  bool isWall(Vec2) const;

  SERIALIZATION_DECL(RoofSupport)

  private:
  Table<int> SERIAL(numRectangles);
  Table<int> SERIAL(wall);
  void modify(Vec2, int);
};
