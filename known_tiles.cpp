#include "stdafx.h"
#include "known_tiles.h"

template <class Archive>
void KnownTiles::serialize(Archive& ar, const unsigned int version) {
  ar(known, border);
}

SERIALIZABLE(KnownTiles);

void KnownTiles::addTile(Position pos) {
  known.set(pos, true);
  border.erase(pos);
  for (Position v : pos.neighbors4())
    if (!known.get(v))
      border.insert(v);
}

const set<Position>& KnownTiles::getBorderTiles() const {
  return border;
}

bool KnownTiles::isKnown(Position pos) const {
  return known.get(pos);
};

void KnownTiles::limitToModel(const WModel m) {
  set<Position> copy;
  for (Position p : border)
    if (p.getModel() == m)
      copy.insert(p);
  border = copy;
  known.limitToModel(m);
}
