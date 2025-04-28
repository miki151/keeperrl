#pragma once

#include "stdafx.h"
#include "util.h"

using Token = string;

struct LayoutCanvas {
  struct Map {
    Table<vector<Token>> elems;
  };
  LayoutCanvas with(Rectangle area) const {
    USER_CHECK(map->elems.getBounds().contains(area)) << "Level generator exceeded map bounds. " << map->elems.getBounds() << " and " << area;
    return LayoutCanvas{area, map};
  }
  Rectangle area;
  Map* map;
};
