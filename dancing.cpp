#include "dancing.h"
#include "content_factory.h"
#include "creature.h"


template <class Archive>
void Dancing::serialize(Archive& ar, const unsigned int version) {
  ar(positions, currentDanceInfo);
  if (version < 1) {
    vector<Creature*> SERIAL(assignments);
    map<Creature*, LocalTime> SERIAL(lastSeen);
    ar(assignments, lastSeen);
    for (auto c : assignments)
      this->assignments.push_back(c ? c->getUniqueId() : 0);
    for (auto& elem : lastSeen)
      this->lastSeen.set(elem.first, elem.second);
  } else
    ar(assignments, lastSeen);
  ar(area);
}

SERIALIZABLE(Dancing)

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
        assignments = vector<UniqueEntity<Creature>::Id>(candidatePositions.coord[0].size(), 0);
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
    if (assignments[i] == creature->getUniqueId())
      return i;
  for (int i : All(assignments))
    if (!assignments[i].getGenericId() || time - lastSeen.getOrFail(assignments[i]) > 20_visible) {
      assignments[i] = creature->getUniqueId();
      return i;
    }
  return none;
}

int Dancing::getNumActive(LocalTime time) {
  int res = 0;
  for (auto& elem : assignments)
    if (!!elem.getGenericId() && time - lastSeen.getOrFail(elem) < 20_visible)
      ++res;
  return res;
}

optional<Position> Dancing::getTarget(Creature* creature) {
  if (!currentDanceInfo || creature->isAffected(LastingEffect::SLOWED) || creature->isAffected(LastingEffect::SPEED))
    return none;
  auto time = currentDanceInfo->origin.getModel()->getLocalTime();
  if (currentDanceInfo->startTime < time - 100_visible) {
    currentDanceInfo = none;
    initializeCurrentDance(time);
    if (!currentDanceInfo)
      return none;
  }
  auto danceIndex = currentDanceInfo->index;
  auto& curPos = positions[danceIndex];
  lastSeen.set(creature->getUniqueId(), time);
  int numIterations = curPos.type == Positions::Type::FULL ? curPos.coord.size() : curPos.coord[0].size() * curPos.coord.size();
  // due to the possibility of the model's local time being set back make it at least 0
  int iteration = max(0, (time - currentDanceInfo->startTime).getVisibleInt() % numIterations);
  auto creatureIndex = assignCreatureIndex(creature, time);
  if (!creatureIndex)
    return none;
  if (getNumActive(time) < curPos.minCount)
    return none;
  CHECK(iteration >= 0) << iteration << " " << time << " " << currentDanceInfo->startTime << " " << numIterations;
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
