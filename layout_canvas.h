#pragma once

#include "stdafx.h"
#include "util.h"

using Token = string;

struct LayoutCanvas {
  struct Map {
    Table<unordered_set<Token>> elems;
  };
  LayoutCanvas with(Rectangle area) const { return LayoutCanvas{area, map}; }
  Rectangle area;
  Map* map;
};
