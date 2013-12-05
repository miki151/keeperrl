#ifndef _MEMORY_H
#define _MEMORY_H

#include "view_object.h"
#include "view_index.h"
#include "util.h"

class MapMemory {
  public:
  void addObject(Vec2 pos, const ViewObject& obj);
  void clearSquare(Vec2 pos);
  bool hasViewIndex(Vec2 pos) const;
  ViewIndex getViewIndex(Vec2 pos) const;
  static const MapMemory& empty();

  private:
  unordered_map<Vec2, ViewIndex> table;
};

#endif
