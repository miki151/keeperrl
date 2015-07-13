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
#include "entity_set.h"
#include "effect.h"
#include "item_factory.h"
#include "creature_attributes.h"
#include "square.h"
#include "view_object.h"
#include "view_id.h"
#include "location.h"
#include "creature.h"
#include "model.h"
#include "name_generator.h"
#include "player_message.h"
#include "equipment.h"
#include "minion_task_map.h"
#include "spell_map.h"
#include "event.h"
#include "tribe.h"
#include "square_type.h"
#include "monster_ai.h"

template <class Archive> 
void CreatureFactory::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tribe)
    & SVAR(creatures)
    & SVAR(weights)
    & SVAR(unique)
    & SVAR(tribeOverrides)
    & SVAR(levelIncrease);
}

SERIALIZABLE(CreatureFactory);

SERIALIZATION_CONSTRUCTOR_IMPL(CreatureFactory);

CreatureFactory::SingleCreature::SingleCreature(Tribe* t, CreatureId i) : id(i), tribe(t) {}

bool CreatureFactory::SingleCreature::operator == (const SingleCreature& o) const {
  return tribe == o.tribe && id == o.id;
}

template <class Archive> 
void CreatureFactory::SingleCreature::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(tribe)
    & SVAR(id);
}

SERIALIZABLE(CreatureFactory::SingleCreature);

SERIALIZATION_CONSTRUCTOR_IMPL2(CreatureFactory::SingleCreature, SingleCreature);

CreatureFactory::CreatureFactory(const SingleCreature& s) : CreatureFactory(s.tribe, {s.id}, {1}, {}) {
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
          if (!(getCreature()->getPosition() + (v * i)).inRectangle(getCreature()->getLevel()->getBounds()))
            break;
          bool isWall = true;
          for (Square* square : getCreature()->getSquare(v * i)) {
            if (Creature* other = square->getCreature())
              if (other->getTribe() != myTribe) {
                if (!other->hasSkill(Skill::get(SkillId::DISARM_TRAPS))) {
                  direction = v;
                  stopped = false;
                  found = true;
                  getCreature()->getLevel()->getModel()->
                      onTrapTrigger(getCreature()->getPosition2());
                  getCreature()->monsterMessage(
                      PlayerMessage("The boulder starts rolling.", PlayerMessage::CRITICAL),
                      PlayerMessage("You hear a heavy boulder rolling.", PlayerMessage::CRITICAL));
                  break;
                } else {
                  other->you(MsgType::DISARM_TRAP, "");
                  getCreature()->getLevel()->getModel()->
                      onTrapDisarm(getCreature()->getPosition2(), other);
                  getCreature()->die();
                  return;
                }
              }
            if (square->canEnterEmpty(getCreature()))
              isWall = false;
          }
          if (isWall)
            break;
        }
        if (found)
          break;
      }
    }
    if (stopped) {
      getCreature()->wait().perform(getCreature());
      return;
    }
    for (Square* square : getCreature()->getSquare(direction))
      if (square->getStrength() < 300) {
        if (Creature* c = square->getCreature()) {
          if (!c->isCorporal()) {
            if (auto action = getCreature()->swapPosition(direction, true))
              action.perform(getCreature());
          } else {
            decreaseHealth(c->getSize());
            if (health < 0) {
              getCreature()->getLevel()->globalMessage(getCreature()->getPosition() + direction,
                  getCreature()->getName().the() + " crashes on the " + c->getName().the(),
                  "You hear a crash");
              getCreature()->die();
              c->bleed(Random.getDouble(0.1, 0.3));
              return;
            } else {
              c->you(MsgType::KILLED_BY, getCreature()->getName().the());
              c->die(getCreature());
            }
          }
        }
        if (auto action = getCreature()->destroy(direction, Creature::DESTROY))
          action.perform(getCreature());
      }
    if (auto action = getCreature()->move(direction))
      action.perform(getCreature());
    else for (Square* square : getCreature()->getSquare(direction)) {
      if (health >= 0.9 && square->canConstruct(SquareId::FLOOR)) {
        getCreature()->globalMessage("The " + square->getName() + " is destroyed!");
        while (!square->construct(SquareId::FLOOR)) {} // This should use destroy() probably
        if (auto action = getCreature()->move(direction))
          action.perform(getCreature());
        health = 0.1;
      } else {
        getCreature()->getLevel()->globalMessage(getCreature()->getPosition() + direction,
            getCreature()->getName().the() + " crashes on the " + square->getName(), "You hear a crash");
        getCreature()->die();
        return;
      }
    }
    double speed = getCreature()->getAttr(AttrType::SPEED);
    double deceleration = 0.1;
    speed -= deceleration * 100 * 100 / speed;
    if (speed < 30 && !getCreature()->isDead()) {
      if (myTribe) {
        getCreature()->die();
        return;
      }
      speed = 100;
      stopped = true;
      getCreature()->setStationary();
      myTribe = nullptr;
    }
    getCreature()->setBoulderSpeed(speed);
  }

  virtual void you(MsgType type, const string& param) override {
    string msg, msgNoSee;
    switch (type) {
      case MsgType::BURN: msg = getCreature()->getName().the() + " burns in the " + param; break;
      case MsgType::DROWN: msg = getCreature()->getName().the() + " falls into the " + param;
                           msgNoSee = "You hear a loud splash"; break;
      case MsgType::KILLED_BY: msg = getCreature()->getName().the() + " is destroyed by " + param; break;
      case MsgType::ENTER_PORTAL: msg = getCreature()->getName().the() + " disappears in the portal."; break;
      default: break;
    }
    if (!msg.empty())
      getCreature()->monsterMessage(msg, msgNoSee);
  }

  virtual void onBump(Creature* c) override {
    if (myTribe)
      return;
    Vec2 dir = getCreature()->getPosition() - c->getPosition();
    string it = c->canSee(getCreature()) ? getCreature()->getName().the() : "it";
    string something = c->canSee(getCreature()) ? getCreature()->getName().the() : "something";
    if (!getCreature()->move(dir)) {
      c->playerMessage(it + " won't move in this direction");
      return;
    }
    c->playerMessage("You push " + something);
    if (stopped || dir == direction) {
      direction = dir;
      getCreature()->setBoulderSpeed(100 * c->getAttr(AttrType::STRENGTH) / 30);
      stopped = false;
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Monster) 
      & SVAR(direction)
      & SVAR(stopped)
      & SVAR(myTribe);
  }

  SERIALIZATION_CONSTRUCTOR(BoulderController);

  private:
  Vec2 SERIAL(direction);
  bool SERIAL(stopped) = false;
  Tribe* SERIAL(myTribe) = nullptr;
  double health = 1;
};

class Boulder : public Creature {
  public:
  Boulder(const CreatureAttributes& attr, Tribe* tribe, Vec2 dir) : 
    Creature(tribe, attr, ControllerFactory([dir](Creature* c) { 
            return new BoulderController(c, dir); })) {}

  Boulder(const CreatureAttributes& attr, Tribe* myTribe) : 
    Creature(myTribe, attr, ControllerFactory([myTribe](Creature* c) { 
            return new BoulderController(c, myTribe); })) {}

  virtual vector<PItem> getCorpse() override {
    return ItemFactory::fromId(ItemId::ROCK, Random.get(10, 20));
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Creature);
  }
  
  SERIALIZATION_CONSTRUCTOR(Boulder);
};

PCreature CreatureFactory::getRollingBoulder(Vec2 direction, Tribe* tribe) {
  return PCreature(new Boulder(CATTR(
            c.viewId = ViewId::BOULDER;
            c.attr[AttrType::DEXTERITY] = 1;
            c.attr[AttrType::STRENGTH] = 1000;
            c.weight = 1000;
            c.humanoid = false;
            c.size = CreatureSize::LARGE;
            c.attr[AttrType::SPEED] = 200;
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.stationary = true;
            c.noSleep = true;
            c.invincible = true;
            c.breathing = false;
            c.brain = false;
            c.name = "boulder";), tribe, direction));
}

PCreature CreatureFactory::getGuardingBoulder(Tribe* tribe) {
  return PCreature(new Boulder(CATTR(
            c.viewId = ViewId::BOULDER;
            c.attr[AttrType::DEXTERITY] = 1;
            c.attr[AttrType::STRENGTH] = 1000;
            c.weight = 1000;
            c.humanoid = false;
            c.size = CreatureSize::LARGE;
            c.attr[AttrType::SPEED] = 140;
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
      c.attr[AttrType::SPEED] = 40;
      c.size = CreatureSize::LARGE;
      c.attr[AttrType::STRENGTH] = 15;
      c.attr[AttrType::DEXTERITY] = 15;
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

  virtual void you(MsgType type, const string& param) override {
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
      getCreature()->monsterMessage(msg, msgNoSee);
  }

  virtual void makeMove() override {
    int radius = 10;
    if (waitNow) {
      getCreature()->wait().perform(getCreature());
      waitNow = false;
      return;
    }
    for (Creature* c : spawns)
      if (c->isDead()) {
        ++numSpawns;
        removeElement(spawns, c);
        break;
      }
    if (held && ((held->getPosition() - getCreature()->getPosition()).length8() != 1 || held->isDead()))
      held = nullptr;
    if (held) {
      held->you(MsgType::HAPPENS_TO, getCreature()->getName().the() + " pulls");
      if (father) {
        held->setHeld(father->getCreature());
        father->held = held;
        getCreature()->die(nullptr, false);
        getCreature()->getLevel()->moveCreature(held, getCreature()->getPosition() - held->getPosition());
      } else {
        held->you(MsgType::ARE, "eaten by " + getCreature()->getName().the());
        held->die();
      }
    }
    bool isEnemy = false;
    for (Square* square : getCreature()->getSquares(
          Rectangle(Vec2(-radius, -radius), Vec2(radius + 1, radius + 1)).getAllSquares()))
        if (Creature * c = square->getCreature())
          if (getCreature()->canSee(c) && getCreature()->isEnemy(c) && !getCreature()->isStationary()) {
            Vec2 v = square->getPosition() - getCreature()->getPosition();
            isEnemy = true;
            if (numSpawns > 0) {
              if (v.length8() == 1) {
                if (ready) {
                  c->you(MsgType::HAPPENS_TO, getCreature()->getName().the() + " swings itself around");
                  c->setHeld(getCreature());
                  held = c;
                  unReady();
                } else
                  makeReady();
                break;
              }
              pair<Vec2, Vec2> dirs = v.approxL1();
              vector<Vec2> moves;
              if (getCreature()->getSafeSquare(dirs.first)->canEnter({{MovementTrait::WALK, MovementTrait::SWIM}}))
                moves.push_back(dirs.first);
              if (getCreature()->getSafeSquare(dirs.second)->canEnter({{MovementTrait::WALK, MovementTrait::SWIM}}))
                moves.push_back(dirs.second);
              if (!moves.empty()) {
                if (!ready) {
                  makeReady();
                } else {
                  Vec2 move = chooseRandom(moves);
                  ViewId viewId = getCreature()->getSafeSquare(move)->canEnter({MovementTrait::SWIM}) 
                    ? ViewId::KRAKEN_WATER : ViewId::KRAKEN_LAND;
                  PCreature spawn(new Creature(getCreature()->getTribe(), getKrakenAttributes(viewId),
                        ControllerFactory([=](Creature* c) {
                          return new KrakenController(c);
                          })));
                  spawns.push_back(spawn.get());
                  dynamic_cast<KrakenController*>(spawn->getController())->father = this;
                  getCreature()->getLevel()->addCreature(getCreature()->getPosition() + move, std::move(spawn));
                  --numSpawns;
                  unReady();
                }
              } else
                unReady();
              break;
            }
          }
    if (!isEnemy && spawns.size() == 0 && father && Random.roll(5)) {
      getCreature()->die(nullptr, false, false);
    }
    getCreature()->wait().perform(getCreature());
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
  }

  SERIALIZATION_CONSTRUCTOR(KrakenController);

  private:
  int SERIAL(numSpawns) = 0;
  bool SERIAL(waitNow) = true;
  bool SERIAL(ready) = false;
  Creature* SERIAL(held) = nullptr;
  vector<Creature*> SERIAL(spawns);
  KrakenController* SERIAL(father) = nullptr;
};

class KamikazeController : public Monster {
  public:
  KamikazeController(Creature* c, MonsterAIFactory f) : Monster(c, f) {}

  virtual void makeMove() override {
    for (Square* square : getCreature()->getSquares(Vec2::directions8()))
      if (Creature* c = square->getCreature())
        if (getCreature()->isEnemy(c) && getCreature()->canSee(c)) {
          getCreature()->monsterMessage(getCreature()->getName().the() + " explodes!");
          for (Square* square : c->getSquares(Vec2::directions8()))
            square->setOnFire(1);
          c->getSquare()->setOnFire(1);
          getCreature()->die(nullptr, false);
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
  ShopkeeperController(Creature* c, Location* area)
      : Monster(c, MonsterAIFactory::stayInLocation(area)), shopArea(area) {
  }

  virtual void makeMove() override {
    if (getCreature()->getLevel() != shopArea->getLevel()) {
      Monster::makeMove();
      return;
    }
    if (firstMove) {
      for (Vec2 v : shopArea->getAllSquares())
        for (Item* item : getCreature()->getLevel()->getSafeSquare(v)->getItems()) {
          myItems.insert(item);
          item->setShopkeeper(getCreature());
        }
      firstMove = false;
    }
    vector<const Creature*> creatures;
    for (Square* square : getCreature()->getLevel()->getSquares(
          shopArea->getAllSquares()))
      if (const Creature* c = square->getCreature()) {
        creatures.push_back(c);
        if (!prevCreatures.count(c) && !thieves.count(c) && !getCreature()->isEnemy(c)) {
          if (!debt.count(c))
            c->playerMessage("\"Welcome to " + *getCreature()->getFirstName() + "'s shop!\"");
          else {
            c->playerMessage("\"Pay your debt or... !\"");
            thiefCount.erase(c);
          }
        }
      }
    for (auto elem : debt) {
      const Creature* c = elem.first;
      if (!contains(creatures, c)) {
        c->playerMessage("\"Come back, you owe me " + toString(elem.second) + " zorkmids!\"");
        if (++thiefCount[c] == 4) {
          c->playerMessage("\"Thief! Thief!\"");
          getCreature()->getTribe()->onItemsStolen(c);
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
    getCreature()->pickUp(items, false).perform(getCreature());
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
    if (l == getCreature()->getLevel())
      if (shopArea->contains(position)) {
        for (Item* it : items) {
          it->setShopkeeper(getCreature());
          myItems.insert(it);
        }
      }
  }

  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items) {
    if (c->getLevel() == getCreature()->getLevel() && shopArea->contains(c->getPosition())) {
      for (const Item* item : items)
        if (myItems.contains(item)) {
          debt[c] += item->getPrice();
          unpaidItems[c].insert(item);
        }
    }
  }

  REGISTER_HANDLER(DropEvent, const Creature* c, const vector<Item*>& items) {
    if (c->getLevel() == getCreature()->getLevel() && shopArea->contains(c->getPosition())) {
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
  bool SERIAL(firstMove) = true;
};

template <class Archive>
void CreatureFactory::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, BoulderController);
  REGISTER_TYPE(ar, Boulder);
  REGISTER_TYPE(ar, KrakenController);
  REGISTER_TYPE(ar, KamikazeController);
  REGISTER_TYPE(ar, ShopkeeperController);
}

REGISTER_TYPES(CreatureFactory::registerTypes);

PCreature CreatureFactory::addInventory(PCreature c, const vector<ItemType>& items) {
  for (ItemType item : items) {
    PItem it = ItemFactory::fromId(item);
    c->take(std::move(it));
  }
  return c;
}

PCreature CreatureFactory::getShopkeeper(Location* shopArea, Tribe* tribe) {
  PCreature ret(new Creature(tribe,
      CATTR(
        c.viewId = ViewId::SHOPKEEPER;
        c.attr[AttrType::SPEED] = 100;
        c.size = CreatureSize::LARGE;
        c.attr[AttrType::STRENGTH] = 17;
        c.attr[AttrType::DEXTERITY] = 13;
        c.barehandedDamage = 13;
        c.humanoid = true;
        c.weight = 100;
        c.chatReactionFriendly = "complains about high import tax";
        c.chatReactionHostile = "\"Die!\"";
        c.name = "shopkeeper";
        c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();),
      ControllerFactory([shopArea](Creature* c) { 
          return new ShopkeeperController(c, shopArea); })));
  vector<ItemType> inventory(Random.get(100, 300), ItemId::GOLD_PIECE);
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

PCreature CreatureFactory::random() {
  return random(MonsterAIFactory::monster());
}

PCreature CreatureFactory::random(const MonsterAIFactory& actorFactory) {
  CreatureId id;
  if (unique.size() > 0) {
    id = unique.back();
    unique.pop_back();
  } else
    id = chooseRandom(creatures, weights);
  PCreature ret = fromId(id, getTribeFor(id), actorFactory);
  ret->increaseExpLevel(levelIncrease);
  return ret;
}

PCreature get(
    CreatureAttributes attr, 
    Tribe* tribe,
    ControllerFactory factory) {
  return PCreature(new Creature(tribe, attr, factory));
}

CreatureFactory& CreatureFactory::increaseLevel(double l) {
  levelIncrease += l;
  return *this;
}

CreatureFactory::CreatureFactory(Tribe* t, const vector<CreatureId>& c, const vector<double>& w,
    const vector<CreatureId>& u, EnumMap<CreatureId, Tribe*> overrides, double lIncrease)
    : tribe(t), creatures(c), weights(w), unique(u), tribeOverrides(overrides), levelIncrease(lIncrease) {
}

CreatureFactory CreatureFactory::humanVillage(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::PESEANT,
      CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW, CreatureId::PIG, CreatureId::DOG },
      { 2, 1, 1, 1, 1, 1, 1}, {});
}

CreatureFactory CreatureFactory::gnomeVillage(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::GNOME },
      { 1}, { CreatureId::GNOME_CHIEF});
}

CreatureFactory CreatureFactory::humanCastle(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER,
      CreatureId::PESEANT, CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW,
      CreatureId::PIG, CreatureId::DOG },
      { 10, 6, 2, 1, 1, 1, 1, 1, 1}, {CreatureId::AVATAR});
}

static optional<pair<CreatureFactory, CreatureFactory>> splashFactories;

void CreatureFactory::initSplash(Tribe* tribe) {
  splashFactories = chooseRandom<optional<pair<CreatureFactory, CreatureFactory>>>( {
      make_pair(CreatureFactory(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER}, { 1, 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::AVATAR)),
      make_pair(CreatureFactory(tribe, { CreatureId::WARRIOR}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::SHAMAN)),
      make_pair(CreatureFactory(tribe, { CreatureId::ELF_ARCHER}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::ELF_LORD)),
      make_pair(CreatureFactory(tribe, { CreatureId::DWARF}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::DWARF_BARON)),
      make_pair(CreatureFactory(tribe, { CreatureId::LIZARDMAN}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::LIZARDLORD)),
      });
}

CreatureFactory CreatureFactory::splashHeroes(Tribe* tribe) {
  if (!splashFactories)
    initSplash(tribe);
  return splashFactories->first;
}

CreatureFactory CreatureFactory::splashLeader(Tribe* tribe) {
  if (!splashFactories)
    initSplash(tribe);
  return splashFactories->second;
}

CreatureFactory CreatureFactory::splashMonsters(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::GNOME, CreatureId::GOBLIN, CreatureId::OGRE,
      CreatureId::SPECIAL_HUMANOID, CreatureId::SPECIAL_MONSTER_KEEPER, CreatureId::WOLF, CreatureId::CAVE_BEAR,
      CreatureId::BAT, CreatureId::WEREWOLF, CreatureId::ZOMBIE, CreatureId::VAMPIRE, CreatureId::DOPPLEGANGER,
      CreatureId::SUCCUBUS},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {}, {}, 25);
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

CreatureFactory::SingleCreature CreatureFactory::coffins(Tribe* tribe) {
  return SingleCreature(tribe, CreatureId::VAMPIRE);
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
  return CreatureFactory(tribe, { CreatureId::DWARF, CreatureId::DWARF_FEMALE}, { 2, 1},{ CreatureId::DWARF_BARON});
}

CreatureFactory CreatureFactory::splash(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::IMP}, { 1}, { CreatureId::KEEPER });
}

CreatureFactory CreatureFactory::orcTown(Tribe* tribe) {
  return CreatureFactory(tribe, { CreatureId::ORC, CreatureId::RAT}, {2, 1}, {CreatureId::GREAT_ORC});
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

CreatureFactory CreatureFactory::singleType(Tribe* tribe, CreatureId id) {
  return CreatureFactory(tribe, { id}, {1}, {});
}

PCreature getSpecial(const string& name, Tribe* tribe, bool humanoid, ControllerFactory factory, bool keeper) {
  RandomGen r;
  r.init(hash<string>()(name));
  PCreature c = get(CATTR(
        c.viewId = humanoid ? ViewId::SPECIAL_HUMANOID : ViewId::SPECIAL_BEAST;
        c.attr[AttrType::SPEED] = r.get(70, 150);
        c.size = chooseRandom({CreatureSize::SMALL, CreatureSize::MEDIUM, CreatureSize::LARGE}, {1, 1, 1});
        c.attr[AttrType::STRENGTH] = r.get(20, 26);
        c.attr[AttrType::DEXTERITY] = r.get(20, 26);
        c.barehandedDamage = r.get(5, 15);
        c.humanoid = humanoid;
        c.spawnType = humanoid ? SpawnType::HUMANOID : SpawnType::BEAST;
        if (humanoid) {
          c.skills.setValue(SkillId::WEAPON_MELEE, r.getDouble(0, 1));
          c.skills.setValue(SkillId::UNARMED_MELEE, r.getDouble(0, 1));
          c.skills.setValue(SkillId::ARCHERY, r.getDouble(0, 1));
          c.skills.setValue(SkillId::SORCERY, r.getDouble(0, 1));
        }
        c.weight = c.size == CreatureSize::LARGE ? r.get(80,120) : 
                   c.size == CreatureSize::MEDIUM ? r.get(40, 60) :
                   r.get(5, 20);
        if (*c.humanoid) {
          c.chatReactionFriendly = "\"I am the mighty " + name + "\"";
          c.chatReactionHostile = "\"I am the mighty " + name + ". Die!\"";
          c.minionTasks.setWorkshopTasks(1);
          c.minionTasks.setValue(MinionTask::TRAIN, 4); 
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.minionTasks.setValue(MinionTask::STUDY, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
        } else {
          c.chatReactionFriendly = c.chatReactionHostile = "The " + name + " snarls.";
        }
        c.name = name;
        c.speciesName = humanoid ? "legendary humanoid" : "legendary beast";
        c.firstName = NameGenerator::get(NameGeneratorId::DEMON)->getNext();
        if (!(*c.humanoid) && Random.roll(10)) {
          c.uncorporal = true;
          c.bodyParts.clear();
          c.attr[AttrType::STRENGTH] -= 5;
          c.attr[AttrType::DEXTERITY] += 10;
          c.barehandedDamage += 10;
        } else {
          if (r.roll(4)) {
            c.bodyParts[BodyPart::WING] = 2;
            c.permanentEffects[LastingEffect::FLYING] = 1;
          }
          if (*c.humanoid == false) {
            c.bodyParts[BodyPart::ARM] = r.roll(2) ? 2 : 0;
            c.bodyParts[BodyPart::LEG] = r.get(3) * 2;
            c.attr[AttrType::STRENGTH] += 5;
            c.attr[AttrType::DEXTERITY] += 5;
            c.barehandedDamage += 5;
            switch (Random.get(8)) {
              case 0: c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON); break;
              case 1: c.attackEffect = EffectId::FIRE; c.barehandedAttack = AttackType::HIT; break;
              default: break;
            }
          }
          if (Random.roll(10)) {
            c.undead = true;
            c.name = "undead " + (*c.name).bare();
          }
        }
        if (r.roll(3))
          c.skills.insert(SkillId::SWIMMING);
        ), tribe, factory);
  if (c->isHumanoid()) {
    if (Random.roll(400)) {
      c->take(ItemFactory::fromId(ItemId::BOW));
      c->take(ItemFactory::fromId(ItemId::ARROW, Random.get(20, 36)));
    } else
      c->take(ItemFactory::fromId(chooseRandom(
            {ItemId::SPECIAL_SWORD, ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER})));
  } else if (!keeper) {
    switch (Random.get(3)) {
      case 0:
        c->take(ItemFactory::fromId(
              chooseRandom({ItemId::WARNING_AMULET, ItemId::HEALING_AMULET, ItemId::DEFENSE_AMULET})));
        break;
      case 1:
        c->take(ItemFactory::fromId({ItemId::POTION,
              EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)},
              Random.get(3, 6)));
        break;
      case 2:
        c->take(ItemFactory::fromId(chooseRandom<ItemType>({
              {ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::STR_BONUS)},
              {ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS)}}), Random.get(3, 6)));
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
          c.attr[AttrType::SPEED] = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 15;
          c.attr[AttrType::DEXTERITY] = 15;
          c.barehandedDamage = 5;
          c.humanoid = true;
          c.name = "Keeper";
          c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();
          c.spells.add(SpellId::HEALING);
          c.attributeGain = 1;
          c.minionTasks.setValue(MinionTask::STUDY, 1);
          c.minionTasks.setValue(MinionTask::LABORATORY, 0.01); 
          c.minionTasks.setValue(MinionTask::TRAIN, 0.0001); 
          c.skills.setValue(SkillId::SORCERY, 0.2););
    case CreatureId::BANDIT: 
      return CATTR(
          c.viewId = ViewId::BANDIT;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 15;
          c.attr[AttrType::DEXTERITY] = 15;
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
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 15;
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
          c.attr[AttrType::STRENGTH] = 24;
          c.attr[AttrType::SPEED] = 100;
          c.attr[AttrType::DEXTERITY] = 30;
          c.barehandedDamage = 30;
          c.name = "ancient spirit";);
    case CreatureId::LOST_SOUL:
      return INHERIT(GHOST,
          c.attr[AttrType::STRENGTH] = 5;
          c.attr[AttrType::DEXTERITY] = 35;
          c.courage = 10;
          c.spawnType = SpawnType::DEMON;
          c.barehandedAttack = AttackType::POSSESS;
          c.minionTasks.setValue(MinionTask::RITUAL, 1);
          c.name = "ghost";);
    case CreatureId::SUCCUBUS:
      return INHERIT(GHOST,
          c.attr[AttrType::STRENGTH] = 5;
          c.viewId = ViewId::SUCCUBUS;
          c.spawnType = SpawnType::DEMON;
          c.minionTasks.setValue(MinionTask::COPULATE, 1);
          c.minionTasks.setValue(MinionTask::RITUAL, 1);
          c.gender = Gender::female;
          c.humanoid = true;
          c.bodyParts[BodyPart::HEAD] = 1;
          c.bodyParts[BodyPart::LEG] = 2;
          c.bodyParts[BodyPart::ARM] = 2;
          c.bodyParts[BodyPart::WING] = 2;
          c.courage = 0.0;
          c.name = EntityName("succubus", "succubi");
          );
    case CreatureId::DOPPLEGANGER:
      return INHERIT(GHOST,
          c.viewId = ViewId::DOPPLEGANGER;
          c.permanentEffects[LastingEffect::FLYING] = 0;
          c.minionTasks.setValue(MinionTask::CONSUME, 0.4);
          c.minionTasks.setValue(MinionTask::RITUAL, 1);
          c.spawnType = SpawnType::DEMON;
          c.skills.insert(SkillId::CONSUMPTION);
          c.name = "doppleganger";
          );
    case CreatureId::DEVIL: 
      return CATTR(
          c.viewId = ViewId::DEVIL;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 19;
          c.attr[AttrType::DEXTERITY] = 16;
          c.barehandedDamage = 10;
          c.humanoid = true;
          c.weight = 80;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "devil";);
    case CreatureId::DARK_KNIGHT: 
      return CATTR(
          c.viewId = ViewId::DARK_KNIGHT;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 22;
          c.attr[AttrType::DEXTERITY] = 19;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.name = "dark knight";);
    case CreatureId::WITCH: 
      return CATTR(
          c.viewId = ViewId::WITCH;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 12;
          c.firstName = "Cornelia";
          c.barehandedDamage = 6;
          c.humanoid = true;
          c.weight = 50;
          c.gender = Gender::female;
          c.chatReactionFriendly = "curses all humans";
          c.chatReactionHostile = "\"Die!\"";
          c.name = EntityName("witch", "witches"););
    case CreatureId::CYCLOPS: 
      return CATTR(
          c.viewId = ViewId::CYCLOPS;
          c.attr[AttrType::SPEED] = 90;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 33;
          c.attr[AttrType::DEXTERITY] = 23;
          c.barehandedDamage = 10;
          c.barehandedAttack = AttackType::EAT;
          c.humanoid = true;
          c.weight = 400;
          c.firstName = NameGenerator::get(NameGeneratorId::CYCLOPS)->getNext();
          c.name = EntityName("cyclops", "cyclopes"););
    case CreatureId::GREEN_DRAGON: 
      return CATTR(
          c.viewId = ViewId::GREEN_DRAGON;
          c.attr[AttrType::SPEED] = 90;
          c.size = CreatureSize::HUGE;
          c.attr[AttrType::STRENGTH] = 38;
          c.attr[AttrType::DEXTERITY] = 28;
          c.barehandedDamage = 10;
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
          c.attr[AttrType::SPEED] = 110;
          c.attr[AttrType::STRENGTH] = 47;
          c.fireCreature = true;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 0;
          c.speciesName = "red dragon";
          );
    case CreatureId::KNIGHT: 
      return CATTR(
          c.viewId = ViewId::KNIGHT;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 28;
          c.attr[AttrType::DEXTERITY] = 21;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.skills.setValue(SkillId::WEAPON_MELEE, 0.3);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "knight";);
    case CreatureId::CASTLE_GUARD: 
      return INHERIT(KNIGHT,
          c.viewId = ViewId::CASTLE_GUARD;
          c.name = "guard";);
    case CreatureId::AVATAR: 
      return INHERIT(KNIGHT,
          c.viewId = ViewId::AVATAR;
          c.attr[AttrType::STRENGTH] += 3;
          c.attr[AttrType::DEXTERITY] += 2;
          c.courage = 3;
          c.barehandedDamage += 5;
          c.skills.setValue(SkillId::WEAPON_MELEE, 1);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext(););
    case CreatureId::WARRIOR:
      return INHERIT(KNIGHT,
          c.viewId = ViewId::WARRIOR;
          c.attr[AttrType::STRENGTH] -= 3;
          c.attr[AttrType::DEXTERITY] -= 3;
          c.name = "warrior";);
    case CreatureId::SHAMAN:
      return INHERIT(WARRIOR,
          c.viewId = ViewId::SHAMAN;
          c.courage = 3;
          c.spells.add(SpellId::HEALING);
          c.spells.add(SpellId::SPEED_SELF);
          c.spells.add(SpellId::STR_BONUS);
          c.spells.add(SpellId::SUMMON_SPIRIT);
          c.spells.add(SpellId::STUN_RAY);
          c.spells.add(SpellId::BLAST);
          c.skills.setValue(SkillId::SORCERY, 1);
          c.skills.insert(SkillId::HEALING);
          c.name = "shaman";);
    case CreatureId::ARCHER: 
      return CATTR(
          c.viewId = ViewId::ARCHER;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 17;
          c.attr[AttrType::DEXTERITY] = 24;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.skills.setValue(SkillId::ARCHERY, 1);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "archer";);
    case CreatureId::PESEANT: 
      return CATTR(
          c.viewId = chooseRandom({ViewId::PESEANT, ViewId::PESEANT_WOMAN});
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 12;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.innocent = true;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Heeelp!\"";
          c.minionTasks.setValue(MinionTask::CROPS, 4);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "peasant";);
    case CreatureId::CHILD: 
      return INHERIT(PESEANT,
          c.viewId = ViewId::CHILD;
          c.attr[AttrType::SPEED] = 140;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 8;
          c.attr[AttrType::DEXTERITY] = 16;
          c.weight = 40;
          c.chatReactionFriendly = "\"plaaaaay!\"";
          c.chatReactionHostile = "\"Heeelp!\"";
          c.name = EntityName("child", "children"););
    case CreatureId::CLAY_GOLEM: 
      return CATTR(
          c.viewId = ViewId::CLAY_GOLEM;
          c.attr[AttrType::SPEED] = 50;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 12;
          c.barehandedDamage = 13;
          c.barehandedAttack = AttackType::PUNCH;
          c.humanoid = false;
          c.noSleep = true;
          c.brain = false;
          c.notLiving = true;
          c.weight = 100;
          c.minionTasks.setValue(MinionTask::TRAIN, 1);
          c.name = "clay golem";);
    case CreatureId::STONE_GOLEM: 
      return INHERIT(CLAY_GOLEM,
          c.viewId = ViewId::STONE_GOLEM;
          c.attr[AttrType::SPEED] += 10;
          c.attr[AttrType::STRENGTH] += 2;
          c.attr[AttrType::DEXTERITY] += 2;
          c.barehandedDamage += 2;
          c.weight += 20;
          c.name = "stone golem";);
    case CreatureId::IRON_GOLEM: 
      return INHERIT(STONE_GOLEM,
          c.viewId = ViewId::IRON_GOLEM;
          c.attr[AttrType::SPEED] += 10;
          c.attr[AttrType::STRENGTH] += 2;
          c.attr[AttrType::DEXTERITY] += 2;
          c.barehandedDamage += 2;
          c.name = "iron golem";);
    case CreatureId::LAVA_GOLEM: 
      return INHERIT(IRON_GOLEM,
          c.viewId = ViewId::LAVA_GOLEM;
          c.attr[AttrType::SPEED] += 10;
          c.attr[AttrType::STRENGTH] += 2;
          c.attr[AttrType::DEXTERITY] += 2;
          c.barehandedDamage += 2;
          c.attackEffect = EffectId::FIRE;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.name = "lava golem";);
    case CreatureId::ZOMBIE: 
      return CATTR(
          c.viewId = ViewId::ZOMBIE;
          c.attr[AttrType::SPEED] = 60;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 13;
          c.barehandedDamage = 13;
          c.humanoid = true;
          c.brain = false;
          c.weight = 100;
          c.undead = true;
          c.spawnType = SpawnType::UNDEAD;
          c.minionTasks.setValue(MinionTask::TRAIN, 4); 
          c.minionTasks.setValue(MinionTask::GRAVE, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.name = "zombie";);
    case CreatureId::SKELETON: 
      return CATTR(
          c.viewId = ViewId::SKELETON;
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 13;
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
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 17;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 2;
          c.humanoid = true;
          c.weight = 100;
          c.undead = true;
          c.chatReactionFriendly = "\"All men be cursed!\"";
          c.chatReactionHostile = "\"Die!\"";
          c.spawnType = SpawnType::UNDEAD;
          c.skills.insert(SkillId::NIGHT_VISION);
          c.minionTasks.setValue(MinionTask::TRAIN, 4); 
          c.minionTasks.setValue(MinionTask::STUDY, 1);
          c.minionTasks.setValue(MinionTask::GRAVE, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.skills.setValue(SkillId::SORCERY, 0.1);
          c.name = NameGenerator::get(NameGeneratorId::VAMPIRE)->getNext();
          c.name = "vampire";);
    case CreatureId::VAMPIRE_LORD: 
      return INHERIT(VAMPIRE,
          c.viewId = ViewId::VAMPIRE_LORD;
          c.attr[AttrType::SPEED] += 20;
          c.attr[AttrType::STRENGTH] += 1;
          c.attr[AttrType::DEXTERITY] += 1;
          c.barehandedDamage += 4;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.skills.setValue(SkillId::SORCERY, 0.5);
          c.name = "vampire lord";);
      /*   case CreatureId::VAMPIRE_BAT: 
           return PCreature(new Shapechanger(
           ViewObject(ViewId::BAT, ViewLayer::CREATURE, "Bat"),
           tribe,
           CATTR(
           c.attr[AttrType::SPEED] = 150;
           c.size = CreatureSize::SMALL;
           c.attr[AttrType::STRENGTH] = 3;
           c.attr[AttrType::DEXTERITY] = 16;
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
          c.attr[AttrType::SPEED] = 60;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 12;
          c.attr[AttrType::DEXTERITY] = 13;
          c.barehandedDamage = 13;
          c.humanoid = true;
          c.weight = 100;
          c.brain = false;
          c.undead = true;
          c.spawnType = SpawnType::UNDEAD;
          c.minionTasks.setValue(MinionTask::TRAIN, 4); 
          c.minionTasks.setValue(MinionTask::GRAVE, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.name = EntityName("mummy", "mummies"););
    case CreatureId::MUMMY_LORD: 
      return INHERIT(MUMMY,
          c.viewId = ViewId::MUMMY_LORD;
          c.attr[AttrType::STRENGTH] += 4;
          c.attr[AttrType::DEXTERITY] += 2;
          c.weight = 120;
          c.chatReactionFriendly = "curses all gravediggers";
          c.chatReactionHostile = "\"Die!\"";
          c.name = NameGenerator::get(NameGeneratorId::AZTEC)->getNext(););
    case CreatureId::ORC: 
      return CATTR(
          c.viewId = ViewId::ORC;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 16;
          c.attr[AttrType::DEXTERITY] = 14;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 100;
          c.spawnType = SpawnType::HUMANOID;
          c.chatReactionFriendly = "curses all elves";
          c.chatReactionHostile = "\"Die!\"";
          c.minionTasks.setWorkshopTasks(1);
          c.minionTasks.setValue(MinionTask::TRAIN, 4);
          c.minionTasks.setValue(MinionTask::LABORATORY, 0.5); 
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.skills.setValue(SkillId::WEAPON_MELEE, 0.3);
          c.firstName = NameGenerator::get(NameGeneratorId::ORC)->getNext();
          c.name = "orc";);
    case CreatureId::ORC_SHAMAN:
      return INHERIT(ORC,
          c.viewId = ViewId::ORC_SHAMAN;
          c.attr[AttrType::STRENGTH] -= 3;
          c.attr[AttrType::DEXTERITY] -= 3;
          c.minionTasks.clear();
          c.minionTasks.setValue(MinionTask::LABORATORY, 4); 
          c.minionTasks.setValue(MinionTask::STUDY, 4);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.skills.setValue(SkillId::SORCERY, 0.7);
          c.skills.setValue(SkillId::WEAPON_MELEE, 0.0);
          c.skills.insert(SkillId::HEALING);
          c.name = "orc shaman";);
    case CreatureId::GREAT_ORC: 
      return INHERIT(ORC,
          c.viewId = ViewId::GREAT_ORC;
          c.attr[AttrType::STRENGTH] += 6;
          c.attr[AttrType::DEXTERITY] += 6;
          c.weight += 80;
          c.name = "great orc";);
    case CreatureId::HARPY:
      return INHERIT(ORC,
          c.viewId = ViewId::HARPY;
          c.attr[AttrType::SPEED] = 120;
          c.attr[AttrType::STRENGTH] -= 3;
          c.attr[AttrType::DEXTERITY] += 2;
          c.weight = 70;
          c.gender = Gender::female;
          c.minionTasks.setWorkshopTasks(0);
          c.bodyParts[BodyPart::WING] = 2;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.skills.setValue(SkillId::ARCHERY, 1);
          c.name = EntityName("harpy", "harpies"););
    case CreatureId::GNOME: 
      return CATTR(
          c.viewId = ViewId::GNOME;
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 12;
          c.attr[AttrType::DEXTERITY] = 13;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 45;
          c.chatReactionFriendly = "talks about digging";
          c.chatReactionHostile = "\"Die!\"";
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "gnome";);
    case CreatureId::GNOME_CHIEF:
      return INHERIT(GNOME,
          c.viewId = ViewId::GNOME_BOSS;
          c.attr[AttrType::STRENGTH] += 3;
          c.attr[AttrType::DEXTERITY] += 3;
          c.name = "gnome chief";);
    case CreatureId::GOBLIN: 
      return CATTR(
          c.viewId = ViewId::GOBLIN;
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 12;
          c.attr[AttrType::DEXTERITY] = 13;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 45;
          c.spawnType = SpawnType::HUMANOID;
          c.chatReactionFriendly = "talks about crafting";
          c.chatReactionHostile = "\"Die!\"";
          c.minionTasks.setWorkshopTasks(4);
          c.minionTasks.setValue(MinionTask::TRAIN, 1); 
          c.minionTasks.setValue(MinionTask::LABORATORY, 0.5); 
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.skills.insert(SkillId::DISARM_TRAPS);
          c.firstName = NameGenerator::get(NameGeneratorId::ORC)->getNext();
          c.name = "goblin";);
    case CreatureId::IMP: 
      return CATTR(
          c.viewId = ViewId::IMP;
          c.attr[AttrType::SPEED] = 200;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 8;
          c.attr[AttrType::DEXTERITY] = 15;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 30;
          c.courage = 0.1;
          c.carryAnything = true;
          c.dontChase = true;
          c.cantEquip = true;
          c.skills.insert(SkillId::CONSTRUCTION);
          c.chatReactionFriendly = "talks about digging";
          c.chatReactionHostile = "\"Die!\"";
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.name = "imp";);
    case CreatureId::PRISONER:
      return CATTR(
          c.viewId = ViewId::PRISONER;
          c.attr[AttrType::SPEED] = 100;
          c.attr[AttrType::STRENGTH] = 8;
          c.attr[AttrType::DEXTERITY] = 15;
          c.size = CreatureSize::LARGE;
          c.weight = 60;
          c.humanoid = true;
          c.courage = 0.1;
          c.carryAnything = true;
          c.dontChase = true;
          c.cantEquip = true;
          c.skills.insert(SkillId::CONSTRUCTION);
          c.chatReactionFriendly = "talks about escape plans";
          c.minionTasks.setValue(MinionTask::PRISON, 1);
          c.minionTasks.setValue(MinionTask::TORTURE, 0.0001); 
          c.minionTasks.setValue(MinionTask::EXECUTE, 0.0001);
          c.name = "prisoner";);
    case CreatureId::OGRE: 
      return CATTR(
          c.viewId = ViewId::OGRE;
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 20;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 6;
          c.humanoid = true;
          c.weight = 140;
          c.firstName = NameGenerator::get(NameGeneratorId::DEMON)->getNext();
          c.spawnType = SpawnType::HUMANOID;
          c.minionTasks.setWorkshopTasks(1);
          c.minionTasks.setValue(MinionTask::TRAIN, 4); 
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.minionTasks.setValue(MinionTask::EAT, 5);
          c.skills.setValue(SkillId::WEAPON_MELEE, 0.3);
          c.name = "ogre";);
    case CreatureId::CHICKEN: 
      return CATTR(
          c.viewId = ViewId::CHICKEN;
          c.attr[AttrType::SPEED] = 50;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 2;
          c.attr[AttrType::DEXTERITY] = 2;
          c.barehandedDamage = 5;
          c.humanoid = false;
          c.weight = 3;
          c.isFood = true;
          c.name = "chicken";);
    case CreatureId::LEPRECHAUN: 
      return CATTR(
          c.viewId = ViewId::LEPRECHAUN;
          c.attr[AttrType::SPEED] = 160;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 10;
          c.attr[AttrType::DEXTERITY] = 16;
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
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::MEDIUM;
          c.firstName = NameGenerator::get(NameGeneratorId::DWARF)->getNext();
          c.attr[AttrType::STRENGTH] = 25;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 90;
          c.chatReactionFriendly = "curses all orcs";
          c.chatReactionHostile = "\"Die!\"";
          c.name = EntityName("dwarf", "dwarves"););
    case CreatureId::DWARF_FEMALE:
      return INHERIT(DWARF,
          c.viewId = ViewId::DWARF_FEMALE;
          c.gender = Gender::female;);
    case CreatureId::DWARF_BARON: 
      return INHERIT(DWARF,
          c.viewId = ViewId::DWARF_BARON;
          c.attr[AttrType::STRENGTH] += 6;
          c.attr[AttrType::DEXTERITY] += 5;
          c.attr[AttrType::SPEED] += 10;
          c.courage = 3;
          c.weight = 120;
          c.name = "dwarf baron";);
    case CreatureId::LIZARDMAN: 
      return CATTR(
          c.viewId = ViewId::LIZARDMAN;
          c.attr[AttrType::SPEED] = 120;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 15;
          c.attr[AttrType::DEXTERITY] = 18;
          c.barehandedDamage = 7;
          c.barehandedAttack = AttackType::BITE;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.harmlessApply = true;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.humanoid = true;
          c.weight = 50;
          c.chatReactionFriendly = "curses all humans";
          c.chatReactionHostile = "\"Die!\"";
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = EntityName("lizardman", "lizardmen"););
    case CreatureId::LIZARDLORD: 
      return INHERIT(LIZARDMAN,
          c.viewId = ViewId::LIZARDLORD;
          c.attr[AttrType::SPEED] += 20;
          c.attr[AttrType::STRENGTH] += 6;
          c.attr[AttrType::DEXTERITY] += 10;
          c.courage = 3;
          c.weight = 60;
          c.name = "lizardman chief";);
    case CreatureId::ELF: 
      return CATTR(
          c.viewId = ViewId::ELF;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 11;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 50;
          c.innocent = true;
          c.chatReactionFriendly = "curses all dwarves";
          c.chatReactionHostile = "\"Die!\"";
          c.spells.add(SpellId::HEALING);
          c.skills.insert(SkillId::ELF_VISION);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = EntityName("elf", "elves"););
    case CreatureId::ELF_ARCHER: 
      return INHERIT(ELF,
          c.viewId = ViewId::ELF_ARCHER;
          c.attr[AttrType::SPEED] += 20;
          c.innocent = false;
          c.skills.setValue(SkillId::ARCHERY, 1);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "elven archer";);
    case CreatureId::ELF_CHILD: 
      return INHERIT(ELF,
          c.viewId = ViewId::ELF_CHILD;
          c.attr[AttrType::SPEED] += 20;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 7;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 0;
          c.weight = 25;
          c.spells.clear();
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = EntityName("elf child", "elf children"););
    case CreatureId::ELF_LORD: 
      return INHERIT(ELF_ARCHER,
          c.viewId = ViewId::ELF_LORD;
          c.attr[AttrType::SPEED] += 20;
          c.attr[AttrType::STRENGTH] += 3;
          c.attr[AttrType::DEXTERITY] += 3;
          c.spells.add(SpellId::HEALING);
          c.spells.add(SpellId::SPEED_SELF);
          c.spells.add(SpellId::STR_BONUS);
          c.spells.add(SpellId::STUN_RAY);
          c.spells.add(SpellId::BLAST);
          c.skills.setValue(SkillId::WEAPON_MELEE, 1);
          c.skills.setValue(SkillId::SORCERY, 1);
          c.skills.insert(SkillId::HEALING);
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.name = "elf lord";);
    case CreatureId::HORSE: 
      return CATTR(
          c.viewId = ViewId::HORSE;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 10;
          c.attr[AttrType::DEXTERITY] = 13;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 500;
          c.animal = true;
          c.dontChase = true;
          c.name = "horse";);
    case CreatureId::COW: 
      return CATTR(
          c.viewId = ViewId::COW;
          c.attr[AttrType::SPEED] = 40;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 10;
          c.attr[AttrType::DEXTERITY] = 12;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 400;
          c.animal = true;
          c.dontChase = true;
          c.name = "cow";);
    case CreatureId::DONKEY: 
      return INHERIT(COW,
          c.viewId = ViewId::DONKEY;
          c.weight = 200;
          c.name = "donkey";);
    case CreatureId::PIG: 
      return CATTR(
          c.viewId = ViewId::PIG;
          c.attr[AttrType::SPEED] = 60;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 12;
          c.attr[AttrType::DEXTERITY] = 8;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 150;
          c.dontChase = true;
          c.animal = true;
          c.isFood = true;
          c.name = "pig";);
    case CreatureId::GOAT:
      return INHERIT(PIG,
          c.viewId = ViewId::GOAT;
          c.weight = 40;
          c.name = "goat";);
    case CreatureId::JACKAL: 
      return CATTR(
          c.viewId = ViewId::JACKAL;
          c.attr[AttrType::SPEED] = 120;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 10;
          c.attr[AttrType::DEXTERITY] = 15;
          c.barehandedDamage = 2;
          c.humanoid = false;
          c.weight = 10;
          c.animal = true;
          c.name = "jackal";);
    case CreatureId::DEER: 
      return CATTR(
          c.viewId = ViewId::DEER;
          c.attr[AttrType::SPEED] = 200;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 10;
          c.attr[AttrType::DEXTERITY] = 17;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 400;
          c.animal = true;
          c.dontChase = true;
          c.name = EntityName("deer", "deer"););
    case CreatureId::BOAR: 
      return CATTR(
          c.viewId = ViewId::BOAR;
          c.attr[AttrType::SPEED] = 180;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 18;
          c.attr[AttrType::DEXTERITY] = 15;
          c.humanoid = false;
          c.innocent = true;
          c.weight = 200;
          c.animal = true;
          c.dontChase = true;
          c.name = "boar";);
    case CreatureId::FOX: 
      return CATTR(
          c.viewId = ViewId::FOX;
          c.attr[AttrType::SPEED] = 140;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 10;
          c.attr[AttrType::DEXTERITY] = 15;
          c.barehandedDamage = 2;
          c.innocent = true;
          c.humanoid = false;
          c.weight = 10;
          c.animal = true;
          c.dontChase = true;
          c.name = EntityName("fox", "foxes"););
    case CreatureId::CAVE_BEAR: 
      return CATTR(
          c.viewId = ViewId::BEAR;
          c.attr[AttrType::SPEED] = 120;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 23;
          c.attr[AttrType::DEXTERITY] = 18;
          c.barehandedDamage = 11;
          c.weight = 250;
          c.humanoid = false;
          c.animal = true;
          c.spawnType = SpawnType::BEAST;
          c.minionTasks.setValue(MinionTask::EXPLORE_CAVES, 1);
          c.minionTasks.setValue(MinionTask::LAIR, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.name = "cave bear";);
    case CreatureId::RAT: 
      return CATTR(
          c.viewId = ViewId::RAT;
          c.attr[AttrType::SPEED] = 180;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 2;
          c.attr[AttrType::DEXTERITY] = 12;
          c.barehandedDamage = 2;
          c.humanoid = false;
          c.weight = 1;
          c.animal = true;
          c.dontChase = true;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "rat";);
    case CreatureId::SCORPION: 
      return CATTR(
          c.viewId = ViewId::SCORPION;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 9;
          c.attr[AttrType::DEXTERITY] = 13;
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
          c.attr[AttrType::SPEED] = 150;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 1;
          c.attr[AttrType::DEXTERITY] = 14;
          c.barehandedDamage = 10;
          c.humanoid = false;
          c.weight = 0.1;
          c.bodyParts[BodyPart::ARM] = 0;
          c.bodyParts[BodyPart::LEG] = 6;
          c.bodyParts[BodyPart::WING] = 2;
          c.courage = 100;
          c.dontChase = true;
          c.animal = true;
          c.name = EntityName("fly", "flies"););
    case CreatureId::SNAKE: 
      return CATTR(
          c.viewId = ViewId::SNAKE;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 2;
          c.attr[AttrType::DEXTERITY] = 14;
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
          c.attr[AttrType::SPEED] = 250;
          c.size = CreatureSize::SMALL;
          c.attr[AttrType::STRENGTH] = 2;
          c.attr[AttrType::DEXTERITY] = 12;
          c.barehandedDamage = 0;
          c.humanoid = false;
          c.weight = 0.5;
          c.bodyParts[BodyPart::LEG] = 2;
          c.bodyParts[BodyPart::WING] = 2;
          c.animal = true;
          c.dontChase = true;
          c.courage = 100;
          c.spawnType = SpawnType::BEAST;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.minionTasks.setValue(MinionTask::EXPLORE, 1);
          c.minionTasks.setValue(MinionTask::LAIR, 1);
          c.groupName = "flock";
          c.name = "raven";);
    case CreatureId::VULTURE: 
      return INHERIT(RAVEN,
          c.viewId = ViewId::VULTURE;
          c.attr[AttrType::SPEED] = 80;
          c.weight = 5;
          c.dontChase = true;
          c.name = "vulture";);
    case CreatureId::WOLF: 
      return CATTR(
          c.viewId = ViewId::WOLF;
          c.attr[AttrType::SPEED] = 160;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 18;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 12;
          c.humanoid = false;
          c.animal = true;
          c.weight = 35;
          c.groupName = "pack";
          c.skills.insert(SkillId::NIGHT_VISION);
          c.spawnType = SpawnType::BEAST;
          c.firstName = NameGenerator::get(NameGeneratorId::DOG)->getNext();
          c.minionTasks.setValue(MinionTask::EXPLORE_NOCTURNAL, 1);
          c.minionTasks.setValue(MinionTask::LAIR, 1);
          c.name = EntityName("wolf", "wolves"););    
    case CreatureId::WEREWOLF:
      return INHERIT(WOLF,
          c.viewId = ViewId::WEREWOLF;
          c.attr[AttrType::SPEED] = 100;
          c.humanoid = true;
          c.weight = 80;
          c.size = CreatureSize::LARGE;
          c.minionTasks.setValue(MinionTask::EXPLORE_NOCTURNAL, 1);
          c.minionTasks.setValue(MinionTask::TRAIN, 1);
          c.minionTasks.setValue(MinionTask::LAIR, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.skills.insert(SkillId::STEALTH);
          c.name = EntityName("werewolf", "werewolves"););    
    case CreatureId::DOG: 
      return INHERIT(WOLF,
          c.viewId = ViewId::DOG;
          c.attr[AttrType::STRENGTH] -= 3;
          c.attr[AttrType::DEXTERITY] -= 3;
          c.weight -= 10;
          c.innocent = true;
          c.name = "dog";);
    case CreatureId::FIRE_SPHERE: 
      return CATTR(
          c.viewId = ViewId::FIRE_SPHERE;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 5;
          c.attr[AttrType::DEXTERITY] = 15;
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
          c.attr[AttrType::SPEED] = 120;
          c.attr[AttrType::STRENGTH] = 20;
          c.attr[AttrType::DEXTERITY] = 20;
          c.barehandedAttack = AttackType::HIT;
          c.name = "fire elemental";);
    case CreatureId::AIR_ELEMENTAL:
      return INHERIT(FIRE_ELEMENTAL,
          c.viewId = ViewId::AIR_ELEMENTAL;
          c.fireCreature = false;
          c.attr[AttrType::SPEED] = 160;
          c.name = "air elemental";);
    case CreatureId::EARTH_ELEMENTAL:
      return INHERIT(AIR_ELEMENTAL,
          c.viewId = ViewId::EARTH_ELEMENTAL;
          c.attr[AttrType::SPEED] = 80;
          c.uncorporal = false;
          c.attr[AttrType::STRENGTH] = 25;
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
          c.name = "tree spirit";);
    case CreatureId::ANGEL:
      return INHERIT(KNIGHT,
          c.viewId = ViewId::ANGEL;
          c.uncorporal = true;
          c.name = "angel";);
    case CreatureId::KRAKEN: return getKrakenAttributes(ViewId::KRAKEN_HEAD);
    case CreatureId::BAT: 
      return INHERIT(RAVEN,
          c.viewId = ViewId::BAT;
          c.attr[AttrType::SPEED] = 150;
          c.attr[AttrType::STRENGTH] = 3;
          c.attr[AttrType::DEXTERITY] = 16;
          c.barehandedDamage = 2;
          c.skills.insert(SkillId::NIGHT_VISION);
          c.minionTasks.clear();
          c.minionTasks.setValue(MinionTask::EXPLORE_CAVES, 1);
          c.minionTasks.setValue(MinionTask::EXPLORE_NOCTURNAL, 1);
          c.minionTasks.setValue(MinionTask::LAIR, 1);
          c.weight = 1;
          c.name = "bat";);
    case CreatureId::DEATH: 
      return CATTR(
          c.viewId = ViewId::DEATH;
          c.attr[AttrType::SPEED] = 95;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 100;
          c.attr[AttrType::DEXTERITY] = 35;
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
    default: return Monster::getFactory(normalFactory);
  }
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
    default: return get(getAttributes(id), tribe, getController(id, aiFactory));
  }
}

ItemType randomHealing() {
  return chooseRandom({ItemType(ItemId::POTION, EffectId::HEAL), {ItemId::FIRST_AID_KIT}});
}

ItemType randomBackup() {
  return chooseRandom({{ItemId::SCROLL, EffectId::DECEPTION}, {ItemId::SCROLL, EffectId::TELEPORT},
      randomHealing()}, {1, 1, 8});
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
    case CreatureId::CYCLOPS: 
      return ItemList().add(ItemId::HEAVY_CLUB).add(ItemId::GOLD_PIECE, Random.get(200, 400));
    case CreatureId::GREEN_DRAGON:
      return ItemList().add(ItemId::GOLD_PIECE, Random.get(300, 500));
    case CreatureId::RED_DRAGON:
      return ItemList().add(ItemId::GOLD_PIECE, Random.get(600, 1000));
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
        .add({ItemId::SCROLL, EffectId::TELEPORT}, Random.get(1, 4));
    case CreatureId::GOBLIN: 
      return ItemList()
        .add(ItemId::CLUB)
        .maybe(0.3, ItemId::LEATHER_BOOTS);
    case CreatureId::WARRIOR: 
      return ItemList()
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::CLUB)
        .add(ItemId::GOLD_PIECE, Random.get(10, 20));
    case CreatureId::SHAMAN: 
      return ItemList()
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::CLUB);
    case CreatureId::LIZARDLORD: 
    case CreatureId::LIZARDMAN: 
      return ItemList().add(ItemId::LEATHER_ARMOR)
        .add(ItemId::GOLD_PIECE, Random.get(10, 20));
    case CreatureId::ARCHER: 
      return ItemList()
        .add(ItemId::BOW).add(ItemId::ARROW, Random.get(20, 36))
        .add(ItemId::KNIFE)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.get(20, 50));
    case CreatureId::CASTLE_GUARD:
    case CreatureId::KNIGHT: 
      return ItemList()
        .add(ItemId::SWORD)
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.get(30, 80));
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
        .add({ItemId::POTION, EffectId::HEAL}, Random.get(1, 4))
        .add(ItemId::GOLD_PIECE, Random.get(200, 300));
    case CreatureId::OGRE: 
      return ItemList().add(ItemId::HEAVY_CLUB);
    case CreatureId::BANDIT:
    case CreatureId::ORC: 
      return ItemList()
        .add(ItemId::SWORD)
        .maybe(0.3, randomBackup())
        .maybe(0.05, ItemList().add(ItemId::BOW).add(ItemId::ARROW, Random.get(20, 36)));
    case CreatureId::GREAT_ORC: 
      return ItemList()
        .add(chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
        .add(ItemId::IRON_HELM)
        .add(ItemId::IRON_BOOTS)
        .add(ItemId::CHAIN_ARMOR)
        .add(randomBackup())
        .add(ItemId::KNIFE, Random.get(2, 5))
        .add(ItemId::GOLD_PIECE, Random.get(100, 200));
    case CreatureId::DWARF: 
      return ItemList()
        .add(chooseRandom({ItemId::BATTLE_AXE, ItemId::WAR_HAMMER}, {1, 1}))
        .maybe(0.6, randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .maybe(0.5, ItemId::IRON_HELM)
        .maybe(0.3, ItemId::IRON_BOOTS)
        .add(ItemId::GOLD_PIECE, Random.get(10, 30));
    case CreatureId::DWARF_BARON: 
      return ItemList()
        .add(chooseRandom({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
        .add(randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::IRON_BOOTS)
        .add(ItemId::IRON_HELM)
        .add(ItemId::GOLD_PIECE, Random.get(200, 400));
    case CreatureId::GNOME_CHIEF:
      return ItemList()
        .add(ItemId::SWORD)
        .add(randomBackup());
    case CreatureId::ELF_LORD: 
      return ItemList()
        .add(ItemId::SPECIAL_ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::BOW)
        .add(ItemId::ARROW, Random.get(20, 36))
        .add(ItemId::GOLD_PIECE, Random.get(100, 300))
        .add(randomBackup());
    case CreatureId::ELF_ARCHER: 
      return ItemList()
        .add(ItemId::ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::BOW)
        .add(ItemId::ARROW, Random.get(20, 36))
        .add(ItemId::GOLD_PIECE, Random.get(10, 30))
        .add(randomBackup());
    case CreatureId::MUMMY_LORD: 
      return ItemList()
        .add(ItemId::GOLD_PIECE, Random.get(100, 200)).add(
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

PCreature CreatureFactory::fromId(CreatureId id, Tribe* t) {
  return fromId(id, t, MonsterAIFactory::monster());
}

PCreature CreatureFactory::fromId(CreatureId id, Tribe* t, const MonsterAIFactory& factory) {
  return addInventory(get(id, t, factory), getInventory(id));
}

