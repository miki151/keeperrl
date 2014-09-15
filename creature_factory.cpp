/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "creature_factory.h"
#include "monster.h"
#include "level.h"
#include "quest.h"
#include "message_buffer.h"
#include "entity_set.h"
#include "effect.h"
#include "item_factory.h"
#include "creature_attributes.h"
#include "square.h"
#include "view_object.h"
#include "view_id.h"
#include "collective.h"

template <class Archive> 
void CreatureFactory::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tribe)
    & SVAR(creatures)
    & SVAR(weights)
    & SVAR(unique)
    & SVAR(tribeOverrides);
  CHECK_SERIAL;
}

SERIALIZABLE(CreatureFactory);

SERIALIZATION_CONSTRUCTOR_IMPL(CreatureFactory);

bool CreatureFactory::operator == (const CreatureFactory& o) const {
  return tribe == o.tribe && creatures == o.creatures && weights == o.weights && unique == o.unique;
}

class BoulderController : public Monster {
  public:
  BoulderController(Creature* c, Vec2 dir) : Monster(c, MonsterAIFactory::idle()), direction(dir) {}

  BoulderController(Creature* c, Tribe* _myTribe) : Monster(c, MonsterAIFactory::idle()),
      stopped(true), myTribe(_myTribe) {}

  void decreaseHealth(CreatureSize size) {
    switch (size) {
      case CreatureSize::HUGE: health = -1; break;
      case CreatureSize::LARGE: health -= 0.3; break;
      case CreatureSize::MEDIUM: health -= 0.15; break;
      case CreatureSize::SMALL: break;
    }
  }

  virtual void makeMove() override {
    if (myTribe != nullptr && stopped) {
      for (Vec2 v : Vec2::directions4(true)) {
        int radius = 4;
        bool found = false;
        for (int i = 1; i <= radius; ++i) {
          if (!(creature->getPosition() + (v * i)).inRectangle(creature->getLevel()->getBounds()))
            break;
          if (Creature* other = creature->getSquare(v * i)->getCreature())
            if (other->getTribe() != myTribe) {
              if (!other->hasSkill(Skill::get(SkillId::DISARM_TRAPS))) {
                direction = v;
                stopped = false;
                found = true;
                GlobalEvents.addTrapTriggerEvent(creature->getLevel(), creature->getPosition());
                creature->monsterMessage(MessageBuffer::important("The boulder starts rolling."),
                    MessageBuffer::important("You hear a heavy boulder rolling."));
                break;
              } else {
                other->you(MsgType::DISARM_TRAP, "");
                creature->die();
              }
            }
          if (!creature->getSquare(v * i)->canEnterEmpty(creature))
            break;
        }
        if (found)
          break;
      }
    }
    if (stopped) {
      creature->wait().perform();
      return;
    }
    if (creature->getConstSquare(direction)->getStrength() < 300) {
      if (Creature* c = creature->getSquare(direction)->getCreature()) {
        if (!c->isCorporal()) {
          if (auto action = creature->swapPosition(direction, true))
            action.perform();
        } else {
          decreaseHealth(c->getSize());
          if (health < 0) {
            creature->getLevel()->globalMessage(creature->getPosition() + direction,
                creature->getTheName() + " crashes on the " + c->getTheName(),
                "You hear a crash");
            creature->die();
            c->bleed(Random.getDouble(0.1, 0.3));
            return;
          } else {
            c->you(MsgType::KILLED_BY, creature->getTheName());
            c->die(creature);
          }
        }
      }
      if (auto action = creature->destroy(direction, Creature::DESTROY))
        action.perform();
    }
    if (auto action = creature->move(direction))
      action.perform();
    else {
      if (health >= 0.9 && creature->getConstSquare(direction)->canConstruct(SquareId::FLOOR)) {
        creature->globalMessage("The " + creature->getConstSquare(direction)->getName() + " is destroyed!");
        while (!creature->getSquare(direction)->construct(SquareId::FLOOR)) {}
        if (auto action = creature->move(direction))
          action.perform();
        health = 0.1;
      } else {
        creature->getLevel()->globalMessage(creature->getPosition() + direction,
            creature->getTheName() + " crashes on the " + creature->getConstSquare(direction)->getName(),
            "You hear a crash");
        creature->die();
        return;
      }
    }
    double speed = creature->getSpeed();
    double deceleration = 0.1;
    speed -= deceleration * 100 * 100 / speed;
    if (speed < 30) {
      if (myTribe) {
        creature->die();
        return;
      }
      speed = 100;
      stopped = true;
      creature->setStationary();
      myTribe = nullptr;
    }
    creature->setSpeed(speed);
  }

  virtual void you(MsgType type, const string& param) const override {
    string msg, msgNoSee;
    switch (type) {
      case MsgType::BURN: msg = creature->getTheName() + " burns in the " + param; break;
      case MsgType::DROWN: msg = creature->getTheName() + " falls into the " + param;
                           msgNoSee = "You hear a loud splash"; break;
      case MsgType::KILLED_BY: msg = creature->getTheName() + " is destroyed by " + param; break;
      case MsgType::ENTER_PORTAL: msg = creature->getTheName() + " disappears in the portal."; break;
      default: break;
    }
    if (!msg.empty())
      creature->monsterMessage(msg, msgNoSee);
  }

  virtual void onBump(Creature* c) override {
    if (myTribe)
      return;
    Vec2 dir = creature->getPosition() - c->getPosition();
    string it = c->canSee(creature) ? creature->getTheName() : "it";
    string something = c->canSee(creature) ? creature->getTheName() : "something";
    if (!creature->move(dir)) {
      c->playerMessage(it + " won't move in this direction");
      return;
    }
    c->playerMessage("You push " + something);
    if (stopped || dir == direction) {
      direction = dir;
      creature->setSpeed(100 * c->getAttr(AttrType::STRENGTH) / 30);
      stopped = false;
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Monster) 
      & SVAR(direction)
      & SVAR(stopped)
      & SVAR(myTribe);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(BoulderController);

  private:
  Vec2 SERIAL(direction);
  bool SERIAL2(stopped, false);
  Tribe* SERIAL2(myTribe, nullptr);
  double health = 1;
};

class Boulder : public Creature {
  public:
  Boulder(const CreatureAttributes& attr, Vec2 dir) : 
    Creature(Tribe::get(TribeId::KILL_EVERYONE), attr, ControllerFactory([dir](Creature* c) { 
            return new BoulderController(c, dir); })) {}

  Boulder(const CreatureAttributes& attr, Tribe* myTribe) : 
    Creature(myTribe, attr, ControllerFactory([myTribe](Creature* c) { 
            return new BoulderController(c, myTribe); })) {}

  virtual vector<PItem> getCorpse() override {
    return ItemFactory::fromId(ItemId::ROCK, Random.getRandom(10, 20));
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Creature);
  }
  
  SERIALIZATION_CONSTRUCTOR(Boulder);
};

PCreature CreatureFactory::getRollingBoulder(Vec2 direction) {
  return PCreature(new Boulder(CATTR(
            c.viewId = ViewId::BOULDER;
            c.dexterity = 1;
            c.strength = 1000;
            c.weight = 1000;
            c.humanoid = false;
            c.size = CreatureSize::LARGE;
            c.speed = 200;
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.stationary = true;
            c.noSleep = true;
            c.invincible = true;
            c.breathing = false;
            c.brain = false;
            c.name = "boulder";), direction));
}

PCreature CreatureFactory::getGuardingBoulder(Tribe* tribe) {
  return PCreature(new Boulder(CATTR(
            c.viewId = ViewId::BOULDER;
            c.dexterity = 1;
            c.strength = 1000;
            c.weight = 1000;
            c.humanoid = false;
            c.size = CreatureSize::LARGE;
            c.speed = 140;
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.noSleep = true;
            c.stationary = true;
            c.invincible = true;
            c.breathing = false;
            c.brain = false;
            c.name = "boulder";), tribe));
}

CreatureAttributes getKrakenAttributes(ViewId id) {
  return CATTR(
      c.viewId = id;
      c.speed = 40;
      c.size = CreatureSize::LARGE;
      c.strength = 15;
      c.dexterity = 15;
      c.barehandedDamage = 10;
      c.humanoid = false;
      c.bodyParts.clear();
      c.skills.insert(SkillId::SWIMMING);
      c.weight = 100;
      c.name = "kraken";);
}

class KrakenController : public Monster {
  public:
  KrakenController(Creature* c) : Monster(c, MonsterAIFactory::monster()) {
    numSpawns = chooseRandom({1, 2}, {4, 1});
  }

  void makeReady() {
    ready = true;
  }
  
  void unReady() {
    ready = false;
  }
  
  virtual void onKilled(const Creature* attacker) override {
    if (attacker) {
      if (father)
        attacker->playerMessage("You cut the kraken's tentacle");
      else
        attacker->playerMessage("You kill the kraken!");
    }
    for (Creature* c : spawns)
      if (!c->isDead())
        c->die(nullptr);
  }

  virtual void you(MsgType type, const string& param) const override {
    string msg, msgNoSee;
    switch (type) {
      case MsgType::KILLED_BY:
        if (father)
          msg = param + "cuts the kraken's tentacle";
        else
          msg = param + "kills the kraken!";
        break;
      case MsgType::DIE:
      case MsgType::DIE_OF: return;
      default: Monster::you(type, param); break;
    }
    if (!msg.empty())
      creature->monsterMessage(msg, msgNoSee);
  }

  virtual void makeMove() override {
    int radius = 10;
    if (waitNow) {
      creature->wait().perform();
      waitNow = false;
      return;
    }
    for (Creature* c : spawns)
      if (c->isDead()) {
        ++numSpawns;
        removeElement(spawns, c);
        break;
      }
    if (held && ((held->getPosition() - creature->getPosition()).length8() != 1 || held->isDead()))
      held = nullptr;
    if (held) {
      held->you(MsgType::HAPPENS_TO, creature->getTheName() + " pulls");
      if (father) {
        held->setHeld(father->creature);
        father->held = held;
        creature->die(nullptr, false);
        creature->getLevel()->moveCreature(held, creature->getPosition() - held->getPosition());
      } else {
        held->you(MsgType::ARE, "eaten by " + creature->getTheName());
        held->die();
      }
    }
    bool isEnemy = false;
    for (Vec2 v: Rectangle(Vec2(-radius, -radius), Vec2(radius + 1, radius + 1)))
      if (creature->getLevel()->inBounds(creature->getPosition() + v))
        if (Creature * c = creature->getSquare(v)->getCreature())
          if (creature->canSee(c) && creature->isEnemy(c) && !creature->isStationary()) {
            isEnemy = true;
            if (numSpawns > 0) {
              if (v.length8() == 1) {
                if (ready) {
                  c->you(MsgType::HAPPENS_TO, creature->getTheName() + " swings itself around");
                  c->setHeld(creature);
                  held = c;
                  unReady();
                } else
                  makeReady();
                break;
              }
              pair<Vec2, Vec2> dirs = v.approxL1();
              vector<Vec2> moves;
              if (creature->getSquare(dirs.first)->canEnter({{MovementTrait::WALK, MovementTrait::SWIM}}))
                moves.push_back(dirs.first);
              if (creature->getSquare(dirs.second)->canEnter({{MovementTrait::WALK, MovementTrait::SWIM}}))
                moves.push_back(dirs.second);
              if (!moves.empty()) {
                if (!ready) {
                  makeReady();
                } else {
                  Vec2 move = chooseRandom(moves);
                  ViewId viewId = creature->getSquare(move)->canEnter({MovementTrait::SWIM}) 
                    ? ViewId::KRAKEN_WATER : ViewId::KRAKEN_LAND;
                  PCreature spawn(new Creature(creature->getTribe(), getKrakenAttributes(viewId),
                        ControllerFactory([=](Creature* c) {
                          return new KrakenController(c);
                          })));
                  spawns.push_back(spawn.get());
                  dynamic_cast<KrakenController*>(spawn->getController())->father = this;
                  creature->getLevel()->addCreature(creature->getPosition() + move, std::move(spawn));
                  --numSpawns;
                  unReady();
                }
              } else
                unReady();
              break;
            }
          }
    if (!isEnemy && spawns.size() == 0 && father && Random.roll(5)) {
      creature->die(nullptr, false, false);
    }
    creature->wait().perform();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Monster)
      & SVAR(numSpawns)
      & SVAR(waitNow)
      & SVAR(ready)
      & SVAR(held)
      & SVAR(spawns)
      & SVAR(father);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(KrakenController);

  private:
  int SERIAL2(numSpawns, 0);
  bool SERIAL2(waitNow, true);
  bool SERIAL2(ready, false);
  Creature* SERIAL2(held, nullptr);
  vector<Creature*> SERIAL(spawns);
  KrakenController* SERIAL2(father, nullptr);
};

/*class Shapechanger : public Monster {
  public:
  Shapechanger(const ViewObject& obj, Tribe* tribe, const CreatureAttributes& attr, vector<CreatureId> _creatures)
      : Monster(obj, tribe, attr, MonsterAIFactory::monster()), creatures(_creatures) {}

  virtual void makeMoveSpecial() override {
    int radius = 3;
    for (Vec2 v: Rectangle(Vec2(-radius, -radius), Vec2(radius + 1, radius + 1)))
      if (getLevel()->inBounds(getPosition() + v))
        if (Creature* enemy = getSquare(v)->getCreature())
          if (canSee(enemy) && isEnemy(enemy) && enemy->isPlayer()) {
            PCreature c = CreatureFactory::fromId(chooseRandom(creatures), getTribe());
            enemy->you(MsgType::ARE, "frozen in place by " + getTheName() + "!");
            enemy->setHeld(c.get());
            globalMessage(getTheName() + " turns into " + c->getAName());
            Vec2 pos = getPosition();
            die(nullptr, false);
            getLevel()->addCreature(pos, std::move(c));
            return;
          }
    Monster::makeMoveSpecial();
  }

  private:
  vector<CreatureId> creatures;
};*/

class KamikazeController : public Monster {
  public:
  KamikazeController(Creature* c, MonsterAIFactory f) : Monster(c, f) {}

  virtual void makeMove() override {
    for (Vec2 v : Vec2::directions8())
      if (creature->getLevel()->inBounds(creature->getPosition() + v))
        if (Creature* c = creature->getSquare(v)->getCreature())
          if (creature->isEnemy(c) && creature->canSee(c)) {
            creature->monsterMessage(creature->getTheName() + " explodes!");
            for (Vec2 v : Vec2::directions8())
              c->getSquare(v)->setOnFire(1);
            c->getSquare()->setOnFire(1);
            creature->die(nullptr, false);
            return;
          }
    Monster::makeMove();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Monster);
  }

  SERIALIZATION_CONSTRUCTOR(KamikazeController);
};

class ShopkeeperController : public Monster {
  public:
  ShopkeeperController(Creature* c, Location* area) : Monster(c, MonsterAIFactory::stayInLocation(area)), shopArea(area) {
  }

  virtual void makeMove() override {
    if (creature->getLevel() != shopArea->getLevel()) {
      Monster::makeMove();
      return;
    }
    if (firstMove) {
      for (Vec2 v : shopArea->getBounds())
        for (Item* item : creature->getLevel()->getSquare(v)->getItems()) {
          myItems.insert(item);
          item->setShopkeeper(creature);
        }
      firstMove = false;
    }
    vector<const Creature*> creatures;
    for (Vec2 v : shopArea->getBounds().minusMargin(-1))
      if (const Creature* c = creature->getLevel()->getSquare(v)->getCreature()) {
        creatures.push_back(c);
        if (!prevCreatures.count(c) && !thieves.count(c) && !creature->isEnemy(c)) {
          if (!debt.count(c))
            c->playerMessage("\"Welcome to " + *creature->getFirstName() + "'s shop!\"");
          else {
            c->playerMessage("\"Pay your debt or... !\"");
            thiefCount.erase(c);
          }
        }
      }
    for (auto elem : debt) {
      const Creature* c = elem.first;
      if (!contains(creatures, c)) {
        c->playerMessage("\"Come back, you owe me " + convertToString(elem.second) + " zorkmids!\"");
        if (++thiefCount[c] == 4) {
          c->playerMessage("\"Thief! Thief!\"");
          creature->getTribe()->onItemsStolen(c);
          thiefCount.erase(c);
          debt.erase(c);
          thieves.insert(c);
          for (Item* item : c->getEquipment().getItems())
            if (unpaidItems[c].contains(item))
              item->setShopkeeper(nullptr);
          break;
        }
      }
    }
    prevCreatures.clear();
    prevCreatures.insert(creatures.begin(), creatures.end());
    Monster::makeMove();
  }

  virtual void onItemsAppeared(vector<Item*> items, const Creature* from) {
    for (Item* item : items) {
      CHECK(item->getClass() == ItemClass::GOLD);
      --debt[from];
    }
    creature->pickUp(items, false).perform();
    CHECK(debt[from] == 0) << "Bad debt " << debt[from];
    debt.erase(from);
    for (Item* it : from->getEquipment().getItems())
      if (unpaidItems[from].contains(it)) {
        it->setShopkeeper(nullptr);
        myItems.erase(it);
      }
    unpaidItems.erase(from);
  }
  
  REGISTER_HANDLER(ItemsAppearedEvent, const Level* l, Vec2 position, const vector<Item*>& items) {
    if (l == creature->getLevel())
      if (position.inRectangle(shopArea->getBounds())) {
        for (Item* it : items) {
          it->setShopkeeper(creature);
          myItems.insert(it);
        }
      }
  }

  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items) {
    if (c->getLevel() == creature->getLevel() && c->getPosition().inRectangle(shopArea->getBounds())) {
      for (const Item* item : items)
        if (myItems.contains(item)) {
          debt[c] += item->getPrice();
          unpaidItems[c].insert(item);
        }
    }
  }

  REGISTER_HANDLER(DropEvent, const Creature* c, const vector<Item*>& items) {
    if (c->getLevel() == creature->getLevel() && c->getPosition().inRectangle(shopArea->getBounds())) {
      for (const Item* item : items)
        if (myItems.contains(item)) {
          if ((debt[c] -= item->getPrice()) <= 0)
            debt.erase(c);
          unpaidItems[c].erase(item);
        }
    }
  }

  virtual int getDebt(const Creature* debtor) const override {
    if (debt.count(debtor)) {
      return debt.at(debtor);
    }
    else {
      return 0;
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Monster)
      & SVAR(prevCreatures)
      & SVAR(debt)
      & SVAR(thiefCount)
      & SVAR(thieves)
      & SVAR(unpaidItems)
      & SVAR(shopArea)
      & SVAR(myItems)
      & SVAR(firstMove);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(ShopkeeperController);

  private:
  unordered_set<const Creature*> SERIAL(prevCreatures);
  unordered_map<const Creature*, int> SERIAL(debt);
  unordered_map<const Creature*, int> SERIAL(thiefCount);
  unordered_set<const Creature*> SERIAL(thieves);
  unordered_map<const Creature*, EntitySet<Item>> SERIAL(unpaidItems);
  Location* SERIAL(shopArea);
  EntitySet<Item> SERIAL(myItems);
  bool SERIAL2(firstMove, true);
};

class VillageElder : public Creature {
  public:
  VillageElder(vector<pair<Skill*, double>> _teachSkills, vector<pair<QuestId, int>> _quests,
      Tribe* t, const CreatureAttributes& attr, ControllerFactory factory) : 
      Creature(t, attr, factory), teachSkills(_teachSkills) {
    getTribe()->setLeader(this);
    for (auto elem : _quests)
      if (Quest::exists(elem.first))
        quests.emplace_back(Quest::get(elem.first), elem.second);
  }

  bool teach(Creature* who) {
    for (auto elem : teachSkills)
      if (!who->hasSkill(elem.first) && getTribe()->getStanding(who) >= elem.second) {
        Skill* teachSkill = elem.first;
        who->addSkill(teachSkill);
        who->playerMessage("\"You are a friend of our tribe. I will share my knowledge with you.\"");
        who->playerMessage(getName() + " teaches you the " + teachSkill->getName());
        if (teachSkill == Skill::get(SkillId::ARCHERY)) {
          who->playerMessage(getName() + " hands you a bow and a quiver of arrows.");
          who->take(std::move(ItemFactory::fromId(ItemId::BOW)));
          who->take(ItemFactory::fromId(ItemId::ARROW, Random.getRandom(20, 36)));
        }
        return true;
      }
    return false;
  }

  bool tribeQuest(Creature* who) {
    for (pair<Quest*, int> q : quests) {
      if (q.first->isFinished()) {  
        who->playerMessage("\"" + (*who->getFirstName()) +
            ", you have fulfilled your quest. Here is your payment.\"");
        who->takeItems(ItemFactory::fromId(ItemId::GOLD_PIECE, q.second), this);
        removeElement(quests, q);
        return true;
      } else {
        q.first->addAdventurer(who);
        return true;
      }
    }
    return false;
  }

  virtual void onChat(Creature* who) override {
    if (isEnemy(who)) {
      Creature::onChat(who);
      return;
    }
    if (tribeQuest(who))
      return;
    Creature::onChat(who);
    return;
 //   if (teach(who))
 //     return;
    const vector<Location*> locations = getLevel()->getAllLocations();
    if (locations.size() == 0)
      Creature::onChat(who);
    else
      while (1) {
        Location* l = locations[Random.getRandom(locations.size())];
        if (l->hasName() && l != getLevel()->getLocation(getPosition())) {
          Vec2 dir = l->getBounds().middle() - getPosition();
          string dist = dir.lengthD() > 150 ? "far" : "close";
          string bearing = getCardinalName(dir.getBearing().getCardinalDir());
          who->playerMessage("\"There is a " + l->getName() + " " + dist + " in the " + bearing + "\"");
          who->playerMessage("\"" + l->getDescription() + "\"");
          return;
        }
      }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Creature)
      & SVAR(teachSkills)
      & SVAR(quests)
      & SVAR(bringItem)
      & SVAR(killCreature);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(VillageElder);

  private:
  vector<pair<Skill*, double>> SERIAL(teachSkills);
  vector<pair<Quest*, int>> SERIAL(quests);
  const Item* SERIAL2(bringItem, nullptr);
  const Creature* SERIAL2(killCreature, nullptr);
};

class GreenDragonController : public Monster {
  public:
  using Monster::Monster;

  virtual void sleeping() override {
    Effect::applyToCreature(creature, EffectId::EMIT_POISON_GAS, EffectStrength::WEAK);
  }

  virtual void makeMove() override {
    Effect::applyToCreature(creature, EffectId::EMIT_POISON_GAS, EffectStrength::WEAK);
    Monster::makeMove();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Monster);
  }
};

class RedDragonController : public Monster {
  public:
  RedDragonController(Creature* c, MonsterAIFactory f) : Monster(c, f) {}

  virtual void makeMove() override {
    if (creature->getTime() > lastSpawn + 10)
      for (Vec2 v : Rectangle(-Vec2(5, 5), Vec2(5, 5)))
        if (creature->getLevel()->inBounds(creature->getPosition() + v))
          if (const Creature* c = creature->getSquare(v)->getCreature())
            if (creature->isEnemy(c)) {
              Effect::applyToCreature(creature, EffectId::FIRE_SPHERE_PET, EffectStrength::NORMAL);
              lastSpawn = creature->getTime();
            }
    Monster::makeMove();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Monster)
      & SVAR(lastSpawn);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(RedDragonController);

  private:
  double SERIAL2(lastSpawn, -100);
};

template <class Archive>
void CreatureFactory::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, GreenDragonController);
  REGISTER_TYPE(ar, RedDragonController);
  REGISTER_TYPE(ar, BoulderController);
  REGISTER_TYPE(ar, Boulder);
  REGISTER_TYPE(ar, KrakenController);
  REGISTER_TYPE(ar, KamikazeController);
  REGISTER_TYPE(ar, ShopkeeperController);
  REGISTER_TYPE(ar, VillageElder);
}

REGISTER_TYPES(CreatureFactory);

PCreature CreatureFactory::addInventory(PCreature c, const vector<ItemType>& items) {
  for (ItemType item : items) {
    PItem it = ItemFactory::fromId(item);
    Item* ref = it.get();
    c->take(std::move(it));
  }
  return c;
}

PCreature CreatureFactory::getShopkeeper(Location* shopArea, Tribe* tribe) {
  PCreature ret(new Creature(tribe,
      CATTR(
        c.viewId = ViewId::SHOPKEEPER;
        c.speed = 100;
        c.size = CreatureSize::LARGE;
        c.strength = 17;
        c.dexterity = 13;
        c.barehandedDamage = 13;
        c.humanoid = true;
        c.weight = 100;
        c.chatReactionFriendly = "complains about high import tax";
        c.chatReactionHostile = "\"Die!\"";
        c.name = "shopkeeper";
        c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();),
      ControllerFactory([shopArea](Creature* c) { 
          return new ShopkeeperController(c, shopArea); })));
  vector<ItemType> inventory(Random.getRandom(100, 300), ItemId::GOLD_PIECE);
  inventory.push_back(ItemId::SWORD);
  inventory.push_back(ItemId::LEATHER_ARMOR);
  inventory.push_back(ItemId::LEATHER_BOOTS);
  inventory.push_back({ItemId::POTION, EffectId::HEAL});
  inventory.push_back({ItemId::POTION, EffectId::HEAL});
  return addInventory(std::move(ret), inventory);
}

Tribe* CreatureFactory::getTribeFor(CreatureId id) {
  if (Tribe* t = tribeOverrides[id])
    return t;
  else
    return tribe;
}

PCreature CreatureFactory::random(MonsterAIFactory actorFactory) {
  CreatureId id;
  if (unique.size() > 0) {
    id = unique.back();
    unique.pop_back();
  } else
    id = chooseRandom(creatures, weights);
  return fromId(id, getTribeFor(id), actorFactory);
}

PCreature get(
    CreatureAttributes attr, 
    Tribe* tribe,
    ControllerFactory factory) {
  return PCreature(new Creature(tribe, attr, factory));
}

CreatureFactory::CreatureFactory(Tribe* t, const vector<CreatureId>& c, const vector<double>& w,
    const vector<CreatureId>& u, EnumMap<CreatureId, Tribe*> overrides)
    : tribe(t), creatures(c), weights(w), unique(u), tribeOverrides(overrides) {
}

CreatureFactory CreatureFactory::humanVillage(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::PESEANT,
      CreatureId::CHILD, CreatureId::HORSE, CreatureId::COW, CreatureId::PIG, CreatureId::DOG },
      { 2, 1, 1, 1, 1, 0}, {});
}

CreatureFactory CreatureFactory::humanCastle(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER,
      CreatureId::PESEANT, CreatureId::CHILD, CreatureId::HORSE, CreatureId::COW, CreatureId::PIG, CreatureId::DOG },
      { 10, 6, 2, 1, 1, 1, 1, 1}, {CreatureId::AVATAR});
}

CreatureFactory CreatureFactory::elvenVillage(Tribe* tribe) {
  double armedRatio = 0.4;
  CreatureFactory ret(tribe, { CreatureId::ELF, CreatureId::ELF_CHILD, CreatureId::HORSE,
      CreatureId::COW, CreatureId::DOG },
      { 2, 2, 1, 1, 0}, {});
  double sum = 0;
  for (double w : ret.weights)
    sum += w;
  ret.weights.push_back(sum * armedRatio / (1 - armedRatio));
  ret.creatures.push_back(CreatureId::ELF_ARCHER);
  ret.unique.push_back(CreatureId::ELF_LORD);
  return ret;
}

CreatureFactory CreatureFactory::forrest(Tribe* tribe) {
  return CreatureFactory(tribe,
      { CreatureId::DEER, CreatureId::FOX, CreatureId::BOAR, CreatureId::LEPRECHAUN },
      { 4, 2, 2, 1}, {});
}

CreatureFactory CreatureFactory::crypt(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::ZOMBIE}, { 1}, {});
}

CreatureFactory CreatureFactory::coffins(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::VAMPIRE}, { 1}, {});
}

CreatureFactory CreatureFactory::hellLevel(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::DEVIL}, { 1}, {CreatureId::DARK_KNIGHT});
}

CreatureFactory CreatureFactory::vikingTown(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::WARRIOR}, { 1}, {CreatureId::SHAMAN});
}

CreatureFactory CreatureFactory::lizardTown(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::LIZARDMAN, }, { 1}, {CreatureId::LIZARDLORD});
}

CreatureFactory CreatureFactory::dwarfTown(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::DWARF}, { 1}, { CreatureId::DWARF_BARON});
}

CreatureFactory CreatureFactory::splash(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::IMP}, { 1}, { CreatureId::KEEPER });
}

CreatureFactory CreatureFactory::goblinTown(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::GOBLIN, CreatureId::RAT}, {2, 1}, {CreatureId::GREAT_GOBLIN});
}

CreatureFactory CreatureFactory::pyramid(Tribe* tribe, int level) {
  if (level == 2)
    return CreatureFactory(tribe, { CreatureId::MUMMY }, {1}, { CreatureId::MUMMY_LORD });
  else
    return CreatureFactory(tribe, { CreatureId::MUMMY }, {1}, { });
}

CreatureFactory CreatureFactory::insects(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::SPIDER, CreatureId::SCORPION }, {1, 1}, { });
}

CreatureFactory CreatureFactory::waterCreatures(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::KRAKEN }, {1}, { });
}

CreatureFactory CreatureFactory::lavaCreatures(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::FIRE_SPHERE }, {1}, { });
}

CreatureFactory CreatureFactory::level(int num, Tribe* allTribe, Tribe* dwarfTribe, Tribe* pestTribe) {
  int maxLevel = 8;
  CHECK(num <= maxLevel && num > 0);
  map<CreatureId, vector<int>> frequencies {
      { CreatureId::SPECIAL_MONSTER, { 5, 8, 10, 12, 15, 18, 20, 22}},
      { CreatureId::SPECIAL_HUMANOID, { 5, 8, 10, 12, 15, 18, 20, 22}},
      { CreatureId::GNOME, { 400, 200, 100, 100, 100, 100, 100, 100, 100, 100}},
      { CreatureId::LEPRECHAUN, { 20, 20, 20, 20, 20, 20, 20, 20}},
      { CreatureId::GOBLIN, { 20, 20, 30, 50, 50, 100, 200, 400 }},
      { CreatureId::OGRE, { 0, 0, 100, 100, 200, 200, 200, 200, 200, 200 }},
      { CreatureId::JACKAL, { 400, 100, 100, 100, 100, 100, 100, 100, 100, 100 }},
      { CreatureId::ACID_MOUND, { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }},
      { CreatureId::SPIDER, { 200, 100, 100, 100, 100, 100, 100, 100, 100, 100 }},
      { CreatureId::SCORPION, { 200, 100, 100, 100, 100, 100, 100, 100, 100, 100 }},
      { CreatureId::RAT, { 200, 200, 200, 200, 200, 200, 200, 200, 200, 200 }},
      { CreatureId::BAT, { 100, 100, 200, 200, 200, 200, 200, 200, 200, 200 }},
      { CreatureId::ZOMBIE, { 0, 0, 0, 30, 50, 100, 100, 100, 100, 100 }},
      { CreatureId::SKELETON, { 0, 0, 0, 30, 50, 100, 100, 100, 100, 100 }},
      { CreatureId::VAMPIRE, { 0, 0, 0, 10, 30, 50, 100, 100, 100, 100 }},
      { CreatureId::VAMPIRE_LORD, { 0, 0, 0, 0, 10, 10, 30, 50, 50, 50 }},
      { CreatureId::NIGHTMARE, { 5, 5, 10, 10, 20, 30, 30, 40, 40, 40 }},
      { CreatureId::DWARF, { 400, 200, 100, 50, 50, 30, 20, 20 }}};
  vector<vector<CreatureId>> uniqueMonsters(maxLevel);
  uniqueMonsters[Random.getRandom(5, maxLevel)].push_back(CreatureId::SPECIAL_MONSTER);
  uniqueMonsters[Random.getRandom(5, maxLevel)].push_back(CreatureId::SPECIAL_HUMANOID);
  vector<CreatureId> ids;
  vector<double> freq;
  for (auto elem : frequencies) {
    ids.push_back(elem.first);
    freq.push_back(elem.second[num - 1]);
  }
  return CreatureFactory(allTribe, ids, freq, uniqueMonsters[num - 1], 
      {{CreatureId::DWARF, dwarfTribe}, {CreatureId::RAT, pestTribe}});
}

CreatureFactory CreatureFactory::singleType(Tribe* tribe, CreatureId id) {
  return CreatureFactory(tribe, { id}, {1}, {});
}

/*vector<PCreature> CreatureFactory::getFlock(int size, CreatureId id, Creature* leader) {
  vector<PCreature> ret;
  for (int i : Range(size)) {
    PCreature c = fromId(id, leader->getTribe(), MonsterAIFactory::follower(leader, 5));
    ret.push_back(std::move(c));
  }
  return ret;
}*/

PCreature getSpecial(const string& name, Tribe* tribe, bool humanoid, ControllerFactory factory, bool keeper) {
  RandomGen r;
  r.init(hash<string>()(name));
  PCreature c = get(CATTR(
        c.viewId = humanoid ? ViewId::SPECIAL_HUMANOID : ViewId::SPECIAL_BEAST;
        c.speed = r.getRandom(70, 150);
        c.size = chooseRandom({CreatureSize::SMALL, CreatureSize::MEDIUM, CreatureSize::LARGE}, {1, 1, 1});
        c.strength = r.getRandom(20, 26);
        c.dexterity = r.getRandom(20, 26);
        c.barehandedDamage = r.getRandom(5, 15);
        c.humanoid = humanoid;
        c.spawnType = humanoid ? SpawnType::HUMANOID : SpawnType::BEAST;
        c.weight = c.size == CreatureSize::LARGE ? r.getRandom(80,120) : 
                   c.size == CreatureSize::MEDIUM ? r.getRandom(40, 60) :
                   r.getRandom(5, 20);
        if (*c.humanoid) {
          c.chatReactionFriendly = "\"I am the mighty " + name + "\"";
          c.chatReactionHostile = "\"I am the mighty " + name + ". Die!\"";
          c.minionTasks[MinionTask::WORKSHOP] = 1;
          c.minionTasks[MinionTask::TRAIN] = 4; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::SLEEP] = 1;
        } else {
          c.chatReactionFriendly = c.chatReactionHostile = "The " + name + " snarls.";
        }
        c.name = name;
        c.speciesName = humanoid ? "legendary humanoid" : "legendary beast";
        if (!(*c.humanoid) && Random.roll(10)) {
          c.uncorporal = true;
          c.bodyParts.clear();
          *c.strength -= 5;
          *c.dexterity += 10;
          c.barehandedDamage += 10;
        } else {
          if (r.roll(4)) {
            c.bodyParts[BodyPart::WING] = 2;
            c.permanentEffects[LastingEffect::FLYING] = 1;
          }
          if (*c.humanoid == false) {
            c.bodyParts[BodyPart::ARM] = r.roll(2) ? 2 : 0;
            c.bodyParts[BodyPart::LEG] = r.getRandom(3) * 2;
            *c.strength += 5;
            *c.dexterity += 5;
            c.barehandedDamage += 5;
            switch (Random.getRandom(8)) {
              case 0: c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON); break;
              case 1: c.attackEffect = EffectId::FIRE; c.barehandedAttack = AttackType::HIT; break;
              default: break;
            }
          }
          if (Random.roll(10)) {
            c.undead = true;
            c.name = "undead " + *c.name;
          }
        }
        if (r.roll(3))
          c.skills.insert(SkillId::SWIMMING);
        c.specialMonster = true;
        ), tribe, factory);
  if (c->isHumanoid()) {
    if (Random.roll(400)) {
      c->take(ItemFactory::fromId(ItemId::BOW));
      c->take(ItemFactory::fromId(ItemId::ARROW, Random.getRandom(20, 36)));
    } else
      c->take(ItemFactory::fromId(chooseRandom(
            {ItemId::SPECIAL_SWORD, ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER})));
  } else if (!keeper) {
    switch (Random.getRandom(3)) {
      case 0:
        c->take(ItemFactory::fromId(
              chooseRandom({ItemId::WARNING_AMULET, ItemId::HEALING_AMULET, ItemId::DEFENSE_AMULET})));
        break;
      case 1:
        c->take(ItemFactory::fromId({ItemId::POTION,
              EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)},
              Random.getRandom(3, 6)));
        break;
      case 2:
        c->take(ItemFactory::fromId(chooseRandom<ItemType>({
              {ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::STR_BONUS)},
              {ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS)}}), Random.getRandom(3, 6)));
        break;
      default:
        FAIL << "Unhandled case value";
    }

  }
  Debug() << c->getDescription();
  return c;
}

#define INHERIT(ID, X) CreatureAttributes([&](CreatureAttributes& c) { c = getAttributes(CreatureId::ID); X })

CreatureAttributes getAttributes(CreatureId id) {
  switch (id) {
    case CreatureId::KEEPER: 
      return CATTR(
          c.viewId = ViewId::KEEPER;
          c.undeadViewId = ViewId::UNDEAD_KEEPER;
          c.undeadName = "Lich";
          c.speed = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.strength = 15;
          c.dexterity = 15;
          c.barehandedDamage = 5;
          c.humanoid = true;
          c.name = "Keeper";
          c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();
          c.spells.push_back(Creature::getSpell(SpellId::HEALING));
          c.attributeGain = 1;
          c.minionTasks[MinionTask::STUDY] = 1;
          c.minionTasks[MinionTask::LABORATORY] = 0.0001; 
          c.minionTasks[MinionTask::WORSHIP] = 0.0001;
          c.skillGain.clear(););
    case CreatureId::BANDIT: 
      return CATTR(
          c.viewId = ViewId::BANDIT;
          c.speed = 80;
          c.size = CreatureSize::LARGE;
          c.strength = 15;
          c.dexterity = 13;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all law enforcement";
          c.chatReactionHostile = "\"Die!\"";
          c.skills.insert(SkillId::DISARM_TRAPS);
          c.name = "bandit";);
    case CreatureId::GHOST: 
      return CATTR(
          c.viewId = ViewId::GHOST;
          c.speed = 80;
          c.size = CreatureSize::LARGE;
          c.strength = 14;
          c.dexterity = 15;
          c.barehandedDamage = 3;
          c.barehandedAttack = AttackType::HIT;
          c.humanoid = false;
          c.uncorporal = true;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.weight = 10;
          c.bodyParts.clear();
          c.chatReactionFriendly = "\"Wouuuouuu!!!\"";
          c.chatReactionHostile = "\"Wouuuouuu!!!\"";
          c.name = "ghost";);
    case CreatureId::SPIRIT:
      return INHERIT(GHOST,
          c.viewId = ViewId::SPIRIT;
          c.strength = 24;
          c.speed = 100;
          c.dexterity = 30;
          c.barehandedDamage = 30;
          c.name = "ancient spirit";);
    case CreatureId::LOST_SOUL:
      return INHERIT(GHOST,
          c.strength = 5;
          c.courage = 10;
          c.spawnType = SpawnType::DEMON;
          c.barehandedAttack = AttackType::POSSESS;
          c.minionTasks[MinionTask::RITUAL] = 1;
          c.name = "ghost";);
    case CreatureId::SUCCUBUS:
      return INHERIT(GHOST,
          c.strength = 5;
          c.viewId = ViewId::SUCCUBUS;
          c.spawnType = SpawnType::DEMON;
          c.minionTasks[MinionTask::COPULATE] = 1;
          c.minionTasks[MinionTask::RITUAL] = 1;
          c.gender = Gender::female;
          c.humanoid = true;
          c.bodyParts[BodyPart::HEAD] = 1;
          c.bodyParts[BodyPart::LEG] = 2;
          c.bodyParts[BodyPart::ARM] = 2;
          c.bodyParts[BodyPart::WING] = 2;
          c.courage = 0.0;
          c.name = "succubus";
          );
    case CreatureId::DOPPLEGANGER:
      return INHERIT(GHOST,
          c.viewId = ViewId::DOPPLEGANGER;
          c.permanentEffects[LastingEffect::FLYING] = 0;
          c.minionTasks[MinionTask::CONSUME] = 0.4;
          c.minionTasks[MinionTask::RITUAL] = 1;
          c.spawnType = SpawnType::DEMON;
          c.skills.insert(SkillId::CONSUMPTION);
          c.name = "doppleganger";
          );
    case CreatureId::DEVIL: 
      return CATTR(
          c.viewId = ViewId::DEVIL;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 19;
          c.dexterity = 16;
          c.barehandedDamage = 10;
          c.humanoid = true;
          c.weight = 80;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "devil";);
    case CreatureId::DARK_KNIGHT: 
      return CATTR(
          c.viewId = ViewId::DARK_KNIGHT;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 22;
          c.dexterity = 19;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "dark knight";);
    case CreatureId::WITCH: 
      return CATTR(
          c.viewId = ViewId::WITCH;
          c.speed = 100;
          c.size = CreatureSize::MEDIUM;
          c.strength = 14;
          c.dexterity = 12;
          c.barehandedDamage = 6;
          c.humanoid = true;
          c.weight = 50;
          c.gender = Gender::female;
          c.chatReactionFriendly = "curses all humans";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "witch";);
    case CreatureId::CYCLOPS: 
      return CATTR(
          c.viewId = ViewId::CYCLOPS;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 25;
          c.dexterity = 18;
          c.barehandedDamage = 10;
          c.humanoid = true;
          c.weight = 150;
          c.firstName = NameGenerator::get(NameGeneratorId::DEMON)->getNext();
          c.name = "cyclops";);
    case CreatureId::GREEN_DRAGON: 
      return CATTR(
          c.viewId = ViewId::GREEN_DRAGON;
          c.speed = 90;
          c.size = CreatureSize::HUGE;
          c.strength = 40;
          c.dexterity = 25;
          c.barehandedDamage = 5;
          c.barehandedAttack = AttackType::EAT;
          c.humanoid = false;
          c.weight = 1000;
          c.worshipped = true;
          c.bodyParts[BodyPart::WING] = 2;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.name = NameGenerator::get(NameGeneratorId::DRAGON)->getNext();
          c.speciesName = "green dragon";
          );
    case CreatureId::RED_DRAGON:
      return INHERIT(GREEN_DRAGON,
          c.viewId = ViewId::RED_DRAGON;
          c.fireCreature = true;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 0;
          c.speciesName = "red dragon";
          );
    case CreatureId::KNIGHT: 
      return CATTR(
          c.viewId = ViewId::KNIGHT;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 26;
          c.dexterity = 19;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "knight";);
    case CreatureId::CASTLE_GUARD: 
      return INHERIT(KNIGHT,
          c.viewId = ViewId::CASTLE_GUARD;
          c.name = "guard";);
    case CreatureId::AVATAR: 
      return INHERIT(KNIGHT,
          c.viewId = ViewId::AVATAR;
          c.strength += 5;
          c.dexterity += 2;
          c.courage = 3;
          c.barehandedDamage += 5;
          c.name = "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext(););
    case CreatureId::WARRIOR:
      return INHERIT(KNIGHT,
          c.viewId = ViewId::WARRIOR;
          c.strength -= 3;
          c.dexterity -= 3;
          c.name = "warrior";);
    case CreatureId::SHAMAN:
      return INHERIT(WARRIOR,
          c.viewId = ViewId::SHAMAN;
          c.courage = 3;
          c.spells.push_back(Creature::getSpell(SpellId::HEALING));
          c.spells.push_back(Creature::getSpell(SpellId::SPEED_SELF));
          c.spells.push_back(Creature::getSpell(SpellId::STR_BONUS));
          c.spells.push_back(Creature::getSpell(SpellId::SUMMON_SPIRIT));
          c.name = "shaman";);
    case CreatureId::ARCHER: 
      return CATTR(
          c.viewId = ViewId::ARCHER;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 17;
          c.dexterity = 22;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "archer";);
    case CreatureId::PESEANT: 
      return CATTR(
          c.viewId = chooseRandom({ViewId::PESEANT, ViewId::PESEANT_WOMAN});
          c.speed = 80;
          c.size = CreatureSize::LARGE;
          c.strength = 14;
          c.dexterity = 12;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.innocent = true;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Heeelp!\"";
          c.name = "peasant";);
    case CreatureId::CHILD: 
      return INHERIT(PESEANT,
          c.viewId = ViewId::CHILD;
          c.speed = 140;
          c.size = CreatureSize::MEDIUM;
          c.strength = 8;
          c.dexterity = 16;
          c.weight = 40;
          c.chatReactionFriendly = "\"plaaaaay!\"";
          c.chatReactionHostile = "\"Heeelp!\"";
          c.name = "child";);
    case CreatureId::CLAY_GOLEM: 
      return CATTR(
          c.viewId = ViewId::CLAY_GOLEM;
          c.speed = 50;
          c.size = CreatureSize::LARGE;
          c.strength = 14;
          c.dexterity = 12;
          c.barehandedDamage = 13;
          c.barehandedAttack = AttackType::PUNCH;
          c.humanoid = false;
          c.noSleep = true;
          c.brain = false;
          c.notLiving = true;
          c.weight = 100;
          c.minionTasks[MinionTask::TRAIN] = 1;
          c.name = "clay golem";);
    case CreatureId::STONE_GOLEM: 
      return INHERIT(CLAY_GOLEM,
          c.viewId = ViewId::STONE_GOLEM;
          c.speed += 10;
          c.strength += 2;
          c.dexterity += 2;
          c.barehandedDamage += 2;
          c.weight += 20;
          c.name = "stone golem";);
    case CreatureId::IRON_GOLEM: 
      return INHERIT(STONE_GOLEM,
          c.viewId = ViewId::IRON_GOLEM;
          c.speed += 10;
          c.strength += 2;
          c.dexterity += 2;
          c.barehandedDamage += 2;
          c.name = "iron golem";);
    case CreatureId::LAVA_GOLEM: 
      return INHERIT(IRON_GOLEM,
          c.viewId = ViewId::LAVA_GOLEM;
          c.speed += 10;
          c.strength += 2;
          c.dexterity += 2;
          c.barehandedDamage += 2;
          c.attackEffect = EffectId::FIRE;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.name = "lava golem";);
    case CreatureId::ACID_MOUND: 
      return CATTR(
          c.viewId = ViewId::ACID_MOUND;
          c.speed = 80;
          c.size = CreatureSize::LARGE;
          c.strength = 15;
          c.dexterity = 5;
          c.barehandedDamage = 1;
          c.humanoid = false;
          c.passiveAttack = EffectId::ACID;
          c.bodyParts.clear();
          c.brain = false;
          c.noSleep = true;
          c.stationary = true;
          c.weight = 120;
          c.name = "acid jelly";);
    case CreatureId::ZOMBIE: 
      return CATTR(
          c.viewId = ViewId::ZOMBIE;
          c.speed = 60;
          c.size = CreatureSize::LARGE;
          c.strength = 14;
          c.dexterity = 13;
          c.barehandedDamage = 13;
          c.humanoid = true;
          c.brain = false;
          c.weight = 100;
          c.undead = true;
          c.spawnType = SpawnType::UNDEAD;
          c.minionTasks[MinionTask::TRAIN] = 4; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::GRAVE] = 1;
          c.name = "zombie";);
    case CreatureId::SKELETON: 
      return CATTR(
          c.viewId = ViewId::SKELETON;
          c.speed = 80;
          c.size = CreatureSize::LARGE;
          c.strength = 14;
          c.dexterity = 13;
          c.barehandedDamage = 13;
          c.humanoid = true;
          c.brain = false;
          c.weight = 50;
          c.undead = true;
          c.name = "skeleton";);
    case CreatureId::VAMPIRE_BAT:
    case CreatureId::VAMPIRE: 
      return CATTR(
          c.viewId = ViewId::VAMPIRE;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 17;
          c.dexterity = 17;
          c.barehandedDamage = 2;
          c.humanoid = true;
          c.weight = 100;
          c.undead = true;
          c.chatReactionFriendly = "curses all humans";
          c.chatReactionHostile = "\"Die!\"";
          c.spawnType = SpawnType::UNDEAD;
          c.skills.insert(SkillId::NIGHT_VISION);
          c.minionTasks[MinionTask::TRAIN] = 4; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::GRAVE] = 1;
          c.name = "vampire";);
    case CreatureId::VAMPIRE_LORD: 
      return INHERIT(VAMPIRE,
          c.viewId = ViewId::VAMPIRE_LORD;
          c.speed += 20;
          c.strength += 1;
          c.dexterity += 1;
          c.barehandedDamage += 4;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.minionTasks[MinionTask::STUDY] = 1;
          c.name = "vampire lord";);
      /*   case CreatureId::VAMPIRE_BAT: 
           return PCreature(new Shapechanger(
           ViewObject(ViewId::BAT, ViewLayer::CREATURE, "Bat"),
           tribe,
           CATTR(
           c.speed = 150;
           c.size = CreatureSize::SMALL;
           c.strength = 3;
           c.dexterity = 16;
           c.barehandedDamage = 12;
           c.humanoid = false;
           c.legs = 0;
           c.arms = 0;
           c.wings = 2;
           c.weight = 1;
           c.flyer = true;
           c.name = "bat";), {
           CreatureId::VAMPIRE}
           ));*/
    case CreatureId::MUMMY: 
      return CATTR(
          c.viewId = ViewId::MUMMY;
          c.speed = 60;
          c.size = CreatureSize::LARGE;
          c.strength = 12;
          c.dexterity = 13;
          c.barehandedDamage = 13;
          c.humanoid = true;
          c.weight = 100;
          c.brain = false;
          c.undead = true;
          c.spawnType = SpawnType::UNDEAD;
          c.minionTasks[MinionTask::TRAIN] = 4; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::GRAVE] = 1;
          c.name = "mummy";);
    case CreatureId::MUMMY_LORD: 
      return INHERIT(MUMMY,
          c.viewId = ViewId::MUMMY_LORD;
          c.strength += 4;
          c.dexterity += 2;
          c.weight = 120;
          c.chatReactionFriendly = "curses all gravediggers";
          c.chatReactionHostile = "\"Die!\"";
          c.name = NameGenerator::get(NameGeneratorId::AZTEC)->getNext(););
    case CreatureId::GOBLIN: 
      return CATTR(
          c.viewId = ViewId::GOBLIN;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 16;
          c.dexterity = 14;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.spawnType = SpawnType::HUMANOID;
          c.chatReactionFriendly = "curses all elves";
          c.chatReactionHostile = "\"Die!\"";
          c.minionTasks[MinionTask::WORKSHOP] = 1;
          c.minionTasks[MinionTask::TRAIN] = 4; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::LABORATORY] = 0.5; 
          c.minionTasks[MinionTask::SLEEP] = 1;
          c.name = "goblin";);
    case CreatureId::GOBLIN_SHAMAN:
      return INHERIT(GOBLIN,
          c.viewId = ViewId::GOBLIN_SHAMAN;
          c.minionTasks.clear();
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::LABORATORY] = 4; 
          c.minionTasks[MinionTask::STUDY] = 4;
          c.minionTasks[MinionTask::SLEEP] = 1;
          c.name = "goblin shaman";);
    case CreatureId::GREAT_GOBLIN: 
      return INHERIT(GOBLIN,
          c.viewId = ViewId::GREAT_GOBLIN;
          c.strength += 6;
          c.dexterity += 6;
          c.weight += 80;
          c.name = "great goblin";);
    case CreatureId::HARPY:
      return INHERIT(GOBLIN,
          c.viewId = ViewId::HARPY;
          c.speed = 120;
          c.strength -= 3;
          c.dexterity += 2;
          c.weight = 70;
          c.gender = Gender::female;
          c.minionTasks[MinionTask::WORKSHOP] = 0;
          c.bodyParts[BodyPart::WING] = 2;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.name = "harpy";);
    case CreatureId::GNOME: 
      return CATTR(
          c.viewId = ViewId::GNOME;
          c.speed = 80;
          c.size = CreatureSize::MEDIUM;
          c.strength = 12;
          c.dexterity = 13;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 45;
          c.spawnType = SpawnType::HUMANOID;
          c.chatReactionFriendly = "talks about digging";
          c.chatReactionHostile = "\"Die!\"";
          c.minionTasks[MinionTask::WORKSHOP] = 4;
          c.minionTasks[MinionTask::TRAIN] = 1; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::LABORATORY] = 2; 
          c.minionTasks[MinionTask::SLEEP] = 1;
          c.skills.insert(SkillId::DISARM_TRAPS);
          c.name = "gnome";);
    case CreatureId::IMP: 
      return CATTR(
          c.viewId = ViewId::IMP;
          c.speed = 200;
          c.size = CreatureSize::SMALL;
          c.strength = 8;
          c.dexterity = 15;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 30;
          c.courage = 0.1;
          c.carryAnything = true;
          c.skills.insert(SkillId::CONSTRUCTION);
          c.chatReactionFriendly = "talks about digging";
          c.chatReactionHostile = "\"Die!\"";
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.name = "imp";);
    case CreatureId::PRISONER:
      return INHERIT(IMP,
          c.viewId = ViewId::PRISONER;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.weight = 60;
          c.chatReactionFriendly = "talks about escape plans";
          c.minionTasks[MinionTask::PRISON] = 1;
          c.minionTasks[MinionTask::TORTURE] = 0.0001; 
          c.minionTasks[MinionTask::SACRIFICE] = 0.0001; 
          c.minionTasks[MinionTask::EXECUTE] = 0.0001;
          c.name = "prisoner";);
    case CreatureId::OGRE: 
      return CATTR(
          c.viewId = ViewId::OGRE;
          c.speed = 80;
          c.size = CreatureSize::LARGE;
          c.strength = 20;
          c.dexterity = 17;
          c.barehandedDamage = 6;
          c.humanoid = true;
          c.weight = 140;
          c.firstName = NameGenerator::get(NameGeneratorId::DEMON)->getNext();
          c.spawnType = SpawnType::HUMANOID;
          c.minionTasks[MinionTask::WORKSHOP] = 1;
          c.minionTasks[MinionTask::TRAIN] = 4; 
          c.minionTasks[MinionTask::WORSHIP] = 0.5; 
          c.minionTasks[MinionTask::SLEEP] = 1;
          c.name = "ogre";);
    case CreatureId::CHICKEN: 
      return CATTR(
          c.viewId = ViewId::CHICKEN;
          c.speed = 50;
          c.size = CreatureSize::SMALL;
          c.strength = 2;
          c.dexterity = 2;
          c.barehandedDamage = 5;
          c.humanoid = false;
          c.weight = 3;
          c.hatcheryAnimal = true;
          c.isFood = true;
          c.name = "chicken";);
    case CreatureId::LEPRECHAUN: 
      return CATTR(
          c.viewId = ViewId::LEPRECHAUN;
          c.speed = 160;
          c.size = CreatureSize::MEDIUM;
          c.strength = 10;
          c.dexterity = 16;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 35;
          c.courage = 20;
          c.skills.insert(SkillId::STEALING);
          c.chatReactionFriendly = "discusses the weather";
          c.chatReactionHostile = "discusses the weather";
          c.name = "leprechaun";);
    case CreatureId::DWARF: 
      return CATTR(
          c.viewId = ViewId::DWARF;
          c.speed = 80;
          c.size = CreatureSize::MEDIUM;
          c.firstName = NameGenerator::get(NameGeneratorId::DWARF)->getNext();
          c.strength = 21;
          c.dexterity = 16;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 90;
          c.chatReactionFriendly = "curses all goblins";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "dwarf";);
    case CreatureId::DWARF_BARON: 
      return INHERIT(DWARF,
          c.viewId = ViewId::DWARF_BARON;
          c.strength += 6;
          c.dexterity += 5;
          c.speed += 10;
          c.courage = 3;
          c.weight = 120;
          c.name = "dwarf baron";);
    case CreatureId::LIZARDMAN: 
      return CATTR(
          c.viewId = ViewId::LIZARDMAN;
          c.speed = 120;
          c.size = CreatureSize::MEDIUM;
          c.strength = 15;
          c.dexterity = 18;
          c.barehandedDamage = 7;
          c.barehandedAttack = AttackType::BITE;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.harmlessApply = true;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.humanoid = true;
          c.weight = 50;
          c.chatReactionFriendly = "curses all humans";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "lizardman";);
    case CreatureId::LIZARDLORD: 
      return INHERIT(LIZARDMAN,
          c.viewId = ViewId::LIZARDLORD;
          c.speed += 20;
          c.strength += 6;
          c.dexterity += 10;
          c.courage = 3;
          c.weight = 60;
          c.name = "lizardman chief";);
    case CreatureId::ELF: 
      return CATTR(
          c.viewId = ViewId::ELF;
          c.speed = 100;
          c.size = CreatureSize::MEDIUM;
          c.strength = 11;
          c.dexterity = 17;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 50;
          c.innocent = true;
          c.chatReactionFriendly = "curses all dwarves";
          c.chatReactionHostile = "\"Die!\"";
          c.spells.push_back(Creature::getSpell(SpellId::HEALING));
          c.skills.insert(SkillId::ELF_VISION);
          c.name = "elf";);
    case CreatureId::ELF_ARCHER: 
      return INHERIT(ELF,
          c.viewId = ViewId::ELF_ARCHER;
          c.speed += 20;
          c.innocent = false;
          c.skills.insert(SkillId::ARCHERY);
          c.name = "elven archer";);
    case CreatureId::ELF_CHILD: 
      return INHERIT(ELF,
          c.viewId = ViewId::ELF_CHILD;
          c.speed += 20;
          c.size = CreatureSize::SMALL;
          c.strength = 7;
          c.dexterity = 17;
          c.barehandedDamage = 0;
          c.weight = 25;
          c.spells.clear();
          c.name = "elf child";);
    case CreatureId::ELF_LORD: 
      return INHERIT(ELF_ARCHER,
          c.viewId = ViewId::ELF_LORD;
          c.speed += 20;
          c.strength += 3;
          c.dexterity += 3;
          c.healer = true;
          c.spells.push_back(Creature::getSpell(SpellId::HEALING));
          c.spells.push_back(Creature::getSpell(SpellId::SPEED_SELF));
          c.spells.push_back(Creature::getSpell(SpellId::STR_BONUS));
          c.name = "elf lord";);
    case CreatureId::HORSE: 
      return CATTR(
          c.viewId = ViewId::HORSE;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 10;
          c.dexterity = 17;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 500;
          c.animal = true;
          c.name = "horse";);
    case CreatureId::COW: 
      return CATTR(
          c.viewId = ViewId::COW;
          c.speed = 40;
          c.size = CreatureSize::LARGE;
          c.strength = 10;
          c.dexterity = 12;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 400;
          c.animal = true;
          c.name = "cow";);
    case CreatureId::SHEEP: 
      return CATTR(
          c.viewId = ViewId::SHEEP;
          c.speed = 80;
          c.size = CreatureSize::MEDIUM;
          c.strength = 5;
          c.dexterity = 12;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 40;
          c.animal = true;
          c.name = "sheep";);
    case CreatureId::PIG: 
      return CATTR(
          c.viewId = ViewId::PIG;
          c.speed = 60;
          c.size = CreatureSize::MEDIUM;
          c.strength = 12;
          c.dexterity = 8;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 150;
          c.animal = true;
          c.name = "pig";);
    case CreatureId::GOAT:
      return INHERIT(PIG,
          c.viewId = ViewId::GOAT;
          c.weight = 40;
          c.name = "goat";);
    case CreatureId::JACKAL: 
      return CATTR(
          c.viewId = ViewId::JACKAL;
          c.speed = 120;
          c.size = CreatureSize::SMALL;
          c.strength = 10;
          c.dexterity = 15;
          c.barehandedDamage = 2;
          c.humanoid = false;
          c.weight = 10;
          c.animal = true;
          c.name = "jackal";);
    case CreatureId::DEER: 
      return CATTR(
          c.viewId = ViewId::DEER;
          c.speed = 200;
          c.size = CreatureSize::LARGE;
          c.strength = 10;
          c.dexterity = 17;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 400;
          c.animal = true;
          c.name = "deer";);
    case CreatureId::BOAR: 
      return CATTR(
          c.viewId = ViewId::BOAR;
          c.speed = 180;
          c.size = CreatureSize::MEDIUM;
          c.strength = 18;
          c.dexterity = 15;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 200;
          c.animal = true;
          c.name = "boar";);
    case CreatureId::FOX: 
      return CATTR(
          c.viewId = ViewId::FOX;
          c.speed = 140;
          c.size = CreatureSize::SMALL;
          c.strength = 10;
          c.dexterity = 15;
          c.barehandedDamage = 2;
          c.innocent = true;
          c.humanoid = false;
          c.weight = 10;
          c.animal = true;
          c.name = "fox";);
    case CreatureId::CAVE_BEAR: 
      return CATTR(
          c.viewId = ViewId::BEAR;
          c.speed = 120;
          c.size = CreatureSize::MEDIUM;
          c.strength = 23;
          c.dexterity = 18;
          c.barehandedDamage = 14;
          c.weight = 250;
          c.humanoid = false;
          c.animal = true;
          c.spawnType = SpawnType::BEAST;
          c.name = "cave bear";);
    case CreatureId::RAT: 
      return CATTR(
          c.viewId = ViewId::RAT;
          c.speed = 180;
          c.size = CreatureSize::SMALL;
          c.strength = 2;
          c.dexterity = 12;
          c.barehandedDamage = 2;
          c.humanoid = false;
          c.weight = 1;
          c.animal = true;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "rat";);
    case CreatureId::SCORPION: 
      return CATTR(
          c.viewId = ViewId::SCORPION;
          c.speed = 100;
          c.size = CreatureSize::SMALL;
          c.strength = 9;
          c.dexterity = 13;
          c.barehandedDamage = 10;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.humanoid = false;
          c.weight = 0.3;
          c.bodyParts[BodyPart::ARM] = 0;
          c.bodyParts[BodyPart::LEG] = 8;
          c.animal = true;
          c.name = "scorpion";);
    case CreatureId::SPIDER: 
      return INHERIT(SCORPION,
          c.viewId = ViewId::SPIDER;
          c.name = "spider";);
    case CreatureId::FLY: 
      return CATTR(
          c.viewId = ViewId::FLY;
          c.speed = 150;
          c.size = CreatureSize::SMALL;
          c.strength = 1;
          c.dexterity = 14;
          c.barehandedDamage = 10;
          c.humanoid = false;
          c.weight = 0.1;
          c.bodyParts[BodyPart::ARM] = 0;
          c.bodyParts[BodyPart::LEG] = 6;
          c.bodyParts[BodyPart::WING] = 2;
          c.courage = 100;
          c.animal = true;
          c.name = "fly";);
    case CreatureId::SNAKE: 
      return CATTR(
          c.viewId = ViewId::SNAKE;
          c.speed = 100;
          c.size = CreatureSize::SMALL;
          c.strength = 2;
          c.dexterity = 14;
          c.barehandedDamage = 15;
          c.humanoid = false;
          c.weight = 2;
          c.animal = true;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.skills.insert(SkillId::SWIMMING);
          c.name = "snake";);
    case CreatureId::RAVEN: 
      return CATTR(
          c.viewId = ViewId::RAVEN;
          c.speed = 250;
          c.size = CreatureSize::SMALL;
          c.strength = 2;
          c.dexterity = 12;
          c.barehandedDamage = 0;
          c.humanoid = false;
          c.weight = 0.5;
          c.bodyParts[BodyPart::LEG] = 2;
          c.bodyParts[BodyPart::WING] = 2;
          c.animal = true;
          c.spawnType = SpawnType::BEAST;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.skills.insert(SkillId::ELF_VISION);
          c.minionTasks[MinionTask::EXPLORE] = 1;
          c.minionTasks[MinionTask::SLEEP] = 1;
          c.name = "raven";);
    case CreatureId::VULTURE: 
      return INHERIT(RAVEN,
          c.viewId = ViewId::VULTURE;
          c.speed = 80;
          c.weight = 5;
          c.name = "vulture";);
    case CreatureId::WOLF: 
      return CATTR(
          c.viewId = ViewId::WOLF;
          c.speed = 160;
          c.size = CreatureSize::MEDIUM;
          c.strength = 18;
          c.dexterity = 17;
          c.barehandedDamage = 12;
          c.humanoid = false;
          c.animal = true;
          c.weight = 35;
          c.skills.insert(SkillId::NIGHT_VISION);
          c.spawnType = SpawnType::BEAST;
          c.name = "wolf";);    
    case CreatureId::WEREWOLF:
      return INHERIT(WOLF,
          c.viewId = ViewId::WEREWOLF;
          c.speed = 100;
          c.humanoid = true;
          c.weight = 80;
          c.size = CreatureSize::LARGE;
          c.minionTasks[MinionTask::EXPLORE] = 1;
          c.minionTasks[MinionTask::TRAIN] = 1;
          c.minionTasks[MinionTask::SLEEP] = 1;
          c.name = "werewolf";);
    case CreatureId::DOG: 
      return INHERIT(WOLF,
          c.viewId = ViewId::DOG;
          c.strength -= 3;
          c.dexterity -= 3;
          c.weight -= 10;
          c.innocent = true;
          c.name = "dog";);
    case CreatureId::FIRE_SPHERE: 
      return CATTR(
          c.viewId = ViewId::FIRE_SPHERE;
          c.speed = 100;
          c.size = CreatureSize::LARGE;
          c.strength = 5;
          c.dexterity = 15;
          c.barehandedDamage = 10;
          c.humanoid = false;
          c.bodyParts.clear();
          c.uncorporal = true;
          c.breathing = false;
          c.brain = false;
          c.fireCreature = true;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.weight = 10;
          c.name = "fire sphere";);
    case CreatureId::FIRE_ELEMENTAL:
      return INHERIT(FIRE_SPHERE,
          c.viewId = ViewId::FIRE_ELEMENTAL;
          c.speed = 120;
          c.strength = 20;
          c.dexterity = 20;
          c.barehandedAttack = AttackType::HIT;
          c.name = "fire elemental";);
    case CreatureId::AIR_ELEMENTAL:
      return INHERIT(FIRE_ELEMENTAL,
          c.viewId = ViewId::AIR_ELEMENTAL;
          c.fireCreature = false;
          c.speed = 160;
          c.name = "air elemental";);
    case CreatureId::EARTH_ELEMENTAL:
      return INHERIT(AIR_ELEMENTAL,
          c.viewId = ViewId::EARTH_ELEMENTAL;
          c.speed = 80;
          c.uncorporal = false;
          c.strength = 30;
          c.name = "earth elemental";);
    case CreatureId::WATER_ELEMENTAL:
      return INHERIT(EARTH_ELEMENTAL,
          c.viewId = ViewId::WATER_ELEMENTAL;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "water elemental";);
    case CreatureId::ENT:
      return INHERIT(EARTH_ELEMENTAL,
          c.viewId = ViewId::ENT;
          c.skills.insert(SkillId::ELF_VISION);
          c.name = "ent";);
    case CreatureId::ANGEL:
      return INHERIT(KNIGHT,
          c.viewId = ViewId::ANGEL;
          c.uncorporal = true;
          c.name = "angel";);
    case CreatureId::KRAKEN: return getKrakenAttributes(ViewId::KRAKEN_HEAD);
    case CreatureId::NIGHTMARE: /*
                                   return PCreature(new Shapechanger(
                                   ViewObject(ViewId::NIGHTMARE, ViewLayer::CREATURE, "nightmare"),
                                   Tribe::get(TribeId::MONSTER),
                                   CATTR(
                                   c.speed = 100;
                                   c.size = CreatureSize::LARGE;
                                   c.strength = 5;
                                   c.dexterity = 25;
                                   c.barehandedDamage = 10;
                                   c.humanoid = false;
                                   c.arms = 0;
                                   c.legs = 0;
                                   c.flyer = true;
                                   c.weight = 100;
                                   c.name = "nightmare";), {
                                   CreatureId::CAVE_BEAR,
                                   CreatureId::WOLF,
                                   CreatureId::ZOMBIE,
                                   CreatureId::VAMPIRE,
                                   CreatureId::MUMMY,
                                   CreatureId::FIRE_SPHERE,
                                   CreatureId::SPECIAL_MONSTER,
                                   }));*/
    case CreatureId::BAT: 
      return INHERIT(RAVEN,
          c.viewId = ViewId::BAT;
          c.speed = 150;
          c.strength = 3;
          c.dexterity = 16;
          c.barehandedDamage = 12;
          c.weight = 1;
          c.name = "bat";);
    case CreatureId::DEATH: 
      return CATTR(
          c.viewId = ViewId::DEATH;
          c.speed = 95;
          c.size = CreatureSize::LARGE;
          c.strength = 100;
          c.dexterity = 35;
          c.barehandedDamage = 10;
          c.chatReactionFriendly = c.chatReactionHostile = "\"IN ORDER TO HAVE A CHANGE OF FORTUNE AT THE LAST MINUTE YOU HAVE TO TAKE YOUR FORTUNE TO THE LAST MINUTE.\"";
          c.humanoid = true;
          c.weight = 30;
          c.breathing = false;
          c.name = "Death";);
    default: FAIL << "This is not handled here " << int(id);
  }
  FAIL << "unhandled case";
  return CreatureAttributes([](CreatureAttributes&) {});
}

ControllerFactory getController(CreatureId id, MonsterAIFactory normalFactory) {
  switch (id) {
    case CreatureId::KRAKEN:
      return ControllerFactory([=](Creature* c) {
          return new KrakenController(c);
          });
    case CreatureId::FIRE_SPHERE:
      return ControllerFactory([=](Creature* c) {
          return new KamikazeController(c, normalFactory);
          });
    case CreatureId::ACID_MOUND:
      return Monster::getFactory(MonsterAIFactory::idle());
    case CreatureId::GREEN_DRAGON:
      return ControllerFactory([=](Creature* c) {
          return new GreenDragonController(c, normalFactory);
          });
    case CreatureId::RED_DRAGON:
      return ControllerFactory([=](Creature* c) {
          return new RedDragonController(c, normalFactory);
          });
    default: return Monster::getFactory(normalFactory);
  }
}
PCreature getElder(vector<pair<Skill*, double>> teachSkills, vector<pair<QuestId, int>> quests, Tribe* tribe,
    CreatureAttributes attributes, ControllerFactory factory) {
    return PCreature(new VillageElder(teachSkills, quests, tribe, attributes, factory));
}

PCreature get(CreatureId id, Tribe* tribe, MonsterAIFactory aiFactory) {
  ControllerFactory factory = Monster::getFactory(aiFactory);
  switch (id) {
    case CreatureId::SPECIAL_MONSTER:
      return getSpecial(NameGenerator::get(NameGeneratorId::CREATURE)->getNext(),
          tribe, false, factory, false);
    case CreatureId::SPECIAL_MONSTER_KEEPER:
      return getSpecial(NameGenerator::get(NameGeneratorId::CREATURE)->getNext(),
          tribe, false, factory, true);
    case CreatureId::SPECIAL_HUMANOID:
      return getSpecial(NameGenerator::get(NameGeneratorId::CREATURE)->getNext(),
          tribe, true, factory, false);
    case CreatureId::ELF_LORD:
      return PCreature(new VillageElder({},
            {{QuestId::BANDITS, Random.getRandom(80, 120)}},
            tribe, getAttributes(id), factory));
    case CreatureId::DWARF_BARON:
      return PCreature(new VillageElder({},
            {{QuestId::GOBLINS, Random.getRandom(300, 400)}},
            tribe, getAttributes(id), factory));
    case CreatureId::GREAT_GOBLIN:
      return PCreature(new VillageElder({},
            {{QuestId::DWARVES, Random.getRandom(300, 400)}},
            tribe, getAttributes(id), factory));
    case CreatureId::AVATAR:
      return PCreature(new VillageElder({},
            {{QuestId::CASTLE_CELLAR, Random.getRandom(80, 120)},
            {QuestId::DRAGON, Random.getRandom(180, 320)}},
            tribe, getAttributes(id), factory));
    case CreatureId::LIZARDLORD:
      return PCreature(new VillageElder({}, {},
            tribe, getAttributes(id), factory));
    default: return get(getAttributes(id), tribe, getController(id, aiFactory));
  }
}

ItemType randomHealing() {
  return chooseRandom({ItemType(ItemId::POTION, EffectId::HEAL), {ItemId::FIRST_AID_KIT}});
}

ItemType randomBackup() {
  return chooseRandom({{ItemId::SCROLL, EffectId::TELEPORT}, randomHealing()}, {1, 4});
}

ItemType randomArmor() {
  return chooseRandom({ItemId::LEATHER_ARMOR, ItemId::CHAIN_ARMOR}, {4, 1});
}

class ItemList {
  public:
  ItemList& maybe(double chance, ItemType id, int num = 1) {
    if (Random.getDouble() <= chance)
      add(id, num);
    return *this;
  }

  ItemList& maybe(double chance, const vector<ItemType>& ids) {
    if (Random.getDouble() <= chance)
      for (ItemType id : ids)
        add(id);
    return *this;
  }

  ItemList& add(ItemType id, int num = 1) {
    for (int i : Range(num))
      ret.push_back(id);
    return *this;
  }

  ItemList& add(vector<ItemType> ids) {
    for (ItemType id : ids)
      ret.push_back(id);
    return *this;
  }

  operator vector<ItemType>() {
    return ret;
  }

  private:
  vector<ItemType> ret;
};

vector<ItemType> getInventory(CreatureId id) {
  switch (id) {
    case CreatureId::ANGEL:
      return ItemList().add(ItemId::SPECIAL_SWORD);
    case CreatureId::KEEPER: 
      return ItemList()
        .add(ItemId::ROBE);
    case CreatureId::DEATH: 
      return ItemList()
        .add(ItemId::SCYTHE);
    case CreatureId::LEPRECHAUN: 
      return ItemList()
        .add({ItemId::SCROLL, EffectId::TELEPORT}, Random.getRandom(1, 4));
    case CreatureId::GNOME: 
      return ItemList()
        .add(chooseRandom({ItemId::KNIFE}))
        .maybe(0.3, ItemId::LEATHER_BOOTS)
        .maybe(0.05, ItemList().add(ItemId::BOW).add(ItemId::ARROW, Random.getRandom(20, 36)));
    case CreatureId::WARRIOR: 
      return ItemList()
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::SWORD)
        .add(ItemId::GOLD_PIECE, Random.getRandom(10, 20));
    case CreatureId::SHAMAN: 
      return ItemList()
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::SWORD);
    case CreatureId::LIZARDLORD: 
    case CreatureId::LIZARDMAN: 
      return ItemList().add(ItemId::LEATHER_ARMOR)
        .add(ItemId::GOLD_PIECE, Random.getRandom(10, 20));
    case CreatureId::ARCHER: 
      return ItemList()
        .add(ItemId::BOW).add(ItemId::ARROW, Random.getRandom(20, 36))
        .add(ItemId::KNIFE)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.getRandom(20, 50));
    case CreatureId::CASTLE_GUARD:
    case CreatureId::KNIGHT: 
      return ItemList()
        .add(ItemId::SWORD)
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.getRandom(30, 80));
    case CreatureId::DEVIL: 
      return ItemList().add(chooseRandom<ItemType>({
              {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)},
              {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)},
              {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)}}));
    case CreatureId::DARK_KNIGHT:
    case CreatureId::AVATAR: 
      return ItemList()
        .add(ItemId::SPECIAL_BATTLE_AXE)
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::IRON_HELM)
        .add(ItemId::IRON_BOOTS)
        .add({ItemId::POTION, EffectId::HEAL}, Random.getRandom(1, 4))
        .add(ItemId::GOLD_PIECE, Random.getRandom(200, 300));
    case CreatureId::OGRE: 
      return ItemList().add(ItemId::WAR_HAMMER);
    case CreatureId::BANDIT:
    case CreatureId::GOBLIN: 
      return ItemList()
        .add(chooseRandom({ItemId::SWORD}, {3}))
        .add(ItemId::LEATHER_ARMOR)
        .maybe(0.3, randomBackup());
    case CreatureId::GREAT_GOBLIN: 
      return ItemList()
        .add(chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
        .add(ItemId::IRON_HELM)
        .add(ItemId::IRON_BOOTS)
        .add(ItemId::CHAIN_ARMOR)
        .add(randomBackup())
        .add(ItemId::KNIFE, Random.getRandom(2, 5))
        .add(ItemId::GOLD_PIECE, Random.getRandom(100, 200));
    case CreatureId::DWARF: 
      return ItemList()
        .add(chooseRandom({ItemId::BATTLE_AXE, ItemId::WAR_HAMMER}, {1, 1}))
        .maybe(0.6, randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .maybe(0.5, ItemId::IRON_HELM)
        .maybe(0.3, ItemId::IRON_BOOTS)
        .add(ItemId::GOLD_PIECE, Random.getRandom(10, 30));
    case CreatureId::DWARF_BARON: 
      return ItemList()
        .add(chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
        .add(randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::IRON_BOOTS)
        .add(ItemId::IRON_HELM)
        .add(ItemId::GOLD_PIECE, Random.getRandom(100, 200));
    case CreatureId::ELF_LORD: 
      return ItemList()
        .add(ItemId::SPECIAL_ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::BOW)
        .add(ItemId::ARROW, Random.getRandom(20, 36))
        .add(randomBackup());
    case CreatureId::ELF_ARCHER: 
      return ItemList()
        .add(ItemId::ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::BOW)
        .add(ItemId::ARROW, Random.getRandom(20, 36))
        .add(randomBackup());
    case CreatureId::MUMMY_LORD: 
      return ItemList()
        .add(ItemId::GOLD_PIECE, Random.getRandom(100, 200)).add(
            chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER, ItemId::SPECIAL_SWORD}, {1, 1, 1}));
    case CreatureId::WITCH: 
      return ItemList()
        .add(ItemId::KNIFE)
        .add({
            {ItemId::POTION, EffectType(EffectId::HEAL)},
            {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)},
            {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)},
            {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)},
            {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)},
            {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON)},
            {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)}});
    default: return {};
  }
}

PCreature CreatureFactory::fromId(CreatureId id, Tribe* t, MonsterAIFactory factory) {
  return addInventory(get(id, t, factory), getInventory(id));
}

