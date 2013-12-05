#include "stdafx.h"

VillageControl::VillageControl(const Collective* c, const Level* l, StairDirection dir, StairKey key) 
    : villain(c), level(l), direction(dir), stairKey(key){
  EventListener::addListener(this);
}

VillageControl::~VillageControl() {
  EventListener::removeListener(this);
}

void VillageControl::addCreature(Creature* c, int attackTime) {
  attackTimes.insert(make_pair(c, attackTime));
}

bool VillageControl::startedAttack(Creature* c) {
  return attackTimes.count(c) && attackTimes.at(c) <= c->getTime();
}

void VillageControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if (attackTimes.count(victim)) {
    attackTimes.erase(victim);
    if (attackTimes.empty())
      messageBuffer.addMessage(MessageBuffer::important("You have conquered this land. "
            "You've been playing KeeperRL alpha."));
  }
}

class HumanVillageControl : public VillageControl {
  public:
  HumanVillageControl(const Collective* villain, const Location* location, StairDirection dir, StairKey key)
      : VillageControl(villain, location->getLevel(), dir, key), villageLocation(location) {}

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(startedAttack(c));
    if (pathToDungeon.empty())
      genPathToDungeon();
    if (c->getLevel() == villain->getLevel()) {
      if (Optional<Vec2> move = c->getMoveTowards(villain->getHeartPos())) 
        return {1.0, [this, move, c] () {
          c->move(*move);
        }};
      else {
        for (Vec2 v : Vec2::directions8(true))
          if (c->canDestroy(v))
            return {1.0, [this, v, c] () {
              c->destroy(v);
            }};
        return NoMove;
      }
    } else {
      Vec2 stairs = getOnlyElement(c->getLevel()->getLandingSquares(direction, stairKey));
      if (c->getPosition() == stairs)
        return {1.0, [this, c] () {
          c->applySquare();
        }};
      int& pathInd = lastPathLocation[c];
      CHECK(pathInd >= 0 && pathInd < pathToDungeon.size() - 1)
        << "Wrong index " << pathInd << " " << (int)pathToDungeon.size();
      if (c->getPosition() == pathToDungeon[pathInd + 1])
        ++pathInd;
      if (c->getPosition() == pathToDungeon[pathInd]) {
        if (c->canMove(pathToDungeon[pathInd + 1] - pathToDungeon[pathInd]))
          return {1.0, [=] () {
            c->move(pathToDungeon[pathInd + 1] - pathToDungeon[pathInd]);
          }};
        else
          return NoMove;
      } else 
        if (Optional<Vec2> move = c->getMoveTowards(pathToDungeon[pathInd])) 
          return {1.0, [=] () {
            c->move(*move);
          }};
        else
          return NoMove;
    }
  }

  private:

  void genPathToDungeon() {
    const Level* level = villageLocation->getLevel();
    for (Vec2 v : villageLocation->getBounds())
      if (level->getSquare(v)->getTravelDir().size() == 1) {
        pathToDungeon.push_back(v);
      }
    CHECK(pathToDungeon.size() == 1) << "Couldn't find path beginning in village";
    pathToDungeon.push_back(pathToDungeon.back() +
        getOnlyElement(level->getSquare(pathToDungeon.back())->getTravelDir()));
    while (1) {
      vector<Vec2> nextMove = level->getSquare(pathToDungeon.back())->getTravelDir();
      if (nextMove.size() == 1)
        break;
      CHECK(nextMove.size() == 2);
      Vec2 next;
      if (pathToDungeon.back() + nextMove[0] != pathToDungeon[pathToDungeon.size() - 2])
        next = nextMove[0];
      else
        next = nextMove[1];
      pathToDungeon.push_back(pathToDungeon.back() + next);
    }
  }

  const Location* villageLocation;
  vector<Vec2> pathToDungeon;
  map<Creature*, int> lastPathLocation;
};

VillageControl* VillageControl::humanVillage(const Collective* villain, const Location* location,
    StairDirection dir, StairKey key) {
  return new HumanVillageControl(villain, location, dir, key);
}


class DwarfVillageControl : public VillageControl {
  public:
  DwarfVillageControl(const Collective* villain, const Level* level, StairDirection dir, StairKey key)
      : VillageControl(villain, level, dir, key) {}

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(startedAttack(c));
    if (c->getLevel() == villain->getLevel()) {
      if (Optional<Vec2> move = c->getMoveTowards(villain->getHeartPos())) 
        return {1.0, [this, move, c] () {
          c->move(*move);
        }};
      else {
        for (Vec2 v : Vec2::directions8(true))
          if (c->canDestroy(v))
            return {1.0, [this, v, c] () {
              c->destroy(v);
            }};
        return NoMove;
      }
    } else {
      Vec2 stairs = getOnlyElement(c->getLevel()->getLandingSquares(direction, stairKey));
      if (c->getPosition() == stairs)
        return {1.0, [this, c] () {
          c->applySquare();
        }};
      if (Optional<Vec2> move = c->getMoveTowards(stairs)) 
          return {1.0, [=] () {
            c->move(*move);
          }};
        else
          return NoMove;
    }
  }

  private:

  void genPathToDungeon() {
    const Level* level = villageLocation->getLevel();
    for (Vec2 v : villageLocation->getBounds())
      if (level->getSquare(v)->getTravelDir().size() == 1) {
        pathToDungeon.push_back(v);
      }
    CHECK(pathToDungeon.size() == 1) << "Couldn't find path beginning in village";
    pathToDungeon.push_back(pathToDungeon.back() +
        getOnlyElement(level->getSquare(pathToDungeon.back())->getTravelDir()));
    while (1) {
      vector<Vec2> nextMove = level->getSquare(pathToDungeon.back())->getTravelDir();
      if (nextMove.size() == 1)
        break;
      CHECK(nextMove.size() == 2);
      Vec2 next;
      if (pathToDungeon.back() + nextMove[0] != pathToDungeon[pathToDungeon.size() - 2])
        next = nextMove[0];
      else
        next = nextMove[1];
      pathToDungeon.push_back(pathToDungeon.back() + next);
    }
  }

  const Location* villageLocation;
  vector<Vec2> pathToDungeon;
  map<Creature*, int> lastPathLocation;
};

VillageControl* VillageControl::dwarfVillage(const Collective* villain, const Level* level,
    StairDirection dir, StairKey key) {
  return new DwarfVillageControl(villain, level, dir, key);
}

