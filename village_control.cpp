#include "stdafx.h"

#include "village_control.h"
#include "message_buffer.h"
#include "collective.h"
#include "square.h"
#include "level.h"

static int counter = 0;

VillageControl::VillageControl(Collective* c, const Level* l, StairDirection dir, StairKey key, string n) 
    : villain(c), level(l), direction(dir), stairKey(key), name(n) {
  EventListener::addListener(this);
  ++counter;
}

VillageControl::~VillageControl() {
  EventListener::removeListener(this);
}

void VillageControl::addCreature(Creature* c, int attackPoints) {
  if (tribe == nullptr)
    tribe = c->getTribe();
  CHECK(c->getTribe() == tribe);
  attackInfo[attackPoints].creatures.push_back(c);
  allCreatures.push_back(c);
}

bool VillageControl::startedAttack(Creature* c) {
  for (auto& elem : attackInfo)
    if (contains(elem.second.creatures, c)) {
      if (elem.second.attackTime > -1) {
        if (elem.second.attackTime <= c->getTime()) {
          if (elem.second.msg) {
            if (++numAttacks < attackInfo.size())
              messageBuffer.addMessage(MessageBuffer::important("The inhabitants of " + name + " are attacking!"));
            else
              messageBuffer.addMessage(MessageBuffer::important("The inhabitants of " + name + 
                    " have gathered for a final assault!"));
            elem.second.msg = false;
          }
          return true;
        }
      } else
        if (elem.first <= c->getTime() + attackPoints) {
          elem.second.attackTime = c->getTime() + 50;
          messages.insert(elem.second.attackTime);
        }
    }
  return false;
}

void VillageControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() == tribe)
    attackPoints += 2 * victim->getDifficultyPoints();
  if (contains(allCreatures, victim)) {
    removeElement(allCreatures, victim);
    if (allCreatures.empty()) {
      messageBuffer.addMessage(MessageBuffer::important("You have defeated the inhabitants of " + name));
      if (--counter == 0) {
        villain->onConqueredLand(NameGenerator::worldNames.getNext());
      }
    }
  }
}

class HumanVillageControl : public VillageControl {
  public:
  HumanVillageControl(Collective* villain, const Location* location, StairDirection dir, StairKey key)
      : VillageControl(villain, location->getLevel(), dir, key, location->getName()), villageLocation(location) {}

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(startedAttack(c));
    if (pathToDungeon.empty() && c->getLevel() != villain->getLevel())
      pathToDungeon = genPathToDungeon();
    if (c->getLevel() == villain->getLevel()) {
      if (Optional<Vec2> move = c->getMoveTowards(villain->getHeartPos())) 
        return {1.0, [this, move, c] () {
          c->move(*move);
        }};
      else {
        for (Vec2 v : Vec2::directions8(true))
          if (c->canDestroy(v) && c->getSquare(v)->getName() == "door")
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
 /*         if (c->getLevel() == villain->getLevel())
            onEnteredDungeon(attackTimes.at(c));*/
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

  vector<Vec2> genPathToDungeon() {
    Vec2 start(-1, -1);
    const Level* level = villageLocation->getLevel();
    for (Vec2 v : villageLocation->getBounds().minusMargin(-1))
      if (level->getSquare(v)->getName() == "road") {
        start = v;
        break;
      }
    CHECK(start.x > -1) << "Couldn't find path beginning in village";
    vector<Vec2> path = findStaircase(start);
    CHECK(!path.empty()) << "Couldn't find down staircase";
    reverse(path.begin(), path.end());
    return path;
  }

  unordered_set<Vec2> visited;

  vector<Vec2> findStaircase(Vec2 start) {
    const Level* level = villageLocation->getLevel();
    visited.insert(start);
    if (level->getSquare(start)->getApplyType(Creature::getDefault()) == SquareApplyType::DESCEND)
      return {start};
    for (Vec2 dir : level->getSquare(start)->getTravelDir()) {
      Vec2 nextMove = start + dir;
      if (!visited.count(nextMove)) {
        vector<Vec2> path = findStaircase(nextMove);
        if (!path.empty()) {
          path.push_back(start);
          return path;
        }
      }
    }
    return {};
  }

  const Location* villageLocation;
  vector<Vec2> pathToDungeon;
  map<Creature*, int> lastPathLocation;
};

VillageControl* VillageControl::humanVillage(Collective* villain, const Location* location,
    StairDirection dir, StairKey key) {
  return new HumanVillageControl(villain, location, dir, key);
}


class DwarfVillageControl : public VillageControl {
  public:
  DwarfVillageControl(Collective* villain, const Level* level, StairDirection dir, StairKey key)
      : VillageControl(villain, level, dir, key, "the Dwarven Halls") {}

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
 /*         if (c->getLevel() == villain->getLevel())
            onEnteredDungeon(attackTimes.at(c));*/
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

VillageControl* VillageControl::dwarfVillage(Collective* villain, const Level* level,
    StairDirection dir, StairKey key) {
  return new DwarfVillageControl(villain, level, dir, key);
}

