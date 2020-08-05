#include "map_layouts.h"



Vec2 MapLayouts::getSize(MapLayoutId id) const {
  return layouts.at(id)[0].getBounds().getSize();
}

Vec2 MapLayouts::getSize(RandomLayoutId) const {
  return Vec2(0, 0);
}

MapLayouts::Layout MapLayouts::getRandomLayout(MapLayoutId id, RandomGen& random) const {
  return random.choose(layouts.at(id));
}

optional<string> MapLayouts::addLayout(MapLayoutId id, MapLayouts::Layout l) {
  auto& list = layouts[id];
  if (!list.empty() && list[0].getBounds() != l.getBounds())
    return "Map layouts have differing sizes"_s;
  list.push_back(std::move(l));
  return none;
}

SERIALIZE_DEF(MapLayouts, layouts)
