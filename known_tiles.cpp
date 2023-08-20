#include "stdafx.h"
#include "known_tiles.h"
#include "movement_type.h"

template <class Archive>
void KnownTiles::serialize(Archive& ar, const unsigned int version) {
  ar(known, border);
}

SERIALIZABLE(KnownTiles);

void KnownTiles::addTile(Position pos, Model* borderTilesModel) {
  known.insert(pos);
  if (knownWithMargin) {
    knownWithMargin->insert(pos);
    for (auto& v : pos.neighbors8())
      knownWithMargin->insert(v);
  }
  border.erase(pos);
  if (pos.getModel() == borderTilesModel)
    for (Position v : pos.neighbors4())
      if (!known.count(v) && v.canEnter(MovementType({MovementTrait::WALK, MovementTrait::FLY})))
        border.insert(v);
}

const PositionSet& KnownTiles::getBorderTiles() const {
  return border;
}

const PositionSet& KnownTiles::getAll() const {
  return known;
}

bool KnownTiles::isKnown(Position pos) const {
  return known.count(pos);
};

static void limitToModel(PositionSet& s, const Model* m) {
  PositionSet copy;
  for (auto& p : s)
    if (p.getModel() == m)
      copy.insert(p);
  s = copy;
}

void KnownTiles::limitToModel(const Model* m) {
  ::limitToModel(known, m);
  ::limitToModel(border, m);
}

void KnownTiles::limitBorderTiles(Model* m) {
  PositionSet copy;
  for (auto& p : border)
    if (p.getModel() == m && p.canEnter(MovementType(MovementTrait::FLY)))
      copy.insert(p);
  border = copy;
}

const PositionSet& KnownTiles::getKnownTilesWithMargin() {
  if (!knownWithMargin) {
    knownWithMargin = known;
    for (auto& v : known)
      for (auto neighbour : v.neighbors8())
        knownWithMargin->insert(neighbour);
  }
  return *knownWithMargin;
}
