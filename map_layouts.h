#pragma once

#include "util.h"
#include "map_layout_id.h"
#include "random_layout_id.h"

enum class LayoutPiece {
  CORRIDOR,
  WALL,
  DOOR,
  FLOOR_INSIDE,
  FLOOR_OUTSIDE,
  PRETTY_FLOOR,
  BRIDGE,
  UP_STAIRS,
  DOWN_STAIRS,
  WATER
};

class MapLayouts {
  public:

  using Layout = Table<optional<LayoutPiece>>;

  Vec2 getSize(MapLayoutId) const;
  Vec2 getSize(RandomLayoutId) const;
  Layout getRandomLayout(MapLayoutId, RandomGen&) const;
  optional<string> addLayout(MapLayoutId, Layout);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  private:
  map<MapLayoutId, vector<Layout>> SERIAL(layouts);
};

static_assert(std::is_nothrow_move_constructible<MapLayouts>::value, "T should be noexcept MoveConstructible");
