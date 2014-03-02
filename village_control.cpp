#include "stdafx.h"

#include "village_control.h"
#include "message_buffer.h"
#include "collective.h"
#include "square.h"
#include "level.h"
#include "name_generator.h"

static int counter = 0;


template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(EventListener)
    & BOOST_SERIALIZATION_NVP(attackInfo)
    & BOOST_SERIALIZATION_NVP(allCreatures)
    & BOOST_SERIALIZATION_NVP(numAttacks)
    & BOOST_SERIALIZATION_NVP(messages)
    & BOOST_SERIALIZATION_NVP(villain)
    & BOOST_SERIALIZATION_NVP(level)
    & BOOST_SERIALIZATION_NVP(direction)
    & BOOST_SERIALIZATION_NVP(stairKey)
    & BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(tribe)
    & BOOST_SERIALIZATION_NVP(attackPoints);
}

SERIALIZABLE(VillageControl);

template <class Archive>
void VillageControl::AttackInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(creatures)
    & BOOST_SERIALIZATION_NVP(attackTime)
    & BOOST_SERIALIZATION_NVP(msg);
}

SERIALIZABLE(VillageControl::AttackInfo);

VillageControl::VillageControl(Collective* c, const Level* l, StairDirection dir, StairKey key, string n) 
    : villain(c), level(l), direction(dir), stairKey(key), name(n) {
  ++counter;
}

VillageControl::~VillageControl() {
}

void VillageControl::addCreature(Creature* c, int attackPoints) {
  if (tribe == nullptr) {
    tribe = c->getTribe();
    CHECK(!tribe->getImportantMembers().empty());
  }
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
            messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
                  " of " + name + " are attacking!"));
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
    const Creature* leader = getOnlyElement(tribe->getImportantMembers());
    if (allCreatures.empty() && !leader->isDead()) {
      messageBuffer.addMessage(MessageBuffer::important("You have defeated the pathetic attacks of the " 
            + tribe->getName() +" of " + name + ". Take advantage of their weakened defense and kill their leader, "
            "the " + leader->getName()));
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
      if (Optional<Vec2> move = c->getMoveTowards(villain->getKeeper()->getPosition())) 
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

  SERIALIZATION_CONSTRUCTOR(HumanVillageControl);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(VillageControl)
      & BOOST_SERIALIZATION_NVP(visited)
      & BOOST_SERIALIZATION_NVP(villageLocation)
      & BOOST_SERIALIZATION_NVP(pathToDungeon)
      & BOOST_SERIALIZATION_NVP(lastPathLocation);
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

  unordered_set<Vec2> visited;
  const Location* villageLocation;
  vector<Vec2> pathToDungeon;
  map<Creature*, int> lastPathLocation;
};

VillageControl* VillageControl::topLevelVillage(Collective* villain, const Location* location,
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
      if (Optional<Vec2> move = c->getMoveTowards(villain->getKeeper()->getPosition())) 
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

  SERIALIZATION_CONSTRUCTOR(DwarfVillageControl);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(VillageControl);
  }
};

template <class Archive>
void VillageControl::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, HumanVillageControl);
  REGISTER_TYPE(ar, DwarfVillageControl);
}

REGISTER_TYPES(VillageControl);

VillageControl* VillageControl::dwarfVillage(Collective* villain, const Level* level,
    StairDirection dir, StairKey key) {
  return new DwarfVillageControl(villain, level, dir, key);
}

