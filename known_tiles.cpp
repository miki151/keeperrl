#include "stdafx.h"
#include "known_tiles.h"

template <class Archive>
void KnownTiles::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(known)
    & SVAR(border);
}

SERIALIZABLE(KnownTiles);

SERIALIZATION_CONSTRUCTOR_IMPL(KnownTiles);

KnownTiles::KnownTiles(const vector<Level*>& levels) : known(levels) {
}

void KnownTiles::addTile(Position pos) {
  known[pos] = true;
  border.erase(pos);
  for (Position v : pos.neighbors4())
    if (!known[v])
      border.insert(v);
}

const set<Position>& KnownTiles::getBorderTiles() const {
  return border;
}

bool KnownTiles::isKnown(Position pos) const {
  return known[pos];
};
