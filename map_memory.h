#ifndef _MEMORY_H
#define _MEMORY_H

#include "view_object.h"
#include "view_index.h"
#include "util.h"

class MapMemory {
  public:
  MapMemory();
  MapMemory(const MapMemory&);
  void addObject(Vec2 pos, const ViewObject& obj);
  void clearSquare(Vec2 pos);
  bool hasViewIndex(Vec2 pos) const;
  ViewIndex getViewIndex(Vec2 pos) const;
  static const MapMemory& empty();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;

  private:
  Table<Optional<ViewIndex>> SERIAL(table);
};

#endif
