#include "stdafx.h"
#include "tile_predicate.h"


static bool apply(const TilePredicates::On& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  return map->elems[v].contains(p.token);
}

static bool apply(const TilePredicates::Not& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  return !p.predicate->apply(map, v, r);
}

static bool apply(const TilePredicates::True& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  return true;
}

static bool apply(const TilePredicates::And& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  for (auto& pred : p.predicates)
    if (!pred.apply(map, v, r))
      return false;
  return true;
}

static bool apply(const TilePredicates::Or& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  for (auto& pred : p.predicates)
    if (pred.apply(map, v, r))
      return true;
  return false;
}

static bool apply(const TilePredicates::Area& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  int count = 0;
  for (auto pos : Rectangle::centered(v, p.radius))
    if (pos.inRectangle(map->elems.getBounds()) && p.predicate->apply(map, pos, r))
      ++count;
  return count >= p.minCount;
}

static bool apply(const TilePredicates::Chance& p, LayoutCanvas::Map* map, Vec2 v, RandomGen& r) {
  return r.chance(p.value);
}

bool TilePredicate::apply(LayoutCanvas::Map* map, Vec2 v, RandomGen& r) const {
  return visit<bool>([&](const auto& p) { return ::apply(p, map, v, r); });
}
