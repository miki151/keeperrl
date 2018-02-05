#include "position_matching.h"
#include "movement_type.h"


optional<Position> PositionMatching::getMatch(Position pos) const {
  return matches.getValueMaybe(pos);
}

void PositionMatching::releaseTarget(Position pos) {
  targets.erase(pos);
  if (auto match = matches.getValueMaybe(pos)) {
    removeMatch(pos);
    findPath(*match);
  }
}

void PositionMatching::addTarget(Position pos) {
  targets.insert(pos);
  findPath(pos);
}

static bool isOpen(Position pos) {
  return pos.canEnterEmpty({MovementTrait::WALK});
}

void PositionMatching::updateMovement(Position pos) {
  if (targets.count(pos))
    releaseTarget(pos);
  if (isOpen(pos))
    findPath(pos);
  else {
    if (auto match = reverseMatches.getValueMaybe(pos)) {
      removeMatch(pos);
      findPath(*match);
    }
  }
}

void PositionMatching::removeMatch(Position pos) {
  if (auto match = matches.getValueMaybe(pos)) {
    reverseMatches.erase(*match);
    matches.erase(pos);
    std::cout << "Removed match " << pos.getCoord() << " " << match->getCoord() << std::endl;
  }
  if (auto match = reverseMatches.getValueMaybe(pos)) {
    matches.erase(*match);
    reverseMatches.erase(pos);
    std::cout << "Removed match " << pos.getCoord() << " " << match->getCoord() << std::endl;
  }
}

void PositionMatching::setMatch(Position pos1, Position pos2) {
  if (!targets.count(pos1)) {
    swap(pos1, pos2);
    CHECK(targets.count(pos1));
  }
  removeMatch(pos1);
  removeMatch(pos2);
  matches.set(pos1, pos2);
  reverseMatches.set(pos2, pos1);
  std::cout << "Added match " << pos1.getCoord() << " " << pos2.getCoord() << std::endl;
}

void PositionMatching::findPath(Position pos) {
  PositionSet visited;
  findPath(pos, visited, true);
}

bool PositionMatching::findPath(Position pos, PositionSet& visited, bool matchedWithFather) {
  if (visited.count(pos))
    return false;
  visited.insert(pos);
  bool isTarget = targets.count(pos);
  auto match = isTarget ? matches.getValueMaybe(pos) : reverseMatches.getValueMaybe(pos);
  if (matchedWithFather) {
    for (auto candidate : pos.neighbors8())
      if (((isTarget && isOpen(candidate)) || (!isTarget && targets.count(candidate))) &&
          findPath(candidate, visited, false)) {
        setMatch(pos, candidate);
        return true;
      }
    return false;
  } else
    return (!match || findPath(*match, visited, true));
}

SERIALIZE_DEF(PositionMatching, matches, reverseMatches, targets)
