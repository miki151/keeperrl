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

const PositionSet&KnownTiles::getAll() const {
  return known;
}

bool KnownTiles::isKnown(Position pos) const {
  return known.count(pos);
};

static void limitToModel(PositionSet& s, WConstModel m) {
  PositionSet copy;
  for (Position p : s)
    if (p.getModel() == m)
      copy.insert(p);
  s = copy;
}

void KnownTiles::limitToModel(WConstModel m) {
  ::limitToModel(known, m);
  ::limitToModel(border, m);
}
