#include "stdafx.h"

#include "monster_ai.h"
#include "square.h"
#include "level.h"
#include "collective.h"
#include "village_control.h"

using namespace std;

Behaviour::Behaviour(Creature* c) : creature(c) {
}

const Creature* Behaviour::getClosestEnemy() {
  int dist = 1000000000;
  const Creature* result = nullptr;
  for (const Creature* other : creature->getVisibleEnemies()) {
    if ((other->getPosition() - creature->getPosition()).length8() < dist && other->getTribe() != Tribe::pest) {
      result = other;
      dist = (other->getPosition() - creature->getPosition()).length8();
    }
  }
  return result;
}

Item* Behaviour::getBestWeapon() {
  Item* best = nullptr;
  int damage = -1;
  for (Item* item : creature->getEquipment().getItems(Item::typePredicate(ItemType::WEAPON))) 
    if (item->getModifier(AttrType::DAMAGE) > damage) {
      damage = item->getModifier(AttrType::DAMAGE);
      best = item;
    }
  return best;
}

MoveInfo Behaviour::tryToApplyItem(EffectType type, double maxTurns) {
  auto teleItems = creature->getEquipment().getItems(Item::effectPredicate(type)); 
  for (Item* item : teleItems)
    if (creature->canApplyItem(item) && item->getApplyTime() <= maxTurns)
      return { 1, [=]() {
        creature->globalMessage(creature->getTheName() + " " + item->getApplyMsgThirdPerson(),
            item->getNoSeeApplyMsg());
        creature->applyItem(item);
    }};
  return {0, nullptr};
}

class Heal : public Behaviour {
  public:
  Heal(Creature* c) : Behaviour(c) {}

  virtual double itemValue(const Item* item) {
    if (item->getEffectType() == EffectType::HEAL) {
      return 0.5;
    }
    else
      return 0;
  }

  virtual MoveInfo getMove() {
    for (Vec2 v : Vec2::directions8())
      if (creature->canHeal(v) && creature->isFriend(creature->getConstSquare(v)->getCreature())) {
        creature->getConstSquare(v)->getCreature()->privateMessage("\"Let me help you my friend.\"");
        creature->heal(v);
      }
    if (!creature->isHumanoid() || creature->getHealth() == 1)
      return {0, nullptr};
    MoveInfo move = tryToApplyItem(EffectType::HEAL, 1);
    if (move.move)
      return { min(1.0, 1.5 - creature->getHealth()), move.move };
    move = tryToApplyItem(EffectType::HEAL, 3);
    if (move.move)
      return { 0.3 * min(1.0, 1.5 - creature->getHealth()), move.move };
    if (creature->getConstSquare()->getApplyType(creature) == SquareApplyType::SLEEP)
      return { 0.4 * min(1.0, 1.5 - creature->getHealth()), [this] {
        creature->applySquare();
      }};
    Vec2 bedRadius(10, 10);
    Level* l = creature->getLevel();
    if (creature->canSleep()) {
      if (!hasBed || hasBed->level != creature->getLevel()) {
        for (Vec2 v : Rectangle(creature->getPosition() - bedRadius, creature->getPosition() + bedRadius))
          if (l->inBounds(v) && l->getSquare(v)->getApplyType(creature) == SquareApplyType::SLEEP)
            if (Optional<Vec2> move = creature->getMoveTowards(v))
              return { 0.4 * min(1.0, 1.5 - creature->getHealth()), [=] {
                creature->move(*move);
                hasBed = {v, creature->getLevel() };
              }};
      } else 
        if (Optional<Vec2> move = creature->getMoveTowards(hasBed->pos))
          return { 0.4 * min(1.0, 1.5 - creature->getHealth()), [=] {
            creature->move(*move);
          }};
    }

    return {0, nullptr};
  }

  private:
  struct BedInfo {
    Vec2 pos;
    const Level* level;
  };
  Optional<BedInfo> hasBed;
};

class Rest : public Behaviour {
  public:
  Rest(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() {
    return {0.1, [=] { creature->wait(); }};
  }
};

class MoveRandomly : public Behaviour {
  public:
  MoveRandomly(Creature* c, int _memSize) 
      : Behaviour(c), memSize(_memSize) {}

  virtual MoveInfo getMove() {
    if (!visited(creature->getPosition()))
      updateMem(creature->getPosition());
    Vec2 direction(0, 0);
    double val = 0.0001;
    Vec2 pos = creature->getPosition();
    for (Vec2 dir : Vec2::directions8(true))
      if (!visited(pos + dir) && creature->canMove(dir)) {
        direction = dir;
        break;
      }
    if (direction == Vec2(0, 0))
      for (Vec2 dir : Vec2::directions8(true))
        if (creature->canMove(dir)) {
          direction = dir;
          break;
        }
    if (direction == Vec2(0, 0))
      return {val, [this]() { creature->wait(); }};
    else
      return {val, [this, direction]() { creature->move(direction); updateMem(creature->getPosition());}};
  }

  void updateMem(Vec2 pos) {
    memory.push_back(pos);
    if (memory.size() > memSize)
      memory.pop_front();
  }

  bool visited(Vec2 pos) {
    return contains(memory, pos);
  }

  private:
  deque<Vec2> memory;
  int memSize;
};

class AttackPest : public Behaviour {
  public:
  AttackPest(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    if (creature->getTribe() == Tribe::pest)
      return NoMove;
    const Creature* other = nullptr;
    for (Vec2 v : Vec2::directions8(true))
      if (const Creature* c = creature->getConstSquare(v)->getCreature())
        if (c->getTribe() == Tribe::pest) {
          other = c;
          break;
        }
    if (!other)
      return NoMove;
    if (creature->canAttack(other))
      return {1.0, [this, other]() {
        creature->attack(other);
      }};
    else
      return NoMove;
  }
};

class BirdFlyAway : public Behaviour {
  public:
  BirdFlyAway(Creature* c, double _maxDist) : Behaviour(c), maxDist(_maxDist) {}

  virtual MoveInfo getMove() override {
    const Creature* enemy = getClosestEnemy();
    bool fly = Random.roll(15) || ( enemy && (enemy->getPosition() - creature->getPosition()).lengthD() < maxDist);
    if (creature->canFlyAway() && fly)
      return {1.0, [this] () {
        creature->flyAway();
      }};
    else
      return NoMove;
  }

  private:
  double maxDist;
};

class GoldLust : public Behaviour {
  public:
  GoldLust(Creature* c) : Behaviour(c) {}

  virtual double itemValue(const Item* item) {
    if (item->getType() == ItemType::GOLD)
      return 1;
    else
      return 0;
  }
};

class Fighter : public Behaviour, public EventListener {
  public:
  Fighter(Creature* c, double powerR, bool _chase) : Behaviour(c), maxPowerRatio(powerR), chase(_chase) {
    EventListener::addListener(this);
    courage = c->getCourage();
  }

  virtual ~Fighter() override {
    EventListener::removeListener(this);
  }

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override {
    if (victim != creature && victim->getName() == creature->getName() && creature->canSee(victim)) {
      courage -= 0.1;
    }
    if (lastSeen && victim->getPosition() == lastSeen->pos)
      lastSeen = Nothing();
  }

  virtual void onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) override {
    if (!creature->isHumanoid())
      return;
    string name = creature->getName();
    if (contains(trajectory, creature->getPosition())
          && item->getName().size() > name.size() && item->getName().substr(0, name.size()) == name) {
      creature->globalMessage(creature->getTheName() + " screams in terror!", "You hear a scream of terror.");
      if (Random.roll(2))
        creature->rage(30);
      else
        courage -= 0.5;
    }
  }

  virtual const Level* getListenerLevel() const override {
    return creature->getLevel();
  }
 
  virtual MoveInfo getMove() override {
    const Creature* other = getClosestEnemy();
    if (other != nullptr) {
      double myDamage = creature->getAttr(AttrType::DAMAGE);
      Item* weapon = getBestWeapon();
      if (!creature->getWeapon() && weapon)
        myDamage += weapon->getModifier(AttrType::DAMAGE);
      double powerRatio = courage * myDamage / other->getAttr(AttrType::DAMAGE);
      bool significantEnemy = myDamage < 3 * other->getAttr(AttrType::DAMAGE);
      double weight = 1. - creature->getHealth() * 0.9;
      if (powerRatio < maxPowerRatio)
        weight += 2 - powerRatio * 2;
      weight = min(1.0, max(0.0, weight));
      if (creature->isPanicking())
        weight = 1;
      if (other->isSleeping() || other->isStationary())
        weight = 0;
      Debug() << creature->getName() << " panic weight " << weight;
      if (weight >= 0.5) {
        if ((creature->getPosition() - other->getPosition()).length8() < 7) {
          MoveInfo move = getPanicMove(other, weight);
          if (move.isValid())
            return move;
          else
            return getAttackMove(other, significantEnemy && chase);
        }
        return NoMove;
      } else
        return getAttackMove(other, significantEnemy && chase);
    } else
      return getAttackMove(nullptr, chase);
  }

  MoveInfo getPanicMove(const Creature* other, double weight) {
    MoveInfo teleMove = tryToApplyItem(EffectType::TELEPORT, 1);
    if (teleMove.move != nullptr)
      return {weight, teleMove.move};
    Optional<Vec2> move = creature->getMoveAway(other->getPosition(), chase);
    if (move)
      return {weight, [this, move, other] () {
        EventListener::addCombatEvent(creature);
        EventListener::addCombatEvent(other);
        creature->move(*move);
      }};
    else
      return NoMove;
  }

  virtual double itemValue(const Item* item) {
    if (item->getEffectType() &&
        contains({EffectType::INVISIBLE, EffectType::SLOW, EffectType::BLINDNESS, EffectType::SLEEP,
          EffectType::TELEPORT, EffectType::STR_BONUS, EffectType::DEX_BONUS}, *item->getEffectType()))
      return 1;
    if (item->getType() == ItemType::AMMO && creature->hasSkill(Skill::archery))
      return 0.1;
    if (item->getType() != ItemType::WEAPON || !creature->hasSkillToUseWeapon(item))
      return 0;
    if (item->getModifier(AttrType::THROWN_DAMAGE) > 0)
      return (double)item->getModifier(AttrType::THROWN_DAMAGE) / 50;
    int damage = item->getModifier(AttrType::DAMAGE);
    Item* best = getBestWeapon();
    if (best && best != item && best->getModifier(AttrType::DAMAGE) >= damage)
        return 0;
    return (double)damage / 50;
  }

  bool checkFriendlyFire(Vec2 enemyDir) {
    Vec2 dir = enemyDir.shorten();
    for (Vec2 v = dir; v != enemyDir; v += dir) {
      const Creature* c = creature->getConstSquare(v)->getCreature();
      if (c && !creature->isEnemy(c))
        return true;
    }
    return false;
  }

  double getThrowValue(Item* it) {
    if (it->getEffectType() &&
        contains({EffectType::SLOW, EffectType::BLINDNESS, EffectType::SLEEP}, *it->getEffectType()))
      return 100;
    return it->getModifier(AttrType::THROWN_DAMAGE);
  }

  MoveInfo getThrowMove(Vec2 enemyDir) {
    if (enemyDir.x != 0 && enemyDir.y != 0 && abs(enemyDir.x) != abs(enemyDir.y))
      return {0, nullptr};
    if (checkFriendlyFire(enemyDir))
      return {0, nullptr};
    Vec2 dir = enemyDir.shorten();
    Item* best = nullptr;
    int damage = 0;
    auto items = creature->getEquipment().getItems([this](Item* item) {
        return !creature->getEquipment().isEquiped(item);});
    for (Item* item : items)
      if (getThrowValue(item) > damage) {
        damage = getThrowValue(item);
        best = item;
      }
    if (best && creature->canThrowItem(best))
      return {1.0, [this, dir, best]() {
        EventListener::addCombatEvent(creature);
        creature->globalMessage(creature->getTheName() + " throws " + best->getAName());
        creature->throwItem(best, dir);
      }};
    else
      return NoMove;
  }

  MoveInfo getFireMove(Vec2 dir) {
    if (dir.x != 0 && dir.y != 0 && abs(dir.x) != abs(dir.y))
      return {0, nullptr};
    if (checkFriendlyFire(dir))
      return {0, nullptr};
    if (creature->canFire(dir.shorten()))
      return {1.0, [this, dir]() {
        EventListener::addCombatEvent(creature);
        creature->globalMessage(creature->getTheName() + " fires an arrow ");
        creature->fire(dir.shorten());
      }};
    return NoMove;
  }

  MoveInfo getAttackMove(const Creature* other, bool chase) {
    int radius = 4;
    int distance = 10000;
    double lastSeenTimeout = 20;
    if (other == nullptr) {
      if (!lastSeen) {
         return NoMove;
      }
      if (lastSeen->level != creature->getLevel() ||
          lastSeen->time < creature->getTime() - lastSeenTimeout ||
          lastSeen->pos == creature->getPosition()) {
        lastSeen = Nothing();
        return NoMove;
      }
      if (chase) {
        Optional<Vec2> move = creature->getMoveTowards(lastSeen->pos);
        if (move)
          return {0.5, [this, move]() { 
            EventListener::addCombatEvent(creature);
            Debug() << creature->getTheName() << " moving to last seen " << (lastSeen->pos - creature->getPosition());
            creature->move(*move);
          }};
        else
          return NoMove;
      }
    }
    if (other->isInvincible())
      return NoMove;
    Debug() << creature->getName() << " enemy " << other->getName();
    Vec2 enemyDir = (other->getPosition() - creature->getPosition());
    distance = enemyDir.length8();
    if (creature->isHumanoid() && !creature->getEquipment().getItem(EquipmentSlot::WEAPON)) {
      Item* weapon = getBestWeapon();
      if (weapon != nullptr && creature->canEquip(weapon))
        return {3.0 / (2.0 + distance), [this, weapon, other]() {
          EventListener::addCombatEvent(creature);
          EventListener::addCombatEvent(other);
          creature->globalMessage(creature->getTheName() + " draws " + weapon->getAName());
          creature->equip(weapon);
        }};
    }
    if (distance <= 3) {
      MoveInfo move = tryToApplyItem(EffectType::INVISIBLE, 1);
      if (move.isValid())
        return move;
      move = tryToApplyItem(EffectType::STR_BONUS, 1);
      if (move.isValid())
        return move;
      move = tryToApplyItem(EffectType::DEX_BONUS, 1);
      if (move.isValid())
        return move;
    }
    if (distance > 1) {
      MoveInfo move = getFireMove(enemyDir);
      if (move.isValid())
        return move;
      move = getThrowMove(enemyDir);
      if (move.isValid())
        return move;
      if (chase && other->getTribe() != Tribe::wildlife) {
        Optional<Vec2> move = creature->getMoveTowards(creature->getPosition() + enemyDir);
        lastSeen = Nothing();
        if (move)
          return {0.7, [this, enemyDir, move, other]() {
            EventListener::addCombatEvent(creature);
            EventListener::addCombatEvent(other);
            lastSeen = {creature->getPosition() + enemyDir, creature->getTime(), creature->getLevel()};
            creature->move(*move);
          }};
      }
    }
    if (distance == 1) {
      if (creature->canAttack(other))
        return {1.0, [this, other]() {
          EventListener::addCombatEvent(creature);
          EventListener::addCombatEvent(other);
          creature->attack(other);
        }};
    }
    return NoMove;
  }

  private:
  double maxPowerRatio;
  double courage;
  bool chase;
  struct LastSeen {
    Vec2 pos;
    double time;
    const Level* level;
  };
  Optional<LastSeen> lastSeen;
};

class GuardTarget : public Behaviour {
  public:
  GuardTarget(Creature* c, double minD, double maxD) : Behaviour(c), minDist(minD), maxDist(maxD) {}

  protected:
  MoveInfo getMoveTowards(Vec2 target) {
    double dist = (creature->getPosition() - target).lengthD();
    if (dist <= minDist)
      return NoMove;
    double exp = 1.5;
    double weight = pow((dist - minDist) / (maxDist - minDist), exp);
    Optional<Vec2> move = creature->getMoveTowards(target);
    if (move)
      return {weight, [this, move] () {
        creature->move(*move);
      }};
    else
      return NoMove;
  }

  private:
  double minDist;
  double maxDist;
};

class GuardArea : public Behaviour {
  public:
  GuardArea(Creature* c, const Location* l) : Behaviour(c), location(l), area(l->getBounds()) {}

  virtual MoveInfo getMove() override {
    if (creature->getLevel() != location->getLevel())
      return NoMove;
    if (!creature->getPosition().inRectangle(area)) {
      for (Vec2 v : Vec2::directions8())
        if ((creature->getPosition() + v).inRectangle(area) && creature->canMove(v))
          return {1.0, [this, v] () {
            creature->move(v);
          }};
      Optional<Vec2> move = creature->getMoveTowards(
          Vec2((area.getPX() + area.getKX()) / 2, (area.getPY() + area.getKY()) / 2));
      if (move)
        return {1.0, [this, move] () {
          creature->move(*move);
        }};
    }
    return NoMove;
  }

  private:
  const Location* location;
  Rectangle area;
};

class GuardSquare : public GuardTarget {
  public:
  GuardSquare(Creature* c, Vec2 _pos, double minDist, double maxDist) : GuardTarget(c, minDist, maxDist), pos(_pos) {}

  virtual MoveInfo getMove() override {
    return getMoveTowards(pos);
  }

  private:
  Vec2 pos;
};

class Wait : public Behaviour {
  public:
  Wait(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    return {1.0, [this] () {
      creature->wait();
    }};
  }
};

class GuardCreature : public GuardTarget, public EventListener {
  public:
  GuardCreature(Creature* c, Creature* _target, double minDist, double maxDist) 
      : GuardTarget(c, minDist, maxDist), target(_target) {
    EventListener::addListener(this);
  }

  ~GuardCreature() {
    EventListener::removeListener(this);
  }

  virtual void onChangeLevelEvent(const Creature* c, const Level* from,
      Vec2 pos, const Level* to, Vec2 toPos) override {
    if (c == target)
      levelChanges[from] = pos;
  }
  virtual MoveInfo getMove() override {
    if (target->isDead())
      return NoMove;
    if (target->getLevel() == creature->getLevel())
      return getMoveTowards(target->getPosition());
    if (!levelChanges.count(creature->getLevel()))
      return NoMove;
    Vec2 stairs = levelChanges.at(creature->getLevel());
    if (stairs == creature->getPosition() && creature->getSquare()->getApplyType(creature))
      return {1.0, [=] {
        creature->applySquare();
      }};
    else
      return getMoveTowards(stairs);
  }

  private:
  Creature* target;
  map<const Level*, Vec2> levelChanges;
};

class Thief : public Behaviour {
  public:
  Thief(Creature* c) : Behaviour(c) {}
 
  virtual MoveInfo getMove() override {
    if (!creature->hasSkill(Skill::stealing))
      return {0, nullptr};
    for (const Creature* other : robbed) {
      if (creature->canSee(other)) {
        MoveInfo teleMove = tryToApplyItem(EffectType::TELEPORT, 1);
        Debug() << "Gotta get out";
        if (teleMove.move != nullptr)
          return teleMove;
        Optional<Vec2> move = creature->getMoveAway(other->getPosition());
        if (move)
        return {1.0, [this, move] () {
          creature->move(*move);
        }};
      }
    }
    for (Vec2 dir : Vec2::directions8(true)) {
      const Creature* other = creature->getConstSquare(dir)->getCreature();
      if (other && !contains(robbed, other)) {
        vector<Item*> allGold;
        for (Item* it : other->getEquipment().getItems())
          if (it->getType() == ItemType::GOLD)
            allGold.push_back(it);
        if (allGold.size() > 0)
          return {1.0, [=]() {
            creature->stealFrom(dir, allGold);
            other->privateMessage(creature->getTheName() + " steals all your gold!");
            robbed.push_back(other);
          }};
      }
    }
    return {0, nullptr};
  }

  private:
  vector<const Creature*> robbed;
};

class ByCollective : public Behaviour {
  public:
  ByCollective(Creature* c, Collective* col) : Behaviour(c), collective(col) {}

  virtual MoveInfo getMove() override {
    return collective->getMove(creature);
  }

  private:
  Collective* collective;
};

class ChooseRandom : public Behaviour {
  public:
  ChooseRandom(Creature* c, vector<Behaviour*> beh, vector<double> w) : Behaviour(c), behaviours(beh), weights(w) {}

  virtual MoveInfo getMove() override {
    return chooseRandom(behaviours, weights)->getMove();
  }

  private:
  vector<Behaviour*> behaviours;
  vector<double> weights;
};

class GoToHeart : public Behaviour {
  public:
  GoToHeart(Creature* c, Vec2 _heartPos) : Behaviour(c), heartPos(_heartPos) {}

  virtual MoveInfo getMove() override {
    if (Optional<Vec2> move = creature->getMoveTowards(heartPos)) 
      return {1.0, [this, move] () {
        creature->move(*move);
      }};
    else
      return NoMove;
  };

  private:
  Vec2 heartPos;
};

class ByVillageControl : public Behaviour {
  public:
  ByVillageControl(Creature* c, VillageControl* control, Location* l) : 
      Behaviour(c), villageControl(control) {
    if (l)
      guardArea.reset(new GuardArea(c, l));
  }

  virtual MoveInfo getMove() override {
    if (villageControl->startedAttack(creature))
      return villageControl->getMove(creature);
    else if (guardArea)
      return guardArea->getMove();
    else
      return NoMove;
  }

  private:
  VillageControl* villageControl;
  PBehaviour guardArea;
};

MonsterAI::MonsterAI(Creature* c, const vector<Behaviour*>& beh, const vector<int>& w, bool pick) :
    weights(w), creature(c), pickItems(pick) {
  CHECK(beh.size() == w.size());
  for (auto b : beh)
    behaviours.push_back(PBehaviour(b));
}

void MonsterAI::makeMove() {
  vector<pair<MoveInfo, int>> moves;
  for (int i : All(behaviours)) {
    MoveInfo move = behaviours[i]->getMove();
    move.value *= weights[i];
    moves.emplace_back(move, weights[i]);
    if (pickItems) {
      map<string, vector<Item*>> stacks = Item::stackItems(creature->getPickUpOptions());
      for (auto elem : stacks) {
        Item* item = elem.second[0];
        if (!item->isUnpaid() && creature->canPickUp(elem.second)) {
          MoveInfo pickupMove { behaviours[i]->itemValue(item) * weights[i], [=]() {
            creature->globalMessage(creature->getTheName() + " picks up " + elem.first, "");
            creature->pickUp(elem.second);
          }};
          moves.emplace_back(pickupMove, weights[i]);
        }
      }
    }
  }
  /*vector<Item*> inventory = creature->getEquipment().getItems([this](Item* item) { return !creature->getEquipment().isEquiped(item);});
  for (Item* item : inventory) {
    bool useless = true;
    for (PBehaviour& behaviour : behaviours)
      if (behaviour->itemValue(item) > 0)
        useless = false;
    if (useless)
      moves.push_back({ 0.01, [=]() {
        creature->globalMessage(creature->getTheName() + " drops " + item->getAName(), "");
        creature->drop({item});
      }});
  }*/
  MoveInfo winner {0, nullptr};
  for (int i : All(moves)) {
    MoveInfo& move = moves[i].first;
    if (move.value > winner.value)
      winner = move;
    if (i < moves.size() - 1 && move.value > moves[i + 1].second)
      break;
  }
  CHECK(winner.value > 0);
  winner.move();
}

PMonsterAI MonsterAIFactory::getMonsterAI(Creature* c) {
  return PMonsterAI(maker(c));
}

MonsterAIFactory::MonsterAIFactory(MakerFun _maker) : maker(_maker) {
}

MonsterAIFactory MonsterAIFactory::monster() {
  return stayInLocation(nullptr);
}

MonsterAIFactory MonsterAIFactory::collective(Collective* col) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        new Heal(c),
        new Fighter(c, 0.6, true),
        new ByCollective(c, col),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c, 3)}, {3, 1}),
        new AttackPest(c)},
        { 6, 5, 2, 1, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::villageControl(VillageControl* col, Location* l) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        new Heal(c),
        new Fighter(c, 0.6, true),
        new GoldLust(c),
        new ByVillageControl(c, col, l),
        new MoveRandomly(c, 3)},
        { 6, 5, 3, 2, 1 });
      });
}

MonsterAIFactory MonsterAIFactory::stayInLocation(Location* l, bool moveRandomly) {
  return MonsterAIFactory([=](Creature* c) {
      vector<Behaviour*> actors { 
          new Heal(c),
          new Thief(c),
          new Fighter(c, 0.6, true),
          new AttackPest(c),
          new GoldLust(c)};
      vector<int> weights { 5, 4, 3, 1, 1 };
      if (l != nullptr) {
        actors.push_back(new GuardArea(c, l));
        weights.push_back(1);
      }
      if (moveRandomly) {
        actors.push_back(new MoveRandomly(c, 3));
        weights.push_back(1);
      } else {
        actors.push_back(new Wait(c));
        weights.push_back(1);
      }
      return new MonsterAI(c, actors, weights);
  });
}

MonsterAIFactory MonsterAIFactory::wildlifeNonPredator() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new Fighter(c, 1.2, false),
          new MoveRandomly(c, 3)},
          {5, 1});
      });
}

MonsterAIFactory MonsterAIFactory::moveRandomly() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new MoveRandomly(c, 3)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::idle() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new Rest(c)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::scavengerBird(Vec2 corpsePos) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new BirdFlyAway(c, 3),
          new MoveRandomly(c, 3),
          new GuardSquare(c, corpsePos, 1, 2)},
          {1, 1, 2});
      });
}

MonsterAIFactory MonsterAIFactory::follower(Creature* leader, int radius) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new Heal(c),
          new Fighter(c, 0.6, true),
          new GuardCreature(c, leader, radius, 3 * radius),
          new MoveRandomly(c, 3),
          new GoldLust(c)},
          { 4, 3, 2, 1, 1 });
      });
}

