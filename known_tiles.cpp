#include "stdafx.h"
#include "known_tiles.h"

template <class Archive>
void KnownTiles::serialize(Archive& ar, const unsigned int version) {
  ar(known, border);
}

SERIALIZABLE(KnownTiles);

void KnownTiles::addTile(Position pos) {
  known.insert(pos);
  border.erase(pos);
  for (Position v : pos.neighbors4())
    if (!known.count(v))
      border.insert(v);
}

const PositionSet& KnownTiles::getBorderTiles() const {
  return border;
}

bool KnownTiles::isKnown(Position pos) const {
  return known.count(pos);
};

static PositionSet limitToModel(const PositionSet& s, WConstModel m) {
  PositionSet copy;
  for (Position p : s)
    if (p.getModel() == m)
      copy.insert(p);
  return copy;
}

void KnownTiles::limitToModel(WConstModel m) {
  ::limitToModel(known, m);
  ::limitToModel(border, m);
}
