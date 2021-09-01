#include "dancing.h"
#include "content_factory.h"
#include "creature.h"

SERIALIZE_DEF(Dancing, positions, currentDanceInfo, assignments, lastSeen, area)

SERIALIZATION_CONSTRUCTOR_IMPL(Dancing)

void Dancing::initializeCurrentDance(LocalTime startTime) {
  vector<int> indexes = [&] {
    vector<int> res;
    for (int i : Range(0, positions.size()))
      res.push_back(i);
    return Random.permutation(res);
  }();
  for (int index : indexes) {
    auto& candidatePositions = positions[index];
    for (auto& origin : area) {
      auto isGood = [&] {
        for (auto& iteration : candidatePositions.coord)
          for (auto& pos : iteration) {
            auto target = origin.plus(pos - candidatePositions.coord[0][0]);
            if (!area.count(target) || !target.canEnterEmpty(MovementTrait::WALK))
              return false;
          }
        return true;
      }();
      if (isGood) {
        currentDanceInfo = CurrentDanceInfo {
          index,
          origin,
          startTime
        };
        assignments = vector<Creature*>(candidatePositions.coord[0].size(), nullptr);
        return;
      }
    }
  }
}

Dancing::Dancing(const ContentFactory* f) : positions(f->dancePositions) {
}

void Dancing::setArea(PositionSet p, LocalTime time) {
  if (p != area) {
    area = std::move(p);
    currentDanceInfo = none;
    initializeCurrentDance(time);
  }
}

optional<int> Dancing::assignCreatureIndex(Creature* creature, LocalTime time) {
  for (int i : All(assignments))
    if (assignments[i] == creature)
      return i;
  for (int i : All(assignments))
    if (!assignments[i] || time - lastSeen.at(assignments[i]) > 20_visible) {
      assignments[i] = creature;
      return i;
    }
  return none;
}

int Dancing::getNumActive(LocalTime time) {
  int res = 0;
  for (auto& elem : assignments)
    if (elem && time - lastSeen.at(elem) < 20_visible)
      ++res;
  return res;
}

optional<Position> Dancing::getTarget(Creature* creature) {
  if (creature->isAffected(LastingEffect::SLOWED) || creature->isAffected(LastingEffect::SPEED))
    return none;
  auto time = creature->getPosition().getModel()->getLocalTime();
  if (currentDanceInfo && currentDanceInfo->startTime < time - 100_visible) {
    currentDanceInfo = none;
    initializeCurrentDance(time);
  }
  if (!currentDanceInfo)
    return none;
  auto danceIndex = currentDanceInfo->index;
  auto& curPos = positions[danceIndex];
  lastSeen[creature] = time;
  int numIterations = curPos.type == Positions::Type::FULL ? curPos.coord.size() : curPos.coord[0].size() * curPos.coord.size();
  int iteration = (time - currentDanceInfo->startTime).getVisibleInt() % numIterations;
  auto creatureIndex = assignCreatureIndex(creature, time);
  if (!creatureIndex)
    return none;
  if (getNumActive(time) < curPos.minCount)
    return none;
  return currentDanceInfo->origin.plus(curPos.get(iteration, *creatureIndex) - curPos.coord[0][0]);
}

Vec2 Dancing::Positions::get(int iteration, int creatureIndex) {
  switch (type) {
    case Type::FULL:
      return coord[iteration][creatureIndex];
    case Type::CYCLE:
      return coord[iteration % coord.size()][(creatureIndex + iteration / coord.size()) % coord[0].size()];
  }
}
