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
#include "view_object.h"
#include "view_id.h"
#include "location.h"
#include "creature.h"
#include "game.h"
#include "name_generator.h"
#include "player_message.h"
#include "equipment.h"
#include "minion_task_map.h"
#include "spell_map.h"
#include "event.h"
#include "tribe.h"
#include "square_type.h"
#include "monster_ai.h"
#include "sound.h"

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

CreatureFactory::SingleCreature::SingleCreature(TribeId t, CreatureId i) : id(i), tribe(t) {}

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
  BoulderController(Creature* c) : Monster(c, MonsterAIFactory::idle()), stopped(true) {}

  void decreaseHealth(CreatureSize size) {
    switch (size) {
      case CreatureSize::HUGE: health = -1; break;
      case CreatureSize::LARGE: health -= 0.3; break;
      case CreatureSize::MEDIUM: health -= 0.15; break;
      case CreatureSize::SMALL: break;
    }
  }

  void considerRolling() {
    for (Vec2 v : Vec2::directions4(Random)) {
      int radius = 4;
      for (int i = 1; i <= radius; ++i) {
        Position curPos = getCreature()->getPosition().plus(v * i);
        if (Creature* other = curPos.getCreature()) {
          if (other->getTribe() != getCreature()->getTribe()) {
            if (!other->hasSkill(Skill::get(SkillId::DISARM_TRAPS))) {
              direction = v;
              stopped = false;
              getCreature()->getGame()->onTrapTrigger(getCreature()->getPosition());
              getCreature()->monsterMessage(
                  PlayerMessage("The boulder starts rolling.", PlayerMessage::CRITICAL),
                  PlayerMessage("You hear a heavy boulder rolling.", PlayerMessage::CRITICAL));
              return;
            } else {
              other->you(MsgType::DISARM_TRAP, "");
              getCreature()->getGame()->onTrapDisarm(getCreature()->getPosition(), other);
              getCreature()->die();
              return;
            }
          }
        }
        if (!curPos.canEnterEmpty(getCreature()))
          break;
      }
    }
  }

  virtual void makeMove() override {
    if (stopped)
      considerRolling();
    if (stopped) {
      getCreature()->wait().perform(getCreature());
      return;
    }
    Position nextPos = getCreature()->getPosition().plus(direction);
    if (nextPos.getStrength() < 300) {
      if (Creature* c = nextPos.getCreature()) {
        if (!c->isCorporal()) {
          if (auto action = getCreature()->swapPosition(direction, true))
            action.perform(getCreature());
        } else {
          decreaseHealth(c->getSize());
          if (health < 0) {
            nextPos.globalMessage(getCreature()->getName().the() + " crashes on the " + c->getName().the(),
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
    else {
      if (health >= 0.9 && nextPos.canConstruct(SquareId::FLOOR)) {
        getCreature()->globalMessage("The " + nextPos.getName() + " is destroyed!");
        while (!nextPos.construct(SquareId::FLOOR)) {} // This should use destroy() probably
        if (auto action = getCreature()->move(direction))
          action.perform(getCreature());
        health = 0.1;
      } else {
        nextPos.globalMessage(getCreature()->getName().the() + " crashes on the " + nextPos.getName(),
            "You hear a crash");
        getCreature()->die();
        return;
      }
    }
    double speed = getCreature()->getAttr(AttrType::SPEED);
    double deceleration = 0.1;
    speed -= deceleration * 100 * 100 / speed;
    if (speed < 30 && !getCreature()->isDead()) {
      getCreature()->die();
      return;
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

  SERIALIZE_ALL2(Monster, direction, stopped);
  SERIALIZATION_CONSTRUCTOR(BoulderController);

  private:
  Vec2 SERIAL(direction);
  bool SERIAL(stopped) = false;
  double health = 1;
};

class Boulder : public Creature {
  public:
  Boulder(const CreatureAttributes& attr, TribeId tribe, ControllerFactory f) : 
    Creature(ViewObject(ViewId::BOULDER, ViewLayer::CREATURE, "Boulder")
        .setModifier(ViewObjectModifier::NO_UP_MOVEMENT)
        .setModifier(ViewObjectModifier::REMEMBER), tribe, attr, f) {}

  virtual vector<PItem> getCorpse() override {
    return ItemFactory::fromId(ItemId::ROCK, Random.get(10, 20));
  }

  SERIALIZE_SUBCLASS(Creature);
  SERIALIZATION_CONSTRUCTOR(Boulder);
};

PCreature CreatureFactory::getGuardingBoulder(TribeId tribe) {
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
            c.name = "boulder";), tribe, ControllerFactory([](Creature* c) { 
              return new BoulderController(c); })));
}

class SokobanController : public Monster {
  public:
  SokobanController(Creature* c) : Monster(c, MonsterAIFactory::idle()) {}

  virtual void onBump(Creature* player) override {
    Vec2 goDir = player->getPosition().getDir(getCreature()->getPosition());
    if (goDir.isCardinal4() && getCreature()->getPosition().plus(goDir).canEnter(
          getCreature()->getMovementType().setForced(true))) {
      getCreature()->displace(getCreature()->getLocalTime(), goDir);
      player->move(goDir).perform(player);
    }
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

  SERIALIZE_SUBCLASS(Monster);
  SERIALIZATION_CONSTRUCTOR(SokobanController);

  private:
};

static PCreature getSokobanBoulder(TribeId tribe) {
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
            c.name = "boulder";), tribe, ControllerFactory([](Creature* c) { 
              return new SokobanController(c); })));
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
    numSpawns = Random.choose({1, 2}, {4, 1});
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
    if (held && (held->getPosition().dist8(getCreature()->getPosition()) != 1 || held->isDead()))
      held = nullptr;
    if (held) {
      held->you(MsgType::HAPPENS_TO, getCreature()->getName().the() + " pulls");
      if (father) {
        held->setHeld(father->getCreature());
        father->held = held;
        Position newPos = getCreature()->getPosition();
        getCreature()->die(nullptr, false);
        held->getPosition().moveCreature(newPos);
      } else {
        held->you(MsgType::ARE, "eaten by " + getCreature()->getName().the());
        held->die();
      }
    }
    bool isEnemy = false;
    for (Position pos : getCreature()->getPosition().getRectangle(
          Rectangle(Vec2(-radius, -radius), Vec2(radius + 1, radius + 1))))
        if (Creature * c = pos.getCreature())
          if (getCreature()->canSee(c) && getCreature()->isEnemy(c) && !getCreature()->isStationary()) {
            Vec2 v = getCreature()->getPosition().getDir(pos);
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
              if (getCreature()->getPosition().plus(dirs.first).canEnter(
                    {{MovementTrait::WALK, MovementTrait::SWIM}}))
                moves.push_back(dirs.first);
              if (getCreature()->getPosition().plus(dirs.second).canEnter(
                    {{MovementTrait::WALK, MovementTrait::SWIM}}))
                moves.push_back(dirs.second);
              if (!moves.empty()) {
                if (!ready) {
                  makeReady();
                } else {
                  Vec2 move = Random.choose(moves);
                  ViewId viewId = getCreature()->getPosition().plus(move).canEnter({MovementTrait::SWIM}) 
                    ? ViewId::KRAKEN_WATER : ViewId::KRAKEN_LAND;
                  PCreature spawn(new Creature(getCreature()->getTribeId(), getKrakenAttributes(viewId),
                        ControllerFactory([=](Creature* c) {
                          return new KrakenController(c);
                          })));
                  spawns.push_back(spawn.get());
                  dynamic_cast<KrakenController*>(spawn->getController())->father = this;
                  getCreature()->getPosition().plus(move).addCreature(std::move(spawn));
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

  SERIALIZE_ALL2(Monster, numSpawns, waitNow, ready, held, spawns, father);
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
    for (Position pos : getCreature()->getPosition().neighbors8())
      if (Creature* c = pos.getCreature())
        if (getCreature()->isEnemy(c) && getCreature()->canSee(c)) {
          getCreature()->monsterMessage(getCreature()->getName().the() + " explodes!");
          for (Position v : c->getPosition().neighbors8())
            v.setOnFire(1);
          c->getPosition().setOnFire(1);
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
    if (!getCreature()->getPosition().isSameLevel(shopArea->getLevel())) {
      Monster::makeMove();
      return;
    }
    if (firstMove) {
      for (Position v : shopArea->getAllSquares()) {
        for (Item* item : v.getItems())
          item->setShopkeeper(getCreature());
        v.clearItemIndex(ItemIndex::FOR_SALE);
      }
      firstMove = false;
    }
    vector<const Creature*> creatures;
    for (Position v : shopArea->getAllSquares())
      if (const Creature* c = v.getCreature()) {
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

  virtual void onItemsGiven(vector<Item*> items, const Creature* from) override {
    int paid = filter(items, Item::classPredicate(ItemClass::GOLD)).size();
    if ((debt[from] -= paid) <= 0)
      debt.erase(from);
    for (Item* it : from->getEquipment().getItems())
      if (unpaidItems[from].contains(it) && it->getPrice() <= paid) {
        it->setShopkeeper(nullptr);
        paid -= it->getPrice();
      }
    if (unpaidItems[from].empty())
      unpaidItems.erase(from);
  }
  
  REGISTER_HANDLER(ItemsAppearedEvent, Position position, const vector<Item*>& items) {
    if (shopArea->contains(position)) {
      for (Item* it : items) {
        it->setShopkeeper(getCreature());
        position.clearItemIndex(ItemIndex::FOR_SALE);
      }
    }
  }

  REGISTER_HANDLER(PickupEvent, const Creature* c, const vector<Item*>& items) {
    if (shopArea->contains(c->getPosition())) {
      for (const Item* item : items)
        if (item->getShopkeeper() == getCreature()) {
          debt[c] += item->getPrice();
          unpaidItems[c].insert(item);
        }
    }
  }

  REGISTER_HANDLER(DropEvent, const Creature* c, const vector<Item*>& items) {
    if (shopArea->contains(c->getPosition())) {
      for (const Item* item : items)
        if (item->getShopkeeper() == getCreature()) {
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

  SERIALIZE_ALL2(Monster, prevCreatures, debt, thiefCount, thieves, unpaidItems, shopArea, firstMove);
  SERIALIZATION_CONSTRUCTOR(ShopkeeperController);

  private:
  unordered_set<const Creature*> SERIAL(prevCreatures);
  unordered_map<const Creature*, int> SERIAL(debt);
  unordered_map<const Creature*, int> SERIAL(thiefCount);
  unordered_set<const Creature*> SERIAL(thieves);
  unordered_map<const Creature*, EntitySet<Item>> SERIAL(unpaidItems);
  Location* SERIAL(shopArea);
  bool SERIAL(firstMove) = true;
};

template <class Archive>
void CreatureFactory::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, BoulderController);
  REGISTER_TYPE(ar, SokobanController);
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

PCreature CreatureFactory::getShopkeeper(Location* shopArea, TribeId tribe) {
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

TribeId CreatureFactory::getTribeFor(CreatureId id) {
  if (auto t = tribeOverrides[id])
    return *t;
  else
    return *tribe;
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
    id = Random.choose(creatures, weights);
  PCreature ret = fromId(id, getTribeFor(id), actorFactory);
  ret->increaseExpLevel(levelIncrease);
  return ret;
}

PCreature get(
    CreatureAttributes attr, 
    TribeId tribe,
    ControllerFactory factory) {
  return PCreature(new Creature(tribe, attr, factory));
}

CreatureFactory& CreatureFactory::increaseLevel(double l) {
  levelIncrease += l;
  return *this;
}

CreatureFactory::CreatureFactory(TribeId t, const vector<CreatureId>& c, const vector<double>& w,
    const vector<CreatureId>& u, EnumMap<CreatureId, optional<TribeId>> overrides, double lIncrease)
    : tribe(t), creatures(c), weights(w), unique(u), tribeOverrides(overrides), levelIncrease(lIncrease) {
}

CreatureFactory::CreatureFactory(const vector<tuple<CreatureId, double, TribeId>>& c, const vector<CreatureId>& u,
      double lIncrease) : unique(u),levelIncrease(lIncrease) {
  for (auto& elem : c) {
    creatures.push_back(get<0>(elem));
    weights.push_back(get<1>(elem));
    tribeOverrides[get<0>(elem)] = get<2>(elem);
  }
}

CreatureFactory CreatureFactory::humanVillage(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER,
      CreatureId::PESEANT, CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW,
      CreatureId::PIG, CreatureId::DOG },
      { 2, 6, 6, 4, 1, 1, 1, 1, 6}, {CreatureId::KNIGHT});
}

CreatureFactory CreatureFactory::humanPeaceful(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::PESEANT,
      CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW, CreatureId::PIG, CreatureId::DOG },
      { 2, 1, 1, 1, 1, 1, 1}, {});
}

CreatureFactory CreatureFactory::gnomeVillage(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::GNOME },
      { 1}, { CreatureId::GNOME_CHIEF});
}

CreatureFactory CreatureFactory::gnomeEntrance(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::GNOME }, {1});
}

CreatureFactory CreatureFactory::koboldVillage(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::KOBOLD }, {1});
}

CreatureFactory CreatureFactory::darkElfVillage(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::DARK_ELF, CreatureId::DARK_ELF_CHILD, CreatureId::DARK_ELF_WARRIOR },
      { 1, 1, 2}, { CreatureId::DARK_ELF_LORD});
}

CreatureFactory CreatureFactory::darkElfEntrance(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::DARK_ELF_WARRIOR }, {1});
}

CreatureFactory CreatureFactory::humanCastle(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER,
      CreatureId::PESEANT, CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW,
      CreatureId::PIG, CreatureId::DOG },
      { 10, 6, 2, 1, 1, 1, 1, 1, 1}, {CreatureId::AVATAR});
}

static optional<pair<CreatureFactory, CreatureFactory>> splashFactories;

void CreatureFactory::initSplash(TribeId tribe) {
  splashFactories = Random.choose<optional<pair<CreatureFactory, CreatureFactory>>>( {
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

CreatureFactory CreatureFactory::splashHeroes(TribeId tribe) {
  if (!splashFactories)
    initSplash(tribe);
  return splashFactories->first;
}

CreatureFactory CreatureFactory::splashLeader(TribeId tribe) {
  if (!splashFactories)
    initSplash(tribe);
  return splashFactories->second;
}

CreatureFactory CreatureFactory::splashMonsters(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::GNOME, CreatureId::GOBLIN, CreatureId::OGRE,
      CreatureId::SPECIAL_HL, CreatureId::SPECIAL_BL, CreatureId::WOLF, CreatureId::CAVE_BEAR,
      CreatureId::BAT, CreatureId::WEREWOLF, CreatureId::ZOMBIE, CreatureId::VAMPIRE, CreatureId::DOPPLEGANGER,
      CreatureId::SUCCUBUS},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {}, {}, 25);
}

CreatureFactory CreatureFactory::elvenVillage(TribeId tribe) {
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

CreatureFactory CreatureFactory::forrest(TribeId tribe) {
  return CreatureFactory(tribe,
      { CreatureId::DEER, CreatureId::FOX, CreatureId::BOAR, CreatureId::LEPRECHAUN },
      { 4, 2, 2, 1}, {});
}

CreatureFactory CreatureFactory::crypt(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ZOMBIE}, { 1}, {});
}

CreatureFactory::SingleCreature CreatureFactory::coffins(TribeId tribe) {
  return SingleCreature(tribe, CreatureId::VAMPIRE);
}

CreatureFactory CreatureFactory::vikingTown(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::WARRIOR}, { 1}, {CreatureId::SHAMAN});
}

CreatureFactory CreatureFactory::lizardTown(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::LIZARDMAN, }, { 1}, {CreatureId::LIZARDLORD});
}

CreatureFactory CreatureFactory::dwarfTown(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::DWARF, CreatureId::DWARF_FEMALE}, { 2, 1},{ CreatureId::DWARF_BARON});
}

CreatureFactory CreatureFactory::antNest(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ANT_WORKER, CreatureId::ANT_SOLDIER}, { 2, 1},
      { CreatureId::ANT_QUEEN});
}

CreatureFactory CreatureFactory::splash(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::IMP}, { 1}, { CreatureId::KEEPER });
}

CreatureFactory CreatureFactory::orcTown(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ORC, CreatureId::OGRE }, {1, 1});
}

CreatureFactory CreatureFactory::pyramid(TribeId tribe, int level) {
  if (level == 2)
    return CreatureFactory(tribe, { CreatureId::MUMMY }, {1}, { CreatureId::MUMMY_LORD });
  else
    return CreatureFactory(tribe, { CreatureId::MUMMY }, {1}, { });
}

CreatureFactory CreatureFactory::insects(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::SPIDER}, {1});
}

CreatureFactory CreatureFactory::waterCreatures(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::KRAKEN }, {1});
}

CreatureFactory CreatureFactory::elementals(TribeId tribe) {
  return CreatureFactory(tribe, {CreatureId::AIR_ELEMENTAL, CreatureId::FIRE_ELEMENTAL, CreatureId::WATER_ELEMENTAL,
      CreatureId::EARTH_ELEMENTAL}, {1, 1, 1, 1}, {});
}

CreatureFactory CreatureFactory::lavaCreatures(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::FIRE_SPHERE }, {1}, { });
}

CreatureFactory CreatureFactory::singleType(TribeId tribe, CreatureId id) {
  return CreatureFactory(tribe, { id}, {1}, {});
}

CreatureFactory CreatureFactory::gnomishMines(TribeId peaceful, TribeId enemy, int level) {
  return CreatureFactory({
      make_tuple(CreatureId::BANDIT, 100., enemy),
      make_tuple(CreatureId::GREEN_DRAGON, 5., enemy),
      make_tuple(CreatureId::RED_DRAGON, 5., enemy),
      make_tuple(CreatureId::CYCLOPS, 15., enemy),
      make_tuple(CreatureId::WITCH, 15., enemy),
      make_tuple(CreatureId::CLAY_GOLEM, 20., enemy),
      make_tuple(CreatureId::STONE_GOLEM, 20., enemy),
      make_tuple(CreatureId::IRON_GOLEM, 20., enemy),
      make_tuple(CreatureId::LAVA_GOLEM, 20., enemy),
      make_tuple(CreatureId::FIRE_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::WATER_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::EARTH_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::AIR_ELEMENTAL, 10., enemy),
      make_tuple(CreatureId::GNOME, 100., peaceful),
      make_tuple(CreatureId::GNOME_CHIEF, 20., peaceful),
      make_tuple(CreatureId::DWARF, 100., enemy),
      make_tuple(CreatureId::DWARF_FEMALE, 40., enemy),
      make_tuple(CreatureId::JACKAL, 200., enemy),
      make_tuple(CreatureId::BAT, 200., enemy),
      make_tuple(CreatureId::SNAKE, 150., enemy),
      make_tuple(CreatureId::SPIDER, 200., enemy),
      make_tuple(CreatureId::FLY, 100., enemy),
      make_tuple(CreatureId::RAT, 100., enemy)});
}

static ViewId getViewId(bool humanoid, bool large, bool body, bool wings) {
  static vector<ViewId> specialViewIds {
    ViewId::SPECIAL_BLBN,
    ViewId::SPECIAL_BLBW,
    ViewId::SPECIAL_BLGN,
    ViewId::SPECIAL_BLGW,
    ViewId::SPECIAL_BMBN,
    ViewId::SPECIAL_BMBW,
    ViewId::SPECIAL_BMGN,
    ViewId::SPECIAL_BMGW,
    ViewId::SPECIAL_HLBN,
    ViewId::SPECIAL_HLBW,
    ViewId::SPECIAL_HLGN,
    ViewId::SPECIAL_HLGW,
    ViewId::SPECIAL_HMBN,
    ViewId::SPECIAL_HMBW,
    ViewId::SPECIAL_HMGN,
    ViewId::SPECIAL_HMGW,
  };
  return specialViewIds[humanoid * 8 + (!large) * 4 + (!body) * 2 + wings];
}

static string getSpeciesName(bool humanoid, bool large, bool body, bool wings) {
  static vector<string> names {
    "devitablex",
    "owlbeast",
    "hellar dra",
    "marilisk",
    "gelaticorn",
    "mant eatur",
    "phanticore",
    "yeth horro",
    "yeth amon",
    "mantic dra",
    "unic cread",
    "under hulk",
    "nightshasa",
    "manananggal",
    "dire spawn",
    "shamander",
  };
  return names[humanoid * 8 + (!large) * 4 + (!body) * 2 + wings];
}

static optional<EffectType> getSpecialBeastAttack(bool large, bool body, bool wings) {
  static vector<optional<EffectType>> attacks {
    none,
    EffectType(EffectId::FIRE),
    EffectType(EffectId::FIRE),
    none,
    EffectType(EffectId::LASTING, LastingEffect::POISON),
    none,
    EffectType(EffectId::LASTING, LastingEffect::POISON),
    none,
  };
  return attacks[(!large) * 4 + (!body) * 2 + wings];
}

static EnumMap<BodyPart, int> getSpecialBeastBody(bool large, bool body, bool wings) {
  static vector<EnumMap<BodyPart, int>> parts {
    {
      { BodyPart::LEG, 2}},
    {
      { BodyPart::ARM, 2},
      { BodyPart::LEG, 2},
      { BodyPart::WING, 2},
      { BodyPart::HEAD, 1}},
    {
      { BodyPart::LEG, 4},
      { BodyPart::HEAD, 1}},
    {
      { BodyPart::ARM, 2},
      { BodyPart::WING, 2},
      { BodyPart::HEAD, 1}},
    {},
    { 
      { BodyPart::LEG, 2},
      { BodyPart::WING, 2},
      { BodyPart::HEAD, 1}},
    {
      { BodyPart::LEG, 8},
      { BodyPart::HEAD, 1}},
    { 
      { BodyPart::WING, 2},
      { BodyPart::HEAD, 1}},
  };
  return parts[(!large) * 4 + (!body) * 2 + wings];
}

PCreature getSpecial(TribeId tribe, bool humanoid, bool large, ControllerFactory factory) {
  bool wings = Random.roll(2);
  bool body = Random.roll(2);
  string name = getSpeciesName(humanoid, large, body, wings);
  PCreature c = get(CATTR(
        c.viewId = getViewId(humanoid, large, body, wings);
        c.isSpecial = true;
        c.attr[AttrType::SPEED] = Random.get(80, 120);
        if (!large)
          c.attr[AttrType::SPEED] += 20;
        c.size = large ? CreatureSize::LARGE : CreatureSize::MEDIUM;
        c.attr[AttrType::STRENGTH] = Random.get(18, 24);
        c.attr[AttrType::DEXTERITY] = Random.get(18, 24);
        if (large) {
          c.attr[AttrType::STRENGTH] += 6;
          c.attr[AttrType::DEXTERITY] -= 2;
        }
        c.barehandedDamage = Random.get(5, 15);
        c.humanoid = humanoid;
        c.spawnType = humanoid ? SpawnType::HUMANOID : SpawnType::BEAST;
        if (humanoid) {
          c.skills.setValue(SkillId::WEAPON_MELEE, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::UNARMED_MELEE, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::ARCHERY, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::SORCERY, Random.getDouble(0, 1));
        }
        c.weight = c.size == CreatureSize::LARGE ? Random.get(80,120) : 
                   c.size == CreatureSize::MEDIUM ? Random.get(40, 60) :
                   Random.get(5, 20);
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
        if (!body) {
          c.uncorporal = true;
          c.attr[AttrType::STRENGTH] -= 5;
          c.attr[AttrType::DEXTERITY] += 10;
          c.barehandedDamage += 10;
        }
        if (wings) {
          c.bodyParts[BodyPart::WING] = 2;
          c.permanentEffects[LastingEffect::FLYING] = 1;
        }
        if (!humanoid) {
          c.bodyParts = getSpecialBeastBody(large, body, wings);
          c.attr[AttrType::STRENGTH] += 5;
          c.attr[AttrType::DEXTERITY] += 5;
          c.barehandedDamage += 5;
          c.attackEffect = getSpecialBeastAttack(large, body, wings);
        }
        if (Random.roll(3))
          c.skills.insert(SkillId::SWIMMING);
        ), tribe, factory);
  if (c->isHumanoid()) {
    if (Random.roll(400)) {
      c->take(ItemFactory::fromId(ItemId::BOW));
      c->take(ItemFactory::fromId(ItemId::ARROW, Random.get(20, 36)));
    } else
      c->take(ItemFactory::fromId(Random.choose(
            {ItemId::SPECIAL_SWORD, ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER})));
  }
  return c;
}

#define INHERIT(ID, X) CreatureAttributes([&](CreatureAttributes& c) { c = getAttributes(CreatureId::ID); X })

CreatureAttributes getAttributes(CreatureId id) {
  switch (id) {
    case CreatureId::KEEPER: 
      return CATTR(
          c.viewId = ViewId::KEEPER;
          c.attr[AttrType::SPEED] = 100;
          c.weight = 90;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 85;
          c.attr[AttrType::DEXTERITY] = 85;
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
 //         c.skills.insert(SkillId::DISARM_TRAPS);
          c.name = "bandit";);
    case CreatureId::GHOST: 
      return CATTR(
          c.viewId = ViewId::GHOST;
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 14;
          c.attr[AttrType::DEXTERITY] = 35;
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
    case CreatureId::WITCHMAN: 
      return CATTR(
          c.viewId = ViewId::WITCHMAN;
          c.attr[AttrType::SPEED] = 140;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 20;
          c.attr[AttrType::DEXTERITY] = 20;
          c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();
          c.barehandedDamage = 6;
          c.humanoid = true;
          c.weight = 100;
          c.gender = Gender::male;
          c.chatReactionFriendly = "curses all monsters";
          c.chatReactionHostile = "\"Die!\"";
          c.name = EntityName("witchman", "witchmen"););
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
    case CreatureId::MINOTAUR: 
      return CATTR(
          c.viewId = ViewId::MINOTAUR;
          c.attr[AttrType::SPEED] = 90;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 45;
          c.attr[AttrType::DEXTERITY] = 35;
          c.barehandedDamage = 20;
          c.barehandedAttack = AttackType::EAT;
          c.humanoid = true;
          c.weight = 400;
          c.name = "minotaur";);
    case CreatureId::HYDRA: 
      return CATTR(
          c.viewId = ViewId::HYDRA;
          c.attr[AttrType::SPEED] = 110;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 35;
          c.attr[AttrType::DEXTERITY] = 45;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.harmlessApply = true;
          c.humanoid = false;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.barehandedDamage = 10;
          c.barehandedAttack = AttackType::EAT;
          c.skills.insert(SkillId::SWIMMING);
          c.weight = 400;
          c.name = "hydra";);
    case CreatureId::SHELOB:
      return INHERIT(HYDRA,
          c.viewId = ViewId::SHELOB;
          c.minionTasks.setValue(MinionTask::SPIDER, 1); 
          c.name = "giant spider";);
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
          c.firstName = NameGenerator::get(NameGeneratorId::DRAGON)->getNext();
          c.name = "green dragon";
          c.speciesName = "dragon";
          );
    case CreatureId::RED_DRAGON:
      return INHERIT(GREEN_DRAGON,
          c.viewId = ViewId::RED_DRAGON;
          c.attr[AttrType::SPEED] = 110;
          c.attr[AttrType::STRENGTH] = 47;
          c.fireCreature = true;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 0;
          c.name = "red dragon";
          );
    case CreatureId::KNIGHT: 
      return CATTR(
          c.viewId = ViewId::KNIGHT;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 30;
          c.attr[AttrType::DEXTERITY] = 22;
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
          c.attr[AttrType::STRENGTH] += 6;
          c.attr[AttrType::DEXTERITY] += 4;
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
          c.viewId = Random.choose({ViewId::PESEANT, ViewId::PESEANT_WOMAN});
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
    case CreatureId::AUTOMATON: 
      return INHERIT(IRON_GOLEM,
          c.viewId = ViewId::AUTOMATON;
          c.attr[AttrType::SPEED] += 30;
          c.attr[AttrType::STRENGTH] += 22;
          c.attr[AttrType::DEXTERITY] += 2;
          c.barehandedDamage -= 2;
          c.name = "automaton";);
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
          c.attr[AttrType::STRENGTH] += 3;
          c.attr[AttrType::DEXTERITY] += 10;
          c.barehandedDamage += 4;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.skills.setValue(SkillId::SORCERY, 0.5);
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::DARKNESS_SOURCE] = 1;
          for (SpellId id : Random.chooseN(Random.get(3, 6), {SpellId::WORD_OF_POWER, SpellId::DEX_BONUS,
              SpellId::STR_BONUS, SpellId::MAGIC_SHIELD, SpellId::STUN_RAY, SpellId::DECEPTION, SpellId::DECEPTION,
              SpellId::TELEPORT}))
            c.spells.add(id);
          c.chatReactionFriendly = c.chatReactionHostile =
              "\"There are times when you simply cannot refuse a drink!\"";
          c.name = "vampire lord";);
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
          c.recruitmentCost = 50;
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
    case CreatureId::HARPY:
      return INHERIT(ORC,
          c.viewId = ViewId::HARPY;
          c.attr[AttrType::SPEED] = 120;
          c.attr[AttrType::STRENGTH] -= 3;
          c.attr[AttrType::DEXTERITY] += 2;
          c.weight = 70;
          c.gender = Gender::female;
          c.minionTasks.setWorkshopTasks(0);
          c.recruitmentCost = 50;
          c.bodyParts[BodyPart::WING] = 2;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.skills.setValue(SkillId::ARCHERY, 1);
          c.name = EntityName("harpy", "harpies"););
    case CreatureId::KOBOLD: 
      return CATTR(
          c.viewId = ViewId::KOBOLD;
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
          c.name = "kobold";);
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
          c.recruitmentCost = 100;
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
          c.innocent = true;
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
          c.attr[AttrType::STRENGTH] = 16;
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
          c.attr[AttrType::STRENGTH] += 8;
          c.attr[AttrType::DEXTERITY] += 10;
          c.courage = 3;
          c.weight = 60;
          c.name = "lizardman chief";);
    case CreatureId::ELF: 
      return CATTR(
          c.viewId = Random.choose({ViewId::ELF, ViewId::ELF_WOMAN});
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
    case CreatureId::DARK_ELF:
      return INHERIT(ELF,
          c.viewId = Random.choose({ViewId::DARK_ELF, ViewId::DARK_ELF_WOMAN});
          c.skills.erase(SkillId::ELF_VISION);
          c.skills.insert(SkillId::NIGHT_VISION);
          c.name = EntityName("dark elf", "dark elves"););
    case CreatureId::DARK_ELF_WARRIOR:
      return INHERIT(ELF_ARCHER,
          c.viewId = ViewId::DARK_ELF_WARRIOR;
          c.skills.erase(SkillId::ELF_VISION);
          c.skills.insert(SkillId::NIGHT_VISION);
          c.skills.setValue(SkillId::WEAPON_MELEE, 1);
          c.minionTasks.setValue(MinionTask::TRAIN, 4); 
          c.minionTasks.setValue(MinionTask::SLEEP, 1);
          c.minionTasks.setValue(MinionTask::STUDY, 1);
          c.minionTasks.setValue(MinionTask::EAT, 3);
          c.recruitmentCost = 140;
          c.name = EntityName("dark elf", "dark elves"););
    case CreatureId::DARK_ELF_CHILD:
      return INHERIT(ELF_CHILD,
          c.viewId = ViewId::DARK_ELF_CHILD;
          c.skills.erase(SkillId::ELF_VISION);
          c.skills.insert(SkillId::NIGHT_VISION);
          c.name = EntityName("dark elf child", "dark elf children"););
    case CreatureId::DARK_ELF_LORD:
      return INHERIT(ELF_LORD,
          c.viewId = ViewId::DARK_ELF_LORD;
          c.skills.erase(SkillId::ELF_VISION);
          c.skills.insert(SkillId::NIGHT_VISION);
          c.name = "dark elf lord";);
    case CreatureId::DRIAD: 
      return CATTR(
          c.viewId = ViewId::DRIAD;
          c.attr[AttrType::SPEED] = 80;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 11;
          c.attr[AttrType::DEXTERITY] = 17;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 40;
          c.chatReactionFriendly = "curses all humans";
          c.chatReactionHostile = "\"Die!\"";
          c.spells.add(SpellId::HEALING);
          c.skills.insert(SkillId::ELF_VISION);
          c.skills.setValue(SkillId::ARCHERY, 1);
          c.name = "driad";);
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
          c.dyingSound = SoundId::DYING_DONKEY;
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
          c.dyingSound = SoundId::DYING_PIG;
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
    case CreatureId::SPIDER: 
      return CATTR(
          c.viewId = ViewId::SPIDER;
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
          c.noDyingSound = true;
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
          c.noDyingSound = true;
          c.name = EntityName("fly", "flies"););
    case CreatureId::ANT_WORKER:
      return CATTR(
          c.viewId = ViewId::ANT_WORKER;
          c.attr[AttrType::SPEED] = 100;
          c.size = CreatureSize::MEDIUM;
          c.attr[AttrType::STRENGTH] = 16;
          c.attr[AttrType::DEXTERITY] = 16;
          c.barehandedDamage = 4;
          c.humanoid = false;
          c.weight = 10;
          c.bodyParts[BodyPart::ARM] = 0;
          c.bodyParts[BodyPart::LEG] = 6;
          c.bodyParts[BodyPart::WING] = 0;
          c.animal = true;
          c.noDyingSound = true;
          c.name = "giant ant";);
    case CreatureId::ANT_SOLDIER:
      return INHERIT(ANT_WORKER,
          c.viewId = ViewId::ANT_SOLDIER;
          c.attr[AttrType::SPEED] += 30;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.attr[AttrType::STRENGTH] += 8;
          c.attr[AttrType::DEXTERITY] = 8;
          c.barehandedDamage += 8;
          c.name = "soldier ant";);
    case CreatureId::ANT_QUEEN:      
      return INHERIT(ANT_SOLDIER,
          c.viewId = ViewId::ANT_QUEEN;
          c.attr[AttrType::STRENGTH] += 4;
          c.attr[AttrType::DEXTERITY] = 4;
          c.barehandedDamage += 8;
          c.name = "ant queen";);
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
          c.noDyingSound = true;
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
          c.noDyingSound = true;
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
          c.noDyingSound = true;
          c.name = "fire sphere";);
    case CreatureId::ELEMENTALIST: 
      return CATTR(
          c.viewId = ViewId::ELEMENTALIST;
          c.attr[AttrType::SPEED] = 120;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::STRENGTH] = 16;
          c.attr[AttrType::DEXTERITY] = 14;
          c.barehandedDamage = 3;
          c.humanoid = true;
          c.weight = 70;
          c.gender = Gender::female;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
          c.firstName = NameGenerator::get(NameGeneratorId::FIRST)->getNext();
          c.name = "elementalist";);
    case CreatureId::FIRE_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::FIRE_ELEMENTAL;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::SPEED] = 120;
          c.attr[AttrType::STRENGTH] = 25;
          c.attr[AttrType::DEXTERITY] = 30;
          c.barehandedAttack = AttackType::HIT;
          c.barehandedDamage = 10;
          c.humanoid = false;
          c.bodyParts.clear();
          c.uncorporal = true;
          c.breathing = false;
          c.brain = false;
          c.fireCreature = true;
          c.attackEffect = EffectId::FIRE;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.weight = 10;
          c.noDyingSound = true;
          c.name = "fire elemental";);
    case CreatureId::AIR_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::AIR_ELEMENTAL;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::SPEED] = 160;
          c.attr[AttrType::STRENGTH] = 20;
          c.attr[AttrType::DEXTERITY] = 30;
          c.barehandedAttack = AttackType::HIT;
          c.barehandedDamage = 10;
          c.humanoid = false;
          c.bodyParts.clear();
          c.uncorporal = true;
          c.breathing = false;
          c.brain = false;
          c.fireCreature = true;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.weight = 10;
          c.spells.add(SpellId::AIR_BLAST);
          c.noDyingSound = true;
          c.name = "air elemental";);
    case CreatureId::EARTH_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::EARTH_ELEMENTAL;
          c.size = CreatureSize::LARGE;
          c.attr[AttrType::SPEED] = 80;
          c.attr[AttrType::STRENGTH] = 45;
          c.attr[AttrType::DEXTERITY] = 20;
          c.barehandedAttack = AttackType::HIT;
          c.barehandedDamage = 10;
          c.humanoid = false;
          c.bodyParts.clear();
          c.breathing = false;
          c.brain = false;
          c.weight = 500;
          c.noDyingSound = true;
          c.name = "earth elemental";);
    case CreatureId::WATER_ELEMENTAL:
      return INHERIT(EARTH_ELEMENTAL,
          c.viewId = ViewId::WATER_ELEMENTAL;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "water elemental";);
    case CreatureId::ENT:
      return CATTR(
          c.viewId = ViewId::ENT;
          c.size = CreatureSize::HUGE;
          c.attr[AttrType::SPEED] = 30;
          c.attr[AttrType::STRENGTH] = 40;
          c.attr[AttrType::DEXTERITY] = 26;
          c.barehandedDamage = 0;
          c.skills.insert(SkillId::ELF_VISION);
          c.minionTasks.clear();
          c.weight = 2000;
          c.humanoid = false;
          c.chatReactionFriendly = "curses all dungeons";
          c.chatReactionHostile = "\"Die!\"";
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

PCreature get(CreatureId id, TribeId tribe, MonsterAIFactory aiFactory) {
  ControllerFactory factory = Monster::getFactory(aiFactory);
  switch (id) {
    case CreatureId::SPECIAL_BL:
      return getSpecial(tribe, false, true, factory);
    case CreatureId::SPECIAL_BM:
      return getSpecial(tribe, false, false, factory);
    case CreatureId::SPECIAL_HL:
      return getSpecial(tribe, true, true, factory);
    case CreatureId::SPECIAL_HM:
      return getSpecial(tribe, true, false, factory);
    case CreatureId::SOKOBAN_BOULDER:
      return getSokobanBoulder(tribe);
    default: return get(getAttributes(id), tribe, getController(id, aiFactory));
  }
}

PCreature CreatureFactory::getGhost(Creature* creature) {
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, "Ghost");
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  return PCreature(new Creature(viewObject, creature->getTribeId(), getAttributes(CreatureId::LOST_SOUL),
        Monster::getFactory(MonsterAIFactory::monster())));
}

ItemType randomHealing() {
  return Random.choose({ItemType(ItemId::POTION, EffectId::HEAL), {ItemId::FIRST_AID_KIT}});
}

ItemType randomBackup() {
  return Random.choose({{ItemId::SCROLL, EffectId::DECEPTION}, {ItemId::SCROLL, EffectId::TELEPORT},
      randomHealing()}, {1, 1, 8});
}

ItemType randomArmor() {
  return Random.choose({ItemId::LEATHER_ARMOR, ItemId::CHAIN_ARMOR}, {4, 1});
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
    case CreatureId::WITCHMAN:
      return ItemList()
        .add(ItemId::SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add({ItemId::POTION, EffectType{EffectId::LASTING, LastingEffect::SPEED}}, 4)
        .add(ItemId::GOLD_PIECE, Random.get(350, 500));
    case CreatureId::KNIGHT: 
      return ItemList()
        .add(ItemId::SWORD)
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.get(30, 80));
    case CreatureId::MINOTAUR: 
      return ItemList()
        .add(ItemId::BATTLE_AXE);
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
    case CreatureId::DWARF: 
      return ItemList()
        .add(Random.choose({ItemId::BATTLE_AXE, ItemId::WAR_HAMMER}, {1, 1}))
        .maybe(0.6, randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .maybe(0.5, ItemId::IRON_HELM)
        .maybe(0.3, ItemId::IRON_BOOTS)
        .add(ItemId::GOLD_PIECE, Random.get(10, 30));
    case CreatureId::DWARF_BARON: 
      return ItemList()
        .add(Random.choose({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
        .add(randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::IRON_BOOTS)
        .add(ItemId::IRON_HELM)
        .add(ItemId::GOLD_PIECE, Random.get(200, 400));
    case CreatureId::GNOME_CHIEF:
      return ItemList()
        .add(ItemId::SWORD)
        .add(randomBackup());
    case CreatureId::DARK_ELF_LORD: 
    case CreatureId::ELF_LORD: 
      return ItemList()
        .add(ItemId::SPECIAL_ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::BOW)
        .add(ItemId::ARROW, Random.get(20, 36))
        .add(ItemId::GOLD_PIECE, Random.get(100, 300))
        .add(randomBackup());
    case CreatureId::DRIAD: 
      return ItemList()
        .add(ItemId::BOW)
        .add(ItemId::ARROW, Random.get(20, 36));
    case CreatureId::DARK_ELF_WARRIOR: 
      return ItemList()
        .add(ItemId::ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::GOLD_PIECE, Random.get(10, 30))
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
            Random.choose({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER, ItemId::SPECIAL_SWORD}));
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

PCreature CreatureFactory::fromId(CreatureId id, TribeId t) {
  return fromId(id, t, MonsterAIFactory::monster());
}

PCreature CreatureFactory::fromId(CreatureId id, TribeId t, const MonsterAIFactory& factory) {
  return addInventory(get(id, t, factory), getInventory(id));
}

