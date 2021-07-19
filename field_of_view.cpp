/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "field_of_view.h"
#include "square.h"
#include "square_array.h"
#include "level.h"
#include "position.h"

template <class Archive>
void FieldOfView::serialize(Archive& ar, const unsigned int) {
  ar(level, vision, blocking);
  if (Archive::is_loading::value)
    visibility = Table<unique_ptr<Visibility>>(level->getBounds());
}

#ifdef MEM_USAGE_TEST
template <>
void FieldOfView::serialize(MemUsageArchive& ar1, const unsigned int) {
  ar1(level, vision, blocking, visibility);
}
#endif

template void FieldOfView::serialize(InputArchive&, unsigned);
template void FieldOfView::serialize(OutputArchive&, unsigned);

SERIALIZATION_CONSTRUCTOR_IMPL(FieldOfView)

template <typename Archive>
void FieldOfView::Visibility::serialize(Archive& ar, const unsigned int) {
  ar(visibleTiles);
}

SERIALIZABLE(FieldOfView::Visibility)

SERIALIZATION_CONSTRUCTOR_IMPL2(FieldOfView::Visibility, Visibility)

FieldOfView::FieldOfView(WLevel l, VisionId v, const ContentFactory* factory)
    : level(l), visibility(l->getBounds()), vision(v), blocking(l->getBounds().minusMargin(-1), true) {
  for (auto v : blocking.getBounds())
    blocking[v] = !Position(v, level).canSeeThru(vision, factory);
}

bool FieldOfView::canSee(Vec2 from, Vec2 to) {
  PROFILE;;
  if ((from - to).lengthD() > sightRange)
    return false;
  if (!visibility[from])
    visibility[from].reset(new Visibility(level->getBounds(), blocking, from.x, from.y));
  return visibility[from]->checkVisible(to.x - from.x, to.y - from.y);
}

void FieldOfView::squareChanged(Vec2 pos) {
  PROFILE;
  blocking[pos] = !Position(pos, level).canSeeThru(vision);
  vector<Vec2> updateList;
  if (!visibility[pos])
    visibility[pos].reset(new Visibility(level->getBounds(), blocking, pos.x, pos.y));
  for (Vec2 v : Rectangle::centered(pos, sightRange))
    if (v.inRectangle(visibility.getBounds()) && visibility[v] && visibility[v]->checkVisible(pos.x - v.x, pos.y - v.y)) {
      visibility[v].reset();
    }
}

void FieldOfView::Visibility::setVisible(Rectangle bounds, int x, int y) {
  if (Vec2(px + x, py + y).inRectangle(bounds) &&
      !visible[x + sightRange][y + sightRange] && x * x + y * y <= sightRange * sightRange) {
    visible[x + sightRange][y + sightRange] = 1;
    visibleTiles.push_back(SVec2{short(px + x), short(py + y)});
  }
}

template <typename Fun1, typename Fun2>
static void calculate(int left, int right, int up, int h, int x1, int y1, int x2, int y2, Fun1 isBlocking, Fun2 setVisible){
  if (y2*x1>=y1*x2) return;
  if (h>up) return;
  int leftx=x1, lefty=y1, rightx=x2, righty=y2;
  int left_v=(int)floor((double)x1/y1*(h)),
      right_v=(int)ceil((double)x2/y2*(h)),
      left_b=(int)floor((double)x1/y1*(h-1)),
      right_b=(int)ceil((double)x2/y2*(h+1));
  if (left_v % 2)
    ++left_v;
  if (right_v % 2)
    --right_v;
  if(left_b % 2)
    ++left_b;
  if(right_b % 2)
    --right_b;

  if(left_b>=-left && left_b<=right && isBlocking(left_b/2,h/2)){
    leftx=left_b+1;
    lefty=h+(left_b>=0?-1:1);
  }
  if(left_v<-left) left_v=-left;
  if(right_v>right) right_v=right;
  bool prevBlocking = false;
  for (int i=left_v/2;i<=right_v/2;++i){
    setVisible(i, h / 2);
    bool blocking = isBlocking(i, h / 2);
    if(i > left_v / 2 && blocking && !prevBlocking)
      calculate(left, right, up, h + 2, leftx, lefty, i * 2 - 1, h + (i<=0 ? -1:1), isBlocking, setVisible);
    if(blocking){
      leftx=i*2+1;
      lefty=h+(i>=0?-1:1);
    }
    prevBlocking = blocking;
  }
  calculate(left, right, up, h + 2, leftx, lefty, rightx, righty, isBlocking, setVisible);
}

FieldOfView::Visibility::Visibility(Rectangle bounds, const Table<bool>& blocking, int x, int y) : px(x), py(y) {
  PROFILE;
  calculate(2 * sightRange, 2 * sightRange,2 * sightRange, 2,-1,1,1,1,
      [&](int px, int py) { return blocking[Vec2(x + px, y + py)]; },
      [&](int px, int py) { setVisible(bounds, px, py); });
  calculate(2 * sightRange, 2 * sightRange,2 * sightRange, 2,-1,1,1,1,
      [&](int px, int py) { return blocking[Vec2(x + py, y - px)]; },
      [&](int px, int py) { setVisible(bounds, py, -px); });
  calculate(2 * sightRange, 2 * sightRange,2 * sightRange,2,-1,1,1,1,
      [&](int px, int py) { return blocking[Vec2(x - px, y - py)]; },
      [&](int px, int py) { setVisible(bounds, -px, -py); });
  calculate(2 * sightRange, 2 * sightRange,2 * sightRange,2,-1,1,1,1,
      [&](int px, int py) { return blocking[Vec2(x - py, y + px)]; },
      [&](int px, int py) { setVisible(bounds, -py, px); });
  setVisible(bounds, 0, 0);
  visibleTiles.shrink_to_fit();
/*  ++numSamples;
  totalIter += visibleTiles.size();
  if (numSamples%100 == 0)
    INFO << numSamples << " iterations " << totalIter / numSamples << " avg";*/
}

const vector<SVec2>& FieldOfView::Visibility::getVisibleTiles() const {
  return visibleTiles;
}

const vector<SVec2>& FieldOfView::getVisibleTiles(Vec2 from) {
  if (!visibility[from]) {
    visibility[from].reset(new Visibility(level->getBounds(), blocking, from.x, from.y));
  }
  return visibility[from]->getVisibleTiles();
}

bool FieldOfView::Visibility::checkVisible(int x, int y) const {
  return x >= -sightRange && y >= -sightRange && x <= sightRange && y <= sightRange &&
    visible[sightRange + x][sightRange + y] == 1;
}


