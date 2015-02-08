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
#include "sectors.h"

template <class Archive> 
void Sectors::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(bounds)
    & SVAR(sectors)
    & SVAR(sizes);
  CHECK_SERIAL;
}

SERIALIZABLE(Sectors);
SERIALIZATION_CONSTRUCTOR_IMPL(Sectors);

Sectors::Sectors(Rectangle b) : bounds(b), sectors(bounds, -1) {
}

bool Sectors::same(Vec2 v, Vec2 w) const {
  return contains(v) && sectors[v] == sectors[w];
}

bool Sectors::contains(Vec2 v) const {
  return sectors[v] > -1;
}

void Sectors::add(Vec2 pos) {
  if (contains(pos))
    return;
  set<int> neighbors;
  for (Vec2 v : pos.neighbors8())
    if (v.inRectangle(bounds) && contains(v))
      neighbors.insert(sectors[v]);
  if (neighbors.size() == 0)
    setSector(pos, getNewSector());
  else
  if (neighbors.size() == 1)
    setSector(pos, *neighbors.begin());
  else {
    int largest = -1;
    for (int elem : neighbors)
      if (largest == -1 || sizes[largest] < sizes[elem])
        largest = elem;
    join(pos, largest);
  }
}

void Sectors::setSector(Vec2 pos, int sector) {
  CHECK(sectors[pos] != sector);
  if (contains(pos))
    --sizes[sectors[pos]];
  sectors[pos] = sector;
  ++sizes[sector];
}

int Sectors::getNewSector() {
  sizes.push_back(0);
  return sizes.size() - 1;
}

int Sectors::getNumSectors() const {
  int ret = 0;
  for (int s : sizes)
    if (s > 0)
      ++ret;
  return ret;
}

void Sectors::join(Vec2 pos1, int sector) {
  queue<Vec2> q;
  q.push(pos1);
  setSector(pos1, sector);
  while (!q.empty()) {
    Vec2 pos = q.front();
    q.pop();
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(bounds) && sectors[v] > -1 && sectors[v] != sector) {
        setSector(v, sector);
        q.push(v);
      }
  }
}

void Sectors::remove(Vec2 pos) {
  if (!contains(pos))
    return;
  int curNumber = sizes[sectors[pos]];
  --sizes[sectors[pos]];
  sectors[pos] = -1;
  int maxSector = sizes.size() - 1;
  vector<int> newSizes;
  for (Vec2 v : pos.neighbors8())
    if (v.inRectangle(bounds) && contains(v) && sectors[v] <= maxSector) {
      join(v, getNewSector());
      newSizes.push_back(sizes[sectors[v]]);
    }
  Debug() << "Sectors size " << curNumber << " split into " << newSizes;
}

using namespace std;

void Sectors::dump() {
  for (int i : Range(bounds.getH())) {
    for (int j : Range(bounds.getW()))
      cout << sectors[j][i] << " ";
    cout << endl;
  }
  cout << endl;
}
