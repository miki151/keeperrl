#include "stdafx.h"
#include "roof_support.h"

RoofSupport::RoofSupport(Rectangle bounds) : numRectangles(bounds, 0), wall(bounds, false) {
}

SERIALIZE_DEF(RoofSupport, numRectangles, wall)

SERIALIZATION_CONSTRUCTOR_IMPL(RoofSupport)

constexpr int maxRoofSize = 10;

void RoofSupport::add(Vec2 pos) {
  if (!isWall(pos)) {
    modify(pos, 1);
    wall[pos] = true;
  }
}

void RoofSupport::remove(Vec2 pos) {
  if (isWall(pos)) {
    modify(pos, -1);
    wall[pos] = false;
  }
}

bool RoofSupport::isRoof(Vec2 pos) const {
  return numRectangles[pos] > 0 || wall[pos];
}

bool RoofSupport::isWall(Vec2 pos) const {
  return pos.inRectangle(wall.getBounds()) && wall[pos];
}

void RoofSupport::modify(Vec2 pos, int value) {
  //std::cout << "Pos " << pos << " " << value << std::endl;
  for (int x : Range(pos.x - maxRoofSize, pos.x + maxRoofSize + 1).intersection(wall.getBounds().getXRange()))
    if (x != pos.x && wall[Vec2(x, pos.y)])
      for (int y : Range(pos.y - maxRoofSize, pos.y + maxRoofSize + 1).intersection(wall.getBounds().getYRange()))
        if (y != pos.y && wall[Vec2(pos.x, y)] && wall[Vec2(x, y)]) {
          //std::cout << "Rect " << " " << pos << "" << Vec2(x, y) << std::endl;
          for (Vec2 v : Rectangle(min(pos.x, x), min(pos.y, y), max(pos.x, x) + 1, max(pos.y, y) + 1))
            numRectangles[v] += value;
        }
}
