#include "stdafx.h"
#include "known_tiles.h"

template <class Archive>
void KnownTiles::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(known)
    & SVAR(border);
  CHECK_SERIAL;
}

SERIALIZABLE(KnownTiles);

SERIALIZATION_CONSTRUCTOR_IMPL(KnownTiles);

KnownTiles::KnownTiles(Rectangle bounds) : known(bounds, false) {
}

void KnownTiles::addTile(Vec2 pos) {
  known[pos] = true;
  border.erase(pos);
  for (Vec2 v : pos.neighbors4())
    if (v.inRectangle(known.getBounds()) && !known[v])
      border.insert(v);
}

const set<Vec2>& KnownTiles::getBorderTiles() const {
  return border;
}

bool KnownTiles::isKnown(Vec2 pos) const {
  return pos.inRectangle(known.getBounds()) && known[pos];
};
