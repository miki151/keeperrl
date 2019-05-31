#include "stdafx.h"
#include "draw_line.h"
#include "position.h"

static vector<int> drawNormalized(int dx, int dy) {
  CHECK(dx >= 0);
  CHECK(dy >= 0);
  CHECK(dx >= dy);
  int d = 2 * dy - dx;
  int y = 0;
  vector<int> ret;
  for (int x : Range(dx + 1)) {
    ret.push_back(y);
    if (d > 0) {
      ++y;
      d = d - 2 * dx;
    }
    d = d + 2 * dy;
  }
  return ret;
}

vector<Vec2> drawLine(Vec2 from, Vec2 to) {
  Vec2 dir = to - from;
  auto normalized = drawNormalized(max(abs(dir.x), abs(dir.y)), min(abs(dir.x), abs(dir.y)));
  vector<Vec2> ret;
  for (int i : All(normalized)) {
    Vec2 diff(i, normalized[i]);
    if (abs(dir.x) < abs(dir.y)) {
      swap(diff.x, diff.y);
    }
    if (dir.y < 0)
      diff.y = -diff.y;
    if (dir.x < 0)
      diff.x = -diff.x;
    ret.push_back(from + diff);
  }
  return ret;
}

vector<Position> drawLine(Position from, Position to) {
  CHECK(from.isSameLevel(to));
  return drawLine(from.getCoord(), to.getCoord()).transform([l = to.getLevel()](Vec2 v) { return Position(v, l); });
}
