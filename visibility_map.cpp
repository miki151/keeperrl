#include "stdafx.h"
#include "visibility_map.h"
#include "creature.h"
#include "vision.h"

SERIALIZE_DEF(VisibilityMap, lastUpdates, visibilityCount, eyeballs)

void VisibilityMap::addPositions(const vector<Position>& positions) {
  for (Position v : positions)
    if (++visibilityCount.getOrInit(v) == 1)
      v.setNeedsRenderUpdate(true);
}

void VisibilityMap::removePositions(const vector<Position>& positions) {
  for (Position v : positions)
    if (--visibilityCount.getOrFail(v) == 0)
      v.setNeedsRenderUpdate(true);
}

void VisibilityMap::update(const Creature* c, const vector<Position>& visibleTiles) {
  PROFILE;
  remove(c);
  lastUpdates.set(c, visibleTiles);
  addPositions(visibleTiles);
}

void VisibilityMap::remove(const Creature* c) {
  if (auto positions = lastUpdates.getMaybe(c))
    removePositions(*positions);
  lastUpdates.erase(c);
}

const static Vision eyeballVision;

void VisibilityMap::updateEyeball(Position pos) {
  removeEyeball(pos);
  auto visibleTiles = pos.getVisibleTiles(eyeballVision);
  eyeballs.set(pos, visibleTiles);
  addPositions(visibleTiles);
}

void VisibilityMap::removeEyeball(Position pos) {
  if (auto positions = eyeballs.getReferenceMaybe(pos))
    removePositions(*positions);
  eyeballs.erase(pos);
}

void VisibilityMap::onVisibilityChanged(Position pos) {
  if (auto c = pos.getCreature())
    if (lastUpdates.hasKey(c))
      update(c, c->getVisibleTiles());
  if (eyeballs.contains(pos))
    updateEyeball(pos);
}

bool VisibilityMap::isVisible(Position pos) const {
  return visibilityCount.getValueMaybe(pos).value_or(0) > 0;
}

