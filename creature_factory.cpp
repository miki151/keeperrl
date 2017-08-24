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
#include "creature.h"
#include "game.h"
#include "name_generator.h"
#include "player_message.h"
#include "equipment.h"
#include "minion_task_map.h"
#include "spell_map.h"
#include "tribe.h"
#include "monster_ai.h"
#include "sound.h"
#include "player.h"
#include "map_memory.h"
#include "body.h"
#include "attack_type.h"
#include "attack_level.h"
#include "attack.h"
#include "spell_map.h"
#include "item_type.h"
#include "item.h"
#include "spawn_type.h"
#include "furniture.h"
#include "experience_type.h"
#include "creature_debt.h"

SERIALIZE_DEF(CreatureFactory, tribe, creatures, weights, unique, tribeOverrides, levelIncrease, baseLevelIncrease, inventory)
SERIALIZATION_CONSTRUCTOR_IMPL(CreatureFactory)

CreatureFactory CreatureFactory::singleCreature(TribeId tribe, CreatureId id) {
  return CreatureFactory(tribe, {id}, {1}, {});
}

class BoulderController : public Monster {
  public:
  BoulderController(WCreature c, Vec2 dir) : Monster(c, MonsterAIFactory::idle()), direction(dir) {
    CHECK(direction.length4() == 1);
  }

  virtual void makeMove() override {
    Position nextPos = getCreature()->getPosition().plus(direction);
    if (WCreature c = nextPos.getCreature()) {
      if (!c->getBody().isKilledByBoulder()) {
        if (nextPos.canEnterEmpty(getCreature())) {
          getCreature()->swapPosition(direction);
          return;
        }
      } else {
        health -= c->getBody().getBoulderDamage();
        if (health <= 0) {
          nextPos.globalMessage(getCreature()->getName().the() + " crashes on " + c->getName().the());
          nextPos.unseenMessage("You hear a crash");
          getCreature()->dieNoReason();
          c->takeDamage(Attack(getCreature(), AttackLevel::MIDDLE, AttackType::HIT, 1000, AttrType::DAMAGE));
          return;
        } else {
          c->you(MsgType::KILLED_BY, getCreature()->getName().the());
          c->dieWithAttacker(getCreature());
        }
      }
    }
    if (auto furniture = nextPos.getFurniture(FurnitureLayer::MIDDLE))
      if (furniture->canDestroy(getCreature()->getMovementType(), DestroyAction::Type::BOULDER) &&
          *furniture->getStrength(DestroyAction::Type::BOULDER) <
          health * getCreature()->getAttr(AttrType::DAMAGE)) {
        health -= *furniture->getStrength(DestroyAction::Type::BOULDER) /
            (double) getCreature()->getAttr(AttrType::DAMAGE);
        getCreature()->destroyImpl(direction, DestroyAction::Type::BOULDER);
      }
    if (auto action = getCreature()->move(direction))
      action.perform(getCreature());
    else {
      nextPos.globalMessage(getCreature()->getName().the() + " crashes on the " + nextPos.getName());
      nextPos.unseenMessage("You hear a crash");
      getCreature()->dieNoReason();
      return;
    }
    int speed = getCreature()->getAttr(AttrType::SPEED);
    double deceleration = 0.1;
    speed -= deceleration * 100 * 100 / speed;
    if (speed < 30 && !getCreature()->isDead()) {
      getCreature()->dieNoReason();
      return;
    }
    getCreature()->getAttributes().setBaseAttr(AttrType::SPEED, speed);
  }

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator g(MessageGenerator::BOULDER);
    return g;
  }

  SERIALIZE_ALL(SUBCLASS(Monster), direction)
  SERIALIZATION_CONSTRUCTOR(BoulderController)

  private:
  Vec2 SERIAL(direction);
  double health = 1;
};

PCreature CreatureFactory::getRollingBoulder(TribeId tribe, Vec2 direction) {
  ViewObject viewObject(ViewId::BOULDER, ViewLayer::CREATURE);
  viewObject.setModifier(ViewObjectModifier::NO_UP_MOVEMENT);
  auto ret = makeOwner<Creature>(viewObject, tribe, CATTR(
            c.viewId = ViewId::BOULDER;
            c.attr[AttrType::DAMAGE] = 250;
            c.attr[AttrType::DEFENSE] = 250;
            c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::HUGE).setDeathSound(none);
            c.attr[AttrType::SPEED] = 140;
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.boulder = true;
            c.name = "boulder";
            ));
  ret->setController(makeOwner<BoulderController>(ret.get(), direction));
  return ret;
}

class SokobanController : public Monster {
  public:
  SokobanController(WCreature c) : Monster(c, MonsterAIFactory::idle()) {}

  virtual void onBump(WCreature player) override {
    Vec2 goDir = player->getPosition().getDir(getCreature()->getPosition());
    if (goDir.isCardinal4() && getCreature()->getPosition().plus(goDir).canEnter(
          getCreature()->getMovementType().setForced(true))) {
      getCreature()->displace(player->getLocalTime(), goDir);
      player->move(goDir).perform(player);
    }
  }

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator g(MessageGenerator::BOULDER);
    return g;
  }

  SERIALIZE_ALL(SUBCLASS(Monster));
  SERIALIZATION_CONSTRUCTOR(SokobanController);

  private:
};

PCreature CreatureFactory::getSokobanBoulder(TribeId tribe) {
  ViewObject viewObject(ViewId::BOULDER, ViewLayer::CREATURE);
  viewObject.setModifier(ViewObjectModifier::NO_UP_MOVEMENT).setModifier(ViewObjectModifier::REMEMBER);
  auto ret = makeOwner<Creature>(viewObject, tribe, CATTR(
            c.viewId = ViewId::BOULDER;
            c.attr[AttrType::DAMAGE] = 250;
            c.attr[AttrType::DEFENSE] = 250;
            c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::HUGE).setDeathSound(none);
            c.attr[AttrType::SPEED] = 140;
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.boulder = true;
            c.name = "boulder";));
  ret->setController(makeOwner<SokobanController>(ret.get()));
  return ret;
}

CreatureAttributes CreatureFactory::getKrakenAttributes(ViewId id, const char* name) {
  return CATTR(
      c.viewId = id;
      c.attr[AttrType::SPEED] = 40;
      c.body = Body::nonHumanoid(Body::Size::LARGE).setDeathSound(none);
      c.attr[AttrType::DAMAGE] = 15;
      c.attr[AttrType::DEFENSE] = 15;
      c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
      c.skills.insert(SkillId::SWIMMING);
      c.name = name;);
}

ViewId CreatureFactory::getViewId(CreatureId id) {
  static EnumMap<CreatureId, ViewId> idMap([](CreatureId id) {
    auto c = fromId(id, TribeId::getMonster());
    return c->getViewObject().id();
  });
  return idMap[id];
}

class KrakenController : public Monster {
  public:
  KrakenController(WCreature c) : Monster(c, MonsterAIFactory::monster()) {
  }

  KrakenController(WCreature c, WeakPointer<KrakenController> f) : KrakenController(c) {
    father = f;
  }

  virtual bool isCustomController() override {
    return true;
  }

  int getMaxSpawns() {
    if (father)
      return 1;
    else
      return 7;
  }

  virtual void onKilled(WConstCreature attacker) override {
    if (attacker) {
      if (father)
        attacker->secondPerson("You cut the kraken's tentacle");
      else
        attacker->secondPerson("You kill the kraken!");
    }
    for (WCreature c : spawns)
      if (!c->isDead())
        c->dieNoReason();
  }

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator kraken(MessageGenerator::KRAKEN);
    static MessageGenerator third(MessageGenerator::THIRD_PERSON);
    if (father)
      return kraken;
    else
      return third;
  }

  void pullEnemy(WCreature held) {
    held->you(MsgType::HAPPENS_TO, getCreature()->getName().the() + " pulls");
    if (father) {
      held->setHeld(father->getCreature());
      Vec2 pullDir = held->getPosition().getDir(getCreature()->getPosition());
      double localTime = getCreature()->getLocalTime();
      getCreature()->dieNoReason(Creature::DropType::NOTHING);
      held->displace(localTime, pullDir);
    } else {
      held->you(MsgType::ARE, "eaten by " + getCreature()->getName().the());
      held->dieNoReason();
    }
  }

  WCreature getHeld() {
    for (auto pos : getCreature()->getPosition().neighbors8())
      if (auto creature = pos.getCreature())
        if (creature->getHoldingCreature() == getCreature())
          return creature;
    return nullptr;
  }

  WCreature getVisibleEnemy() {
    const int radius = 10;
    WCreature ret = nullptr;
    auto myPos = getCreature()->getPosition();
    for (Position pos : getCreature()->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), radius)))
      if (WCreature c = pos.getCreature())
        if (c->getAttributes().getCreatureId() != getCreature()->getAttributes().getCreatureId() &&
            (!ret || ret->getPosition().dist8(myPos) > c->getPosition().dist8(myPos)) &&
            getCreature()->canSee(c) && getCreature()->isEnemy(c) && !c->getHoldingCreature())
          ret = c;
    return ret;
  }

  void considerAttacking(WCreature c) {
    auto pos = c->getPosition();
    Vec2 v = getCreature()->getPosition().getDir(pos);
    if (v.length8() == 1) {
      c->you(MsgType::HAPPENS_TO, getCreature()->getName().the() + " swings itself around");
      c->setHeld(getCreature());
    } else {
      pair<Vec2, Vec2> dirs = v.approxL1();
      vector<Vec2> moves;
      if (getCreature()->getPosition().plus(dirs.first).canEnter(
            {{MovementTrait::WALK, MovementTrait::SWIM}}))
        moves.push_back(dirs.first);
      if (getCreature()->getPosition().plus(dirs.second).canEnter(
            {{MovementTrait::WALK, MovementTrait::SWIM}}))
        moves.push_back(dirs.second);
      if (!moves.empty()) {
        Vec2 move = Random.choose(moves);
        ViewId viewId = getCreature()->getPosition().plus(move).canEnter({MovementTrait::SWIM})
          ? ViewId::KRAKEN_WATER : ViewId::KRAKEN_LAND;
        auto spawn = makeOwner<Creature>(getCreature()->getTribeId(),
              CreatureFactory::getKrakenAttributes(viewId, "kraken tentacle"));
        spawn->setController(makeOwner<KrakenController>(spawn.get(), getThis().dynamicCast<KrakenController>()));
        spawns.push_back(spawn.get());
        getCreature()->getPosition().plus(move).addCreature(std::move(spawn));
      }
    }
  }

  virtual void makeMove() override {
    for (WCreature c : spawns)
      if (c->isDead()) {
        spawns.removeElement(c);
        break;
      }
    if (spawns.empty()) {
      if (auto held = getHeld()) {
        pullEnemy(held);
        return;
      } else if (auto c = getVisibleEnemy()) {
        considerAttacking(c);
      } else if (father && Random.roll(5)) {
        getCreature()->dieNoReason(Creature::DropType::NOTHING);
        return;
      }
    }
    getCreature()->wait().perform(getCreature());
  }

  SERIALIZE_ALL(SUBCLASS(Monster), ready, spawns, father);
  SERIALIZATION_CONSTRUCTOR(KrakenController);

  private:
  bool SERIAL(ready) = false;
  vector<WCreature> SERIAL(spawns);
  WeakPointer<KrakenController> SERIAL(father);
};

class KamikazeController : public Monster {
  public:
  KamikazeController(WCreature c, MonsterAIFactory f) : Monster(c, f) {}

  virtual void makeMove() override {
    for (Position pos : getCreature()->getPosition().neighbors8())
      if (WCreature c = pos.getCreature())
        if (getCreature()->isEnemy(c) && getCreature()->canSee(c)) {
          getCreature()->thirdPerson(getCreature()->getName().the() + " explodes!");
          for (Position v : c->getPosition().neighbors8())
            v.fireDamage(1);
          c->getPosition().fireDamage(1);
          getCreature()->dieNoReason(Creature::DropType::ONLY_INVENTORY);
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

class ShopkeeperController : public Monster, public EventListener<ShopkeeperController> {
  public:
  ShopkeeperController(WCreature c, Rectangle area)
      : Monster(c, MonsterAIFactory::stayInLocation(area)), shopArea(area) {
  }

  vector<Position> getAllShopPositions() const {
    return shopArea.getAllSquares().transform([this](Vec2 v){ return Position(v, myLevel); });
  }

  bool isShopPosition(const Position& pos) {
    return pos.isSameLevel(myLevel) && pos.getCoord().inRectangle(shopArea);
  }

  virtual void makeMove() override {
    if (firstMove) {
      myLevel = getCreature()->getLevel();
      subscribeTo(getCreature()->getPosition().getModel());
      for (Position v : getAllShopPositions()) {
        for (WItem item : v.getItems())
          item->setShopkeeper(getCreature());
        v.clearItemIndex(ItemIndex::FOR_SALE);
      }
      firstMove = false;
    }
    if (!getCreature()->getPosition().isSameLevel(myLevel)) {
      Monster::makeMove();
      return;
    }
    vector<Creature::Id> creatures;
    for (Position v : getAllShopPositions())
      if (WCreature c = v.getCreature()) {
        creatures.push_back(c->getUniqueId());
        if (!prevCreatures.contains(c) && !thieves.contains(c) && !getCreature()->isEnemy(c)) {
          if (!debtors.contains(c))
            c->secondPerson("\"Welcome to " + *getCreature()->getName().first() + "'s shop!\"");
          else {
            c->secondPerson("\"Pay your debt or... !\"");
            thiefCount.erase(c);
          }
        }
      }
    for (auto debtorId : copyOf(debtors))
      if (!creatures.contains(debtorId))
        for (auto pos : getCreature()->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), 30)))
          if (auto debtor = pos.getCreature())
            if (debtor->getUniqueId() == debtorId) {
              debtor->privateMessage("\"Come back, you owe me " + toString(debtor->getDebt().getAmountOwed(getCreature())) +
                  " gold!\"");
              if (++thiefCount.getOrInit(debtor) == 4) {
                debtor->privateMessage("\"Thief! Thief!\"");
                getCreature()->getTribe()->onItemsStolen(debtor);
                thiefCount.erase(debtor);
                debtors.erase(debtor);
                thieves.insert(debtor);
                for (WItem item : debtor->getEquipment().getItems())
                  item->setShopkeeper(nullptr);
                break;
              }
            }
    prevCreatures.clear();
    for (auto c : creatures)
      prevCreatures.insert(c);
    Monster::makeMove();
  }

  virtual void onItemsGiven(vector<WItem> items, WCreature from) override {
    int paid = items.filter(Item::classPredicate(ItemClass::GOLD)).size();
    from->getDebt().add(getCreature(), -paid);
    if (from->getDebt().getAmountOwed(getCreature()) <= 0)
      debtors.erase(from);
  }
  
  void onEvent(const GameEvent& event) {
    using namespace EventInfo;
    event.visit(
        [&](const ItemsAppeared& info) {
          if (isShopPosition(info.position)) {
            for (auto& it : info.items) {
              it->setShopkeeper(getCreature());
              info.position.clearItemIndex(ItemIndex::FOR_SALE);
            }
          }
        },
        [&](const ItemsPickedUp& info) {
          if (isShopPosition(info.creature->getPosition())) {
            for (auto& item : info.items)
              if (item->isShopkeeper(getCreature())) {
                info.creature->getDebt().add(getCreature(), item->getPrice());
                debtors.insert(info.creature);
              }
          }
        },
        [&](const ItemsDropped& info) {
          if (isShopPosition(info.creature->getPosition())) {
            for (auto& item : info.items)
              if (item->isShopkeeper(getCreature())) {
                info.creature->getDebt().add(getCreature(), -item->getPrice());
                if (info.creature->getDebt().getAmountOwed(getCreature()) == 0)
                  debtors.erase(info.creature);
              }
          }
        },
        [&](const auto&) {}
    );
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int) {
    ar & SUBCLASS(Monster) & SUBCLASS(EventListener);
    ar(prevCreatures, debtors, thiefCount, thieves, shopArea, myLevel, firstMove);
  }

  SERIALIZATION_CONSTRUCTOR(ShopkeeperController);

  private:
  EntitySet<Creature> SERIAL(prevCreatures);
  EntitySet<Creature> SERIAL(debtors);
  EntityMap<Creature, int> SERIAL(thiefCount);
  EntitySet<Creature> SERIAL(thieves);
  Rectangle SERIAL(shopArea);
  WLevel SERIAL(myLevel) = nullptr;
  bool SERIAL(firstMove) = true;
};

void CreatureFactory::addInventory(WCreature c, const vector<ItemType>& items) {
  for (ItemType item : items)
    c->take(ItemFactory::fromId(item));
}

PCreature CreatureFactory::getShopkeeper(Rectangle shopArea, TribeId tribe) {
  auto ret = makeOwner<Creature>(tribe,
      CATTR(
        c.viewId = ViewId::SHOPKEEPER;
        c.attr[AttrType::SPEED] = 100;
        c.body = Body::humanoid(Body::Size::LARGE);
        c.attr[AttrType::DAMAGE] = 17;
        c.attr[AttrType::DEFENSE] = 20;
        c.chatReactionFriendly = "complains about high import tax"_s;
        c.chatReactionHostile = "\"Die!\""_s;
        c.name = "shopkeeper";
        c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());));
  ret->setController(makeOwner<ShopkeeperController>(ret.get(), shopArea));
  vector<ItemType> inventory(Random.get(20, 60), ItemId::GOLD_PIECE);
  inventory.push_back(ItemId::SWORD);
  inventory.push_back(ItemId::LEATHER_ARMOR);
  inventory.push_back(ItemId::LEATHER_BOOTS);
  inventory.push_back({ItemId::POTION, EffectId::HEAL});
  inventory.push_back({ItemId::POTION, EffectId::HEAL});
  addInventory(ret.get(), inventory);
  return ret;
}

class IllusionController : public DoNothingController {
  public:
  IllusionController(WCreature c, double deathT) : DoNothingController(c), deathTime(deathT) {}

  virtual void onBump(WCreature c) override {
    c->attack(getCreature(), none).perform(c);
    getCreature()->message("It was just an illusion!");
    if (!getCreature()->isDead()) // so check necessary, as most likely was killed in attack 2 lines above
      getCreature()->dieNoReason();
  }

  virtual void makeMove() override {
    if (getCreature()->getGlobalTime() >= deathTime) {
      getCreature()->message("The illusion disappears.");
      getCreature()->dieNoReason();
    } else
      getCreature()->wait().perform(getCreature());
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(DoNothingController);
    ar(deathTime);
  }

  SERIALIZATION_CONSTRUCTOR(IllusionController)

  private:
  double SERIAL(deathTime);
};

PCreature CreatureFactory::getIllusion(WCreature creature) {
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, "Illusion");
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  auto ret = makeOwner<Creature>(viewObject, creature->getTribeId(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.illusionViewObject = creature->getViewObject();
          (*c.illusionViewObject)->removeModifier(ViewObject::Modifier::INVISIBLE);
          c.attr[AttrType::SPEED] = 100;
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE).setDeathSound(SoundId::MISSED_ATTACK);
          c.attr[AttrType::DAMAGE] = 20; // just so it's not ignored by creatures
          c.attr[AttrType::DEFENSE] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.noAttackSound = true;
          c.name = creature->getName();));
  ret->setController(makeOwner<IllusionController>(ret.get(), creature->getGlobalTime() + Random.get(5, 10)));
  return ret;
}

REGISTER_TYPE(BoulderController)
REGISTER_TYPE(SokobanController)
REGISTER_TYPE(KrakenController)
REGISTER_TYPE(KamikazeController)
REGISTER_TYPE(ShopkeeperController)
REGISTER_TYPE(IllusionController)
REGISTER_TYPE(ListenerTemplate<ShopkeeperController>)

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
  PCreature ret = fromId(id, getTribeFor(id), actorFactory, inventory);
  for (auto exp : ENUM_ALL(ExperienceType)) {
    ret->getAttributes().increaseBaseExpLevel(exp, baseLevelIncrease[exp]);
    ret->increaseExpLevel(exp, levelIncrease[exp]);
  }
  return ret;
}

PCreature CreatureFactory::get(const CreatureAttributes& attr, TribeId tribe, const ControllerFactory& factory) {
  auto ret = makeOwner<Creature>(tribe, attr);
  ret->setController(factory.get(ret.get()));
  return ret;
}

CreatureFactory& CreatureFactory::increaseLevel(EnumMap<ExperienceType, int> l) {
  levelIncrease = l;
  return *this;
}

CreatureFactory& CreatureFactory::increaseLevel(ExperienceType t, int l) {
  levelIncrease[t] = l;
  return *this;
}

CreatureFactory& CreatureFactory::increaseBaseLevel(ExperienceType t, int l) {
  baseLevelIncrease[t] = l;
  return *this;
}

CreatureFactory& CreatureFactory::addInventory(vector<ItemType> items) {
  inventory = items;
  return *this;
}

CreatureFactory::CreatureFactory(TribeId t, const vector<CreatureId>& c, const vector<double>& w,
    const vector<CreatureId>& u, EnumMap<CreatureId, optional<TribeId>> overrides)
    : tribe(t), creatures(c), weights(w), unique(u), tribeOverrides(overrides) {
}

CreatureFactory::CreatureFactory(const vector<tuple<CreatureId, double, TribeId>>& c, const vector<CreatureId>& u)
    : unique(u) {
  for (auto& elem : c) {
    creatures.push_back(std::get<0>(elem));
    weights.push_back(std::get<1>(elem));
    tribeOverrides[std::get<0>(elem)] = std::get<2>(elem);
  }
}

// These have to be defined here to be able to forward declare some ItemType and other classes
CreatureFactory::~CreatureFactory() {
}

CreatureFactory::CreatureFactory(const CreatureFactory&) = default;

CreatureFactory& CreatureFactory::operator =(const CreatureFactory&) = default;

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

CreatureFactory CreatureFactory::tutorialVillage(TribeId tribe) {
  return CreatureFactory(tribe, {}, {}, {CreatureId::PESEANT, CreatureId::PESEANT, CreatureId::PESEANT,
      CreatureId::PESEANT, CreatureId::PESEANT, CreatureId::DONKEY, CreatureId::PIG, CreatureId::PIG, CreatureId::DOG});
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
  splashFactories = Random.choose(
      make_pair(CreatureFactory(tribe, { CreatureId::KNIGHT, CreatureId::ARCHER}, { 1, 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::AVATAR)),
      make_pair(CreatureFactory(tribe, { CreatureId::WARRIOR}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::SHAMAN)),
      make_pair(CreatureFactory(tribe, { CreatureId::ELF_ARCHER}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::ELF_LORD)),
      make_pair(CreatureFactory(tribe, { CreatureId::DWARF}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::DWARF_BARON)),
      make_pair(CreatureFactory(tribe, { CreatureId::LIZARDMAN}, { 1}, {}),
        CreatureFactory::singleType(tribe, CreatureId::LIZARDLORD))
      );
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
      CreatureId::SPECIAL_HLBN, CreatureId::SPECIAL_BLBW, CreatureId::WOLF, CreatureId::CAVE_BEAR,
      CreatureId::BAT, CreatureId::WEREWOLF, CreatureId::ZOMBIE, CreatureId::VAMPIRE, CreatureId::DOPPLEGANGER,
      CreatureId::SUCCUBUS},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {}, {}).increaseBaseLevel(ExperienceType::MELEE, 25);
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

CreatureFactory CreatureFactory::elvenCottage(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ELF, CreatureId::ELF_CHILD, CreatureId::HORSE,
      CreatureId::COW, CreatureId::DOG },
      { 2, 2, 1, 1, 0}, {CreatureId::ELF_ARCHER});
}

CreatureFactory CreatureFactory::forrest(TribeId tribe) {
  return CreatureFactory(tribe,
      { CreatureId::DEER, CreatureId::FOX, CreatureId::BOAR },
      { 4, 2, 2}, {});
}

CreatureFactory CreatureFactory::crypt(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ZOMBIE}, { 1}, {});
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

CreatureFactory CreatureFactory::dwarfCave(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::DWARF, CreatureId::DWARF_FEMALE}, { 2, 1});
}

CreatureFactory CreatureFactory::antNest(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ANT_WORKER, CreatureId::ANT_SOLDIER}, { 2, 1},
      { CreatureId::ANT_QUEEN});
}

CreatureFactory CreatureFactory::orcTown(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::ORC, CreatureId::OGRE }, {1, 1});
}

CreatureFactory CreatureFactory::demonDen(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::DEMON_DWELLER}, {1},
      { CreatureId::DEMON_LORD});
}

CreatureFactory CreatureFactory::demonDenAbove(TribeId tribe) {
  return CreatureFactory(tribe, { CreatureId::GHOST}, {1});
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
      make_tuple(CreatureId::SOFT_MONSTER, 5., enemy),
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

static ViewId getSpecialViewId(bool humanoid, bool large, bool body, bool wings) {
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

static EnumMap<BodyPart, int> getSpecialBeastBody(bool large, bool living, bool wings) {
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
  return parts[(!large) * 4 + (!living) * 2 + wings];
}

static vector<LastingEffect> getResistanceAndVulnerability(RandomGen& random) {
  vector<LastingEffect> resistances {
      LastingEffect::MAGIC_RESISTANCE,
      LastingEffect::MELEE_RESISTANCE,
      LastingEffect::RANGED_RESISTANCE
  };
  vector<LastingEffect> vulnerabilities {
      LastingEffect::MAGIC_VULNERABILITY,
      LastingEffect::MELEE_VULNERABILITY,
      LastingEffect::RANGED_VULNERABILITY
  };
  vector<LastingEffect> ret;
  ret.push_back(Random.choose(resistances));
  vulnerabilities.removeIndex(*resistances.findElement(ret[0]));
  ret.push_back(Random.choose(vulnerabilities));
  return ret;
}

PCreature CreatureFactory::getSpecial(TribeId tribe, bool humanoid, bool large, bool living, bool wings,
    const ControllerFactory& factory) {
  Body body = Body(humanoid, living ? Body::Material::FLESH : Body::Material::SPIRIT,
      large ? Body::Size::LARGE : Body::Size::MEDIUM);
  if (wings)
    body.addWings();
  string name = getSpeciesName(humanoid, large, living, wings);
  PCreature c = get(CATTR(
        c.viewId = getSpecialViewId(humanoid, large, living, wings);
        c.isSpecial = true;
        c.body = body;
        c.attr[AttrType::SPEED] = Random.get(80, 120);
        if (!large)
          c.attr[AttrType::SPEED] += 20;
        c.attr[AttrType::DAMAGE] = Random.get(18, 24);
        c.attr[AttrType::DEFENSE] = Random.get(18, 24);
        for (auto effect : getResistanceAndVulnerability(Random))
          c.permanentEffects[effect] = 1;
        if (large) {
          c.attr[AttrType::DAMAGE] += 6;
          c.attr[AttrType::DEFENSE] += 2;
        }
        c.spawnType = humanoid ? SpawnType::HUMANOID : SpawnType::BEAST;
        if (humanoid) {
          c.skills.setValue(SkillId::SORCERY, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::WORKSHOP, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::FORGE, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::LABORATORY, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::JEWELER, Random.getDouble(0, 1));
          c.skills.setValue(SkillId::FURNACE, Random.getDouble(0, 1));
          c.maxLevelIncrease[ExperienceType::MELEE] = 10;
          c.maxLevelIncrease[ExperienceType::SPELL] = 10;
        }
        if (humanoid) {
          c.chatReactionFriendly = "\"I am the mighty " + name + "\"";
          c.chatReactionHostile = "\"I am the mighty " + name + ". Die!\"";
        } else {
          c.chatReactionFriendly = c.chatReactionHostile = "The " + name + " snarls.";
        }
        c.name = name;
        c.name->setStack(humanoid ? "legendary humanoid" : "legendary beast");
        c.name->setFirst(NameGenerator::get(NameGeneratorId::DEMON)->getNext());
        if (!humanoid) {
          c.body->setBodyParts(getSpecialBeastBody(large, living, wings));
          c.attr[AttrType::DAMAGE] += 5;
          c.attr[AttrType::DEFENSE] += 5;
          c.attackEffect = getSpecialBeastAttack(large, living, wings);
        }
        if (Random.roll(3))
          c.skills.insert(SkillId::SWIMMING);
        ), tribe, factory);
  if (body.isHumanoid()) {
    if (Random.roll(4))
      c->take(ItemFactory::fromId(ItemId::BOW));
    c->take(ItemFactory::fromId(Random.choose(
            ItemId::SPECIAL_SWORD, ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER)));
  }
  return c;
}

CreatureAttributes CreatureFactory::getAttributes(CreatureId id) {
  return getAttributesFromId(id).setCreatureId(id);
}

#define CREATE_LITERAL(NAME, SHORT) \
static pair<AttrType, int> operator "" _##SHORT(unsigned long long value) {\
  return {AttrType::NAME, value};\
}

CREATE_LITERAL(DAMAGE, dam)
CREATE_LITERAL(DEFENSE, def)
CREATE_LITERAL(SPELL_DAMAGE, spell_dam)
CREATE_LITERAL(RANGED_DAMAGE, ranged_dam)
CREATE_LITERAL(SPEED, spd)

#undef CREATE_LITERAL

CreatureAttributes CreatureFactory::getAttributesFromId(CreatureId id) {
  switch (id) {
    case CreatureId::KEEPER: 
      return CATTR(
          c.viewId = ViewId::KEEPER;
          c.retiredViewId = ViewId::RETIRED_KEEPER;
          c.attr = LIST(12_dam, 12_def, 20_spell_dam, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = "Keeper";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());
          c.name->useFullTitle();
          c.skills.setValue(SkillId::SORCERY, 0.2);
          c.skills.setValue(SkillId::LABORATORY, 0.2);
          c.maxLevelIncrease[ExperienceType::MELEE] = 7;
          c.maxLevelIncrease[ExperienceType::SPELL] = 12;
      );
    case CreatureId::KEEPER_F:
      return CATTR(
          c.viewId = ViewId::KEEPER_F;
          c.retiredViewId = ViewId::RETIRED_KEEPER_F;
          c.attr = LIST(12_dam, 12_def, 12_spell_dam, 100_spd );
          c.gender = Gender::female;
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = "Keeper";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_FEMALE)->getNext());
          c.name->useFullTitle();
          c.skills.setValue(SkillId::SORCERY, 0.2);
          c.skills.setValue(SkillId::LABORATORY, 0.2);
          c.maxLevelIncrease[ExperienceType::MELEE] = 7;
          c.maxLevelIncrease[ExperienceType::SPELL] = 12;
      );
    case CreatureId::ADVENTURER:
      return CATTR(
          c.viewId = ViewId::PLAYER;
          c.attr = LIST(15_dam, 20_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.maxLevelIncrease[ExperienceType::MELEE] = 15;
          c.name = "Adventurer";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());
          c.name->useFullTitle();
          c.skills.insert(SkillId::AMBUSH);
          c.maxLevelIncrease[ExperienceType::MELEE] = 16;
          c.maxLevelIncrease[ExperienceType::SPELL] = 8;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 8;
      );
    case CreatureId::ADVENTURER_F:
      return CATTR(
          c.viewId = ViewId::PLAYER_F;
          c.gender = Gender::female;
          c.attr = LIST(15_dam, 20_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.maxLevelIncrease[ExperienceType::MELEE] = 15;
          c.name = "Adventurer";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_FEMALE)->getNext());
          c.name->useFullTitle();
          c.maxLevelIncrease[ExperienceType::MELEE] = 16;
          c.maxLevelIncrease[ExperienceType::SPELL] = 8;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 8;
      );
    case CreatureId::UNICORN:
      return CATTR(
        c.viewId = ViewId::UNICORN;
        c.attr = LIST(20_dam, 20_def, 20_spell_dam, 200_spd );
        c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(500).setHorseBodyParts();
        //Ideally, you should club them to death or chop them up with a sword.
        c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
        c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
        c.barehandedAttack = AttackType::HIT;
        c.courage = 100;
        //They heal up and summon friends.
        c.spells->add(SpellId::HEAL_SELF);
        c.spells->add(SpellId::HEAL_OTHER);
        c.spells->add(SpellId::SUMMON_SPIRIT);
        c.chatReactionFriendly = "\"mhhhhhrrrr!\""_s;
        c.chatReactionHostile = "\"mhhhhhrrrr!\""_s;
        c.name = "unicorn";
        //Pet names like dogs would have.
        c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
        c.name->setGroup("herd");
        c.animal = true;
        );
    case CreatureId::BANDIT:
      return CATTR(
          c.viewId = ViewId::BANDIT;
          c.attr = LIST(20_dam, 13_def, 100_spd );
          c.attr[AttrType::SPEED] = 100;
          c.body = Body::humanoid(Body::Size::LARGE);
          c.attr[AttrType::DAMAGE] = 20;
          c.attr[AttrType::DEFENSE] = 15;
          c.chatReactionFriendly = "curses all law enforcement"_s;
          c.chatReactionHostile = "\"Die!\""_s;
 //         c.skills.insert(SkillId::DISARM_TRAPS);
          c.name = "bandit";);
    case CreatureId::GHOST: 
      return CATTR(
          c.viewId = ViewId::GHOST;
          c.attr = LIST(35_def, 30_spell_dam, 80_spd );
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.barehandedAttack = AttackType::HIT;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.chatReactionFriendly = "\"Wouuuouuu!!!\""_s;
          c.chatReactionHostile = "\"Wouuuouuu!!!\""_s;
          c.name = "ghost";);
    case CreatureId::SPIRIT:
      return CATTR(
          c.viewId = ViewId::SPIRIT;
          c.attr = LIST(35_def, 40_spell_dam, 100_spd );
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.barehandedAttack = AttackType::HIT;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.chatReactionFriendly = "\"Wouuuouuu!!!\""_s;
          c.chatReactionHostile = "\"Wouuuouuu!!!\""_s;
          c.name = "ancient spirit";);
    case CreatureId::LOST_SOUL:
      return CATTR(
          c.viewId = ViewId::GHOST;
          c.attr = LIST(25_def, 5_spell_dam, 120_spd );
          c.courage = 1;
          c.spawnType = SpawnType::DEMON;
          c.spells->add(SpellId::INVISIBILITY);
          c.barehandedAttack = AttackType::POSSESS;
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.chatReactionFriendly = "\"Wouuuouuu!!!\""_s;
          c.chatReactionHostile = "\"Wouuuouuu!!!\""_s;
          c.name = "ghost";);
    case CreatureId::SUCCUBUS:
      return CATTR(
          c.attr = LIST(25_def, 5_spell_dam, 80_spd );
          c.barehandedAttack = AttackType::HIT;
          c.viewId = ViewId::SUCCUBUS;
          c.spawnType = SpawnType::DEMON;
          c.body = Body::humanoidSpirit(Body::Size::LARGE).addWings();
          c.skills.insert(SkillId::COPULATION);
          c.gender = Gender::female;
          c.courage = -1;
          c.name = CreatureName("succubus", "succubi");
          );
    case CreatureId::DOPPLEGANGER:
      return CATTR(
          c.viewId = ViewId::DOPPLEGANGER;
          c.attr = LIST(25_def, 5_spell_dam, 80_spd );
          c.barehandedAttack = AttackType::HIT;
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.spawnType = SpawnType::DEMON;
          c.skills.insert(SkillId::CONSUMPTION);
          c.name = "doppelganger";
          );
    case CreatureId::WITCH: 
      return CATTR(
          c.viewId = ViewId::WITCH;
          c.attr = LIST(14_dam, 14_def, 20_spell_dam, 60_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.name = CreatureName("witch", "witches");
          c.name->setFirst("Cornelia");
          c.gender = Gender::female;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          );
    case CreatureId::WITCHMAN: 
      return CATTR(
          c.viewId = ViewId::WITCHMAN;
          c.attr = LIST(20_dam, 20_def, 20_spell_dam, 140_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = CreatureName("witchman", "witchmen");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.gender = Gender::male;
          c.chatReactionFriendly = "curses all monsters"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          );
    case CreatureId::CYCLOPS: 
      return CATTR(
          c.viewId = ViewId::CYCLOPS;
          c.attr = LIST(34_dam, 40_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE).setWeight(400);
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.barehandedAttack = AttackType::BITE;
          c.name = CreatureName("cyclops", "cyclopes");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::CYCLOPS)->getNext());
          );
    case CreatureId::DEMON_DWELLER:
      return CATTR(
        c.viewId = ViewId::DEMON_DWELLER;
        c.attr = LIST(25_dam, 30_def, 35_spell_dam, 120_spd );
        c.body = Body::humanoidSpirit(Body::Size::LARGE).addWings();
        c.permanentEffects[LastingEffect::FLYING] = 1;
        c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
        c.barehandedAttack = AttackType::HIT;
        c.courage = 100;
        c.gender = Gender::male;
        c.spells->add(SpellId::BLAST);
        c.chatReactionFriendly = "\"Kneel before us!\""_s;
        c.chatReactionHostile = "\"Face your death!\""_s;
        c.name = "Demon dweller";
        c.name->setFirst(NameGenerator::get(NameGeneratorId::DEMON)->getNext());
        c.name->setGroup("pack");
        );
    case CreatureId::DEMON_LORD:
      return CATTR(
        c.viewId = ViewId::DEMON_LORD;
        c.attr = LIST(40_dam, 45_def, 50_spell_dam, 130_spd );
        c.body = Body::humanoidSpirit(Body::Size::LARGE).addWings();
        c.permanentEffects[LastingEffect::FLYING] = 1;
        c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
        c.barehandedAttack = AttackType::HIT;
        c.courage = 100;
        c.gender = Gender::male;
        c.spells->add(SpellId::BLAST);
        c.chatReactionFriendly = "\"Kneel before us!\""_s;
        c.chatReactionHostile = "\"Face your death!\""_s;
        c.name = "Demon Lord";
        c.name->setFirst(NameGenerator::get(NameGeneratorId::DEMON)->getNext());
        c.name->setGroup("pack");
        );
    case CreatureId::MINOTAUR: 
      return CATTR(
          c.viewId = ViewId::MINOTAUR;
          c.attr = LIST(35_dam, 45_def, 90_spd );
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.body = Body::humanoid(Body::Size::LARGE).setWeight(400);
          c.barehandedAttack = AttackType::BITE;
          c.name = "minotaur";);
    case CreatureId::SOFT_MONSTER:
      return CATTR(
          c.viewId = ViewId::SOFT_MONSTER;
          c.attr = LIST(45_dam, 25_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE).setWeight(400);
          c.courage = -1;
          c.name = "soft monster";);
    case CreatureId::HYDRA:
      return CATTR(
          c.viewId = ViewId::HYDRA;
          c.attr = LIST(35_dam, 45_def, 110_spd );
          c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(400);
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.barehandedAttack = AttackType::BITE;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "hydra";);
    case CreatureId::SHELOB:
      return CATTR(
          c.viewId = ViewId::SHELOB;
          c.attr = LIST(48_dam, 38_def, 130_spd );
          c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(400)
              .setBodyParts({{BodyPart::LEG, 8}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.barehandedAttack = AttackType::BITE;
          c.skills.insert(SkillId::SPIDER);
          c.name = "giant spider";
          );
    case CreatureId::GREEN_DRAGON: 
      return CATTR(
          c.viewId = ViewId::GREEN_DRAGON;
          c.attr = LIST(52_dam, 40_def, 110_spd );
          c.body = Body::nonHumanoid(Body::Size::HUGE).setHorseBodyParts().addWings();
          c.barehandedAttack = AttackType::BITE;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::RANGED_VULNERABILITY] = 1;
          c.name = "green dragon";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DRAGON)->getNext());
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::CURE_POISON);
          c.spells->add(SpellId::DECEPTION);
          c.spells->add(SpellId::SPEED_SELF);
          c.name->setStack("dragon");
          );
    case CreatureId::RED_DRAGON:
      return CATTR(
          c.viewId = ViewId::RED_DRAGON;
          c.attr = LIST(55_dam, 42_def, 120_spd );
          c.body = Body::nonHumanoid(Body::Size::HUGE).setHorseBodyParts().addWings();
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.barehandedAttack = AttackType::BITE;
          c.name = "red dragon";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DRAGON)->getNext());
          c.permanentEffects[LastingEffect::RANGED_VULNERABILITY] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::CURE_POISON);
          c.spells->add(SpellId::DECEPTION);
          c.spells->add(SpellId::SPEED_SELF);
          c.name->setStack("dragon");
          );
    case CreatureId::KNIGHT: 
      return CATTR(
          c.viewId = ViewId::KNIGHT;
          c.attr = LIST(36_dam, 28_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "knight";);
    case CreatureId::AVATAR: 
      return CATTR(
          c.viewId = ViewId::DUKE;
          c.attr = LIST(45_dam, 29_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.courage = 1;
          c.name = "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext(););
    case CreatureId::ARCHER:
      return CATTR(
          c.viewId = ViewId::ARCHER;
          c.attr = LIST(17_dam, 22_def, 30_ranged_dam, 120_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "archer";);
    case CreatureId::WARRIOR:
      return CATTR(
          c.viewId = ViewId::WARRIOR;
          c.attr = LIST(27_dam, 19_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "warrior";);
    case CreatureId::SHAMAN:
      return CATTR(
          c.viewId = ViewId::SHAMAN;
          c.attr = LIST(27_dam, 19_def, 30_spell_dam, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.courage = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::SPEED_SELF);
          c.spells->add(SpellId::DEF_BONUS);
          c.spells->add(SpellId::SUMMON_SPIRIT);
          c.spells->add(SpellId::STUN_RAY);
          c.spells->add(SpellId::BLAST);
          c.spells->add(SpellId::HEAL_OTHER);
          c.skills.setValue(SkillId::SORCERY, 1);
          c.name = "shaman";);
    case CreatureId::PESEANT: 
      return CATTR(
          if (Random.roll(2)) {
            c.viewId = ViewId::PESEANT_WOMAN;
            c.gender = Gender::female;
          } else
            c.viewId = ViewId::PESEANT;
          c.attr = LIST(14_dam, 12_def, 80_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.innocent = true;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Heeelp!\""_s;
          c.skills.insert(SkillId::CROPS);
          c.name = "peasant";);
    case CreatureId::CHILD: 
      return CATTR(
          c.viewId = ViewId::CHILD;
          c.attr = LIST(8_dam, 8_def, 140_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.innocent = true;
          c.chatReactionFriendly = "\"plaaaaay!\""_s;
          c.chatReactionHostile = "\"Heeelp!\""_s;
          c.skills.insert(SkillId::CROPS);
          c.name = CreatureName("child", "children"););
    case CreatureId::CLAY_GOLEM:
      return CATTR(
          c.viewId = ViewId::CLAY_GOLEM;
          c.attr = LIST(14_dam, 12_def, 50_spd );
          c.body = Body::nonHumanoid(Body::Material::CLAY, Body::Size::LARGE).setHumanoidBodyParts();
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.barehandedAttack = AttackType::PUNCH;
          c.name = "clay golem";);
    case CreatureId::STONE_GOLEM: 
      return CATTR(
          c.viewId = ViewId::STONE_GOLEM;
          c.attr = LIST(16_dam, 14_def, 60_spd );
          c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::LARGE).setHumanoidBodyParts();
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.barehandedAttack = AttackType::PUNCH;
          c.name = "stone golem";);
    case CreatureId::IRON_GOLEM: 
      return CATTR(
          c.viewId = ViewId::IRON_GOLEM;
          c.attr = LIST(18_dam, 16_def, 70_spd );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.body = Body::nonHumanoid(Body::Material::IRON, Body::Size::LARGE).setHumanoidBodyParts();
          c.barehandedAttack = AttackType::PUNCH;
          c.name = "iron golem";);
    case CreatureId::LAVA_GOLEM: 
      return CATTR(
          c.viewId = ViewId::LAVA_GOLEM;
          c.attr = LIST(20_dam, 18_def, 80_spd );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.body = Body::nonHumanoid(Body::Material::LAVA, Body::Size::LARGE).setHumanoidBodyParts();
          c.barehandedAttack = AttackType::PUNCH;
          c.attackEffect = EffectId::FIRE;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.name = "lava golem";);
    case CreatureId::AUTOMATON: 
      return CATTR(
          c.viewId = ViewId::AUTOMATON;
          c.attr = LIST(45_dam, 45_def, 100_spd );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.body = Body::nonHumanoid(Body::Material::IRON, Body::Size::LARGE).setHumanoidBodyParts();
          c.barehandedAttack = AttackType::PUNCH;
          c.name = "automaton";);
    case CreatureId::ZOMBIE: 
      return CATTR(
          c.viewId = ViewId::ZOMBIE;
          c.attr = LIST(14_dam, 17_def, 60_spd );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.spawnType = SpawnType::UNDEAD;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.name = "zombie";);
    case CreatureId::SKELETON: 
      return CATTR(
          c.viewId = ViewId::SKELETON;
          c.attr = LIST(17_dam, 13_def, 100_spd, 5_ranged_dam);
          c.body = Body::humanoid(Body::Material::BONE, Body::Size::LARGE);
          c.spawnType = SpawnType::UNDEAD;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 4;
          c.name = "skeleton";);
    case CreatureId::VAMPIRE: 
      return CATTR(
          c.viewId = ViewId::VAMPIRE;
          c.attr = LIST(17_dam, 17_def, 17_spell_dam, 100_spd );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.chatReactionFriendly = "\"All men be cursed!\""_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spawnType = SpawnType::UNDEAD;
          c.skills.setValue(SkillId::SORCERY, 0.1);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 7;
          c.maxLevelIncrease[ExperienceType::SPELL] = 7;
          c.name = "vampire";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::VAMPIRE)->getNext());
          );
    case CreatureId::VAMPIRE_LORD: 
      return CATTR(
          c.viewId = ViewId::VAMPIRE_LORD;
          c.attr = LIST(17_dam, 17_def, 27_spell_dam, 120_spd );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.spawnType = SpawnType::UNDEAD;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.setValue(SkillId::SORCERY, 0.5);
          c.name = "vampire lord";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::VAMPIRE)->getNext());
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          c.maxLevelIncrease[ExperienceType::SPELL] = 12;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::DARKNESS_SOURCE] = 1;
          for (SpellId id : Random.chooseN(Random.get(3, 6), {SpellId::CIRCULAR_BLAST, SpellId::DEF_BONUS,
              SpellId::DAM_BONUS, SpellId::STUN_RAY, SpellId::DECEPTION, SpellId::DECEPTION,
              SpellId::TELEPORT}))
            c.spells->add(id);
          c.chatReactionFriendly = c.chatReactionHostile =
              "\"There are times when you simply cannot refuse a drink!\""_s;
          );
    case CreatureId::MUMMY: 
      return CATTR(
          c.viewId = ViewId::MUMMY;
          c.attr = LIST(15_dam, 14_def, 10_spell_dam, 60_spd );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.spawnType = SpawnType::UNDEAD;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.maxLevelIncrease[ExperienceType::SPELL] = 3;
          c.name = CreatureName("mummy", "mummies"););
    case CreatureId::ORC:
      return CATTR(
          c.viewId = ViewId::ORC;
          c.attr = LIST(16_dam, 14_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.spawnType = SpawnType::HUMANOID;
          c.chatReactionFriendly = "curses all elves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.skills.setValue(SkillId::WORKSHOP, 0.3);
          c.skills.setValue(SkillId::FORGE, 0.3);
          c.maxLevelIncrease[ExperienceType::MELEE] = 7;
          c.name = "orc";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          );
    case CreatureId::ORC_SHAMAN:
      return CATTR(
          c.viewId = ViewId::ORC_SHAMAN;
          c.attr = LIST(12_dam, 8_def, 16_spell_dam, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.spawnType = SpawnType::HUMANOID;
          c.skills.setValue(SkillId::SORCERY, 0.7);
          c.skills.setValue(SkillId::LABORATORY, 0.7);
          c.chatReactionFriendly = "curses all elves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.maxLevelIncrease[ExperienceType::MELEE] = 4;
          c.maxLevelIncrease[ExperienceType::SPELL] = 7;
          c.name = "orc shaman";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          );
    case CreatureId::HARPY:
      return CATTR(
          c.viewId = ViewId::HARPY;
          c.attr = LIST(13_dam, 16_def, 15_ranged_dam, 120_spd );
          c.body = Body::humanoid(Body::Size::LARGE).addWings();
          c.spawnType = SpawnType::HUMANOID;
          c.skills.setValue(SkillId::LABORATORY, 0.3);
          c.gender = Gender::female;
          c.maxLevelIncrease[ExperienceType::MELEE] = 4;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 7;
          c.name = CreatureName("harpy", "harpies");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          );
    case CreatureId::KOBOLD: 
      return CATTR(
          c.viewId = ViewId::KOBOLD;
          c.attr = LIST(12_dam, 13_def, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "kobold";);
    case CreatureId::GNOME: 
      return CATTR(
          c.viewId = ViewId::GNOME;
          c.attr = LIST(12_dam, 13_def, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "gnome";);
    case CreatureId::GNOME_CHIEF:
      return CATTR(
          c.viewId = ViewId::GNOME_BOSS;
          c.attr = LIST(15_dam, 16_def, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "gnome chief";);
    case CreatureId::GOBLIN: 
      return CATTR(
          c.viewId = ViewId::GOBLIN;
          c.attr = LIST(12_dam, 13_def, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.spawnType = SpawnType::HUMANOID;
          c.chatReactionFriendly = "talks about crafting"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.skills.insert(SkillId::DISARM_TRAPS);
          c.skills.setValue(SkillId::LABORATORY, 0.3);
          c.skills.setValue(SkillId::WORKSHOP, 0.9);
          c.skills.setValue(SkillId::FORGE, 0.9);
          c.skills.setValue(SkillId::JEWELER, 0.9);
          c.skills.setValue(SkillId::FURNACE, 0.9);
          c.name = "goblin";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          );
    case CreatureId::IMP: 
      return CATTR(
          c.viewId = ViewId::IMP;
          c.attr = LIST(5_dam, 15_def, 200_spd );
          c.body = Body::humanoid(Body::Size::SMALL).setNoCarryLimit().setDoesntEat();
          c.courage = -1;
          c.noChase = true;
          c.cantEquip = true;
          c.skills.insert(SkillId::CONSTRUCTION);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.moraleSpeedIncrease = 1.3;
          c.name = "imp";);
    case CreatureId::PRISONER:
      return CATTR(
          c.viewId = ViewId::PRISONER;
          c.attr = LIST(8_dam, 15_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE).setWeight(60).setNoCarryLimit();
          c.courage = -1;
          c.noChase = true;
          c.cantEquip = true;
          c.skills.insert(SkillId::CONSTRUCTION);
          c.chatReactionFriendly = "talks about escape plans"_s;
          c.name = "prisoner";);
    case CreatureId::OGRE: 
      return CATTR(
          c.viewId = ViewId::OGRE;
          c.attr = LIST(18_dam, 18_def, 80_spd );
          c.body = Body::humanoid(Body::Size::LARGE).setWeight(140);
          c.name = "ogre";
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          c.spawnType = SpawnType::HUMANOID;
          c.skills.setValue(SkillId::WORKSHOP, 0.5);
          c.skills.setValue(SkillId::FORGE, 0.5);
          c.skills.setValue(SkillId::FURNACE, 0.9);
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          );
    case CreatureId::CHICKEN: 
      return CATTR(
          c.viewId = ViewId::CHICKEN;
          c.attr = LIST(2_dam, 2_def, 50_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(3).setMinionFood();
          c.name = "chicken";);
    case CreatureId::DWARF: 
      return CATTR(
          c.viewId = ViewId::DWARF;
          c.attr = LIST(23_dam, 27_def, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM).setWeight(90);
          c.name = CreatureName("dwarf", "dwarves");
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DWARF)->getNext());
          c.chatReactionFriendly = "curses all orcs"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          );
    case CreatureId::DWARF_FEMALE:
      return CATTR(
          c.viewId = ViewId::DWARF_FEMALE;
          c.innocent = true;
          c.attr = LIST(23_dam, 27_def, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM).setWeight(90);
          c.name = CreatureName("dwarf", "dwarves");
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DWARF)->getNext());
          c.chatReactionFriendly = "curses all orcs"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.gender = Gender::female;);
    case CreatureId::DWARF_BARON: 
      return CATTR(
          c.viewId = ViewId::DWARF_BARON;
          c.attr = LIST(23_dam, 32_def, 90_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM).setWeight(120);
          c.chatReactionFriendly = "curses all orcs"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.courage = 1;
          c.name = "dwarf baron";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DWARF)->getNext());
          );
    case CreatureId::LIZARDMAN: 
      return CATTR(
          c.viewId = ViewId::LIZARDMAN;
          c.attr = LIST(25_dam, 14_def, 120_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.barehandedAttack = AttackType::BITE;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = CreatureName("lizardman", "lizardmen"););
    case CreatureId::LIZARDLORD: 
      return CATTR(
          c.viewId = ViewId::LIZARDLORD;
          c.attr = LIST(38_dam, 16_def, 140_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.barehandedAttack = AttackType::BITE;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.courage = 1;
          c.name = "lizardman chief";);
    case CreatureId::ELF: 
      return CATTR(
          c.viewId = Random.choose(ViewId::ELF, ViewId::ELF_WOMAN);
          c.attr = LIST(14_dam, 6_def, 100_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.innocent = true;
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.name = CreatureName("elf", "elves"););
    case CreatureId::ELF_ARCHER: 
      return CATTR(
          c.viewId = ViewId::ELF_ARCHER;
          c.attr = LIST(18_dam, 12_def, 25_ranged_dam, 120_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.name = "elven archer";);
    case CreatureId::ELF_CHILD: 
      return CATTR(
          c.viewId = ViewId::ELF_CHILD;
          c.attr = LIST(6_dam, 6_def, 120_spd );
          c.body = Body::humanoid(Body::Size::SMALL);
          c.innocent = true;
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.name = CreatureName("elf child", "elf children"););
    case CreatureId::ELF_LORD: 
      return CATTR(
          c.viewId = ViewId::ELF_LORD;
          c.attr = LIST(22_dam, 14_def, 16_spell_dam, 30_ranged_dam, 140_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.skills.setValue(SkillId::SORCERY, 1);
          c.spells->add(SpellId::HEAL_OTHER);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::SPEED_SELF);
          c.spells->add(SpellId::DAM_BONUS);
          c.spells->add(SpellId::DEF_BONUS);
          c.spells->add(SpellId::STUN_RAY);
          c.spells->add(SpellId::BLAST);
          c.name = "elf lord";);
    case CreatureId::DARK_ELF:
      return CATTR(
          c.viewId = Random.choose(ViewId::DARK_ELF, ViewId::DARK_ELF_WOMAN);
          c.attr = LIST(14_dam, 6_def, 100_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.innocent = true;
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.name = CreatureName("dark elf", "dark elves"););
    case CreatureId::DARK_ELF_WARRIOR:
      return CATTR(
          c.viewId = ViewId::DARK_ELF_WARRIOR;
          c.attr = LIST(18_dam, 12_def, 6_spell_dam, 120_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.setValue(SkillId::SORCERY, 0.3);
          c.maxLevelIncrease[ExperienceType::MELEE] = 5;
          c.maxLevelIncrease[ExperienceType::SPELL] = 5;
          c.name = CreatureName("dark elf", "dark elves"););
    case CreatureId::DARK_ELF_CHILD:
      return CATTR(
          c.viewId = ViewId::DARK_ELF_CHILD;
          c.attr = LIST(6_dam, 6_def, 120_spd );
          c.body = Body::humanoid(Body::Size::SMALL);
          c.innocent = true;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.name = CreatureName("dark elf child", "dark elf children"););
    case CreatureId::DARK_ELF_LORD:
      return CATTR(
          c.viewId = ViewId::DARK_ELF_LORD;
          c.attr = LIST(22_dam, 14_def, 16_spell_dam, 140_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.skills.setValue(SkillId::SORCERY, 1);
          c.spells->add(SpellId::HEAL_OTHER);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::SPEED_SELF);
          c.spells->add(SpellId::DEF_BONUS);
          c.spells->add(SpellId::DAM_BONUS);
          c.spells->add(SpellId::STUN_RAY);
          c.spells->add(SpellId::BLAST);
          c.name = "dark elf lord";);
    case CreatureId::DRIAD: 
      return CATTR(
          c.viewId = ViewId::DRIAD;
          c.attr = LIST(6_dam, 14_def, 25_ranged_dam, 80_spd );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.name = "driad";);
    case CreatureId::HORSE: 
      return CATTR(
          c.viewId = ViewId::HORSE;
          c.attr = LIST(16_dam, 7_def, 100_spd );
          c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(500).setHorseBodyParts();
          c.innocent = true;
          c.animal = true;
          c.noChase = true;
          c.name = "horse";);
    case CreatureId::COW: 
      return CATTR(
          c.viewId = ViewId::COW;
          c.attr = LIST(10_dam, 7_def, 40_spd );
          c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(400).setHorseBodyParts();
          c.innocent = true;
          c.animal = true;
          c.noChase = true;
          c.name = "cow";);
    case CreatureId::DONKEY: 
      return CATTR(
          c.viewId = ViewId::DONKEY;
          c.attr = LIST(10_dam, 7_def, 40_spd );
          c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(200).setHorseBodyParts()
              .setDeathSound(SoundId::DYING_DONKEY);
          c.innocent = true;
          c.animal = true;
          c.noChase = true;
          c.name = "donkey";);
    case CreatureId::PIG: 
      return CATTR(
          c.viewId = ViewId::PIG;
          c.attr = LIST(5_dam, 2_def, 60_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM).setWeight(150).setHorseBodyParts().setMinionFood()
              .setDeathSound(SoundId::DYING_PIG);
          c.innocent = true;
          c.noChase = true;
          c.animal = true;
          c.name = "pig";);
    case CreatureId::GOAT:
      return CATTR(
          c.viewId = ViewId::GOAT;
          c.attr = LIST(10_dam, 7_def, 60_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM).setHorseBodyParts().setMinionFood();
          c.innocent = true;
          c.noChase = true;
          c.animal = true;
          c.name = "goat";);
    case CreatureId::JACKAL: 
      return CATTR(
          c.viewId = ViewId::JACKAL;
          c.attr = LIST(15_dam, 10_def, 120_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(10).setHorseBodyParts();
          c.animal = true;
          c.name = "jackal";);
    case CreatureId::DEER: 
      return CATTR(
          c.viewId = ViewId::DEER;
          c.attr = LIST(10_dam, 10_def, 200_spd );
          c.body = Body::nonHumanoid(Body::Size::LARGE).setWeight(400).setHorseBodyParts();
          c.innocent = true;
          c.animal = true;
          c.noChase = true;
          c.name = CreatureName("deer", "deer"););
    case CreatureId::BOAR: 
      return CATTR(
          c.viewId = ViewId::BOAR;
          c.attr = LIST(15_dam, 10_def, 180_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM).setWeight(200).setHorseBodyParts();
          c.innocent = true;
          c.animal = true;
          c.noChase = true;
          c.name = "boar";);
    case CreatureId::FOX: 
      return CATTR(
          c.viewId = ViewId::FOX;
          c.attr = LIST(10_dam, 5_def, 140_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(10).setHorseBodyParts();
          c.innocent = true;
          c.animal = true;
          c.noChase = true;
          c.name = CreatureName("fox", "foxes"););
    case CreatureId::CAVE_BEAR:
      return CATTR(
          c.viewId = ViewId::BEAR;
          c.attr = LIST(26_dam, 10_def, 120_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM).setWeight(250).setHorseBodyParts();
          c.animal = true;
          c.spawnType = SpawnType::BEAST;
          c.skills.insert(SkillId::EXPLORE_CAVES);
          c.name = "cave bear";);
    case CreatureId::RAT: 
      return CATTR(
          c.viewId = ViewId::RAT;
          c.attr = LIST(2_dam, 2_def, 180_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(1).setHorseBodyParts();
          c.animal = true;
          c.noChase = true;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "rat";);
    case CreatureId::SPIDER: 
      return CATTR(
          c.viewId = ViewId::SPIDER;
          c.attr = LIST(9_dam, 13_def, 180_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL)
              .setWeight(0.3)
              .setBodyParts({{BodyPart::LEG, 8}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.animal = true;
          c.name = "spider";);
    case CreatureId::FLY: 
      return CATTR(
          c.viewId = ViewId::FLY;
          c.attr = LIST(2_dam, 12_def, 150_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL)
              .setWeight(0.1)
              .setBodyParts({{BodyPart::LEG, 6}, {BodyPart::WING, 2}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.courage = 1;
          c.noChase = true;
          c.animal = true;
          c.name = CreatureName("fly", "flies"););
    case CreatureId::ANT_WORKER:
      return CATTR(
          c.viewId = ViewId::ANT_WORKER;
          c.attr = LIST(16_dam, 16_def, 100_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM)
              .setWeight(10)
              .setBodyParts({{BodyPart::LEG, 6}, {BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.animal = true;
          c.name = "giant ant";);
    case CreatureId::ANT_SOLDIER:
      return CATTR(
          c.viewId = ViewId::ANT_SOLDIER;
          c.attr = LIST(36_dam, 20_def, 130_spd );
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.skills.insert(SkillId::DIGGING);
          c.body = Body::nonHumanoid(Body::Size::MEDIUM)
              .setWeight(10)
              .setBodyParts({{BodyPart::LEG, 6}, {BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.animal = true;
          c.name = "giant ant soldier";);
    case CreatureId::ANT_QUEEN:      
      return CATTR(
          c.viewId = ViewId::ANT_QUEEN;
          c.attr = LIST(42_dam, 26_def, 130_spd );
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.body = Body::nonHumanoid(Body::Size::MEDIUM)
              .setWeight(10)
              .setBodyParts({{BodyPart::LEG, 6}, {BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.animal = true;
          c.name = "ant queen";);
    case CreatureId::SNAKE: 
      return CATTR(
          c.viewId = ViewId::SNAKE;
          c.attr = LIST(14_dam, 14_def, 100_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL)
              .setWeight(2)
              .setBodyParts({{BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}})
              .setDeathSound(none);
          c.animal = true;
          c.attackEffect = EffectType(EffectId::LASTING, LastingEffect::POISON);
          c.skills.insert(SkillId::SWIMMING);
          c.name = "snake";);
    case CreatureId::RAVEN: 
      return CATTR(
          c.viewId = ViewId::RAVEN;
          c.attr = LIST(2_dam, 12_def, 250_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(0.5).setBirdBodyParts().setDeathSound(none);
          c.animal = true;
          c.noChase = true;
          c.courage = 1;
          c.spawnType = SpawnType::BEAST;
          c.skills.insert(SkillId::EXPLORE);
          c.name = "raven";
          c.name->setGroup("flock");
          );
    case CreatureId::VULTURE: 
      return CATTR(
          c.viewId = ViewId::VULTURE;
          c.attr = LIST(2_dam, 12_def, 80_spd );
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(5).setBirdBodyParts().setDeathSound(none);
          c.animal = true;
          c.noChase = true;
          c.courage = 1;
          c.spawnType = SpawnType::BEAST;
          c.name = "vulture";);
    case CreatureId::WOLF: 
      return CATTR(
          c.viewId = ViewId::WOLF;
          c.attr = LIST(20_dam, 9_def, 160_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM).setWeight(35).setHorseBodyParts();
          c.animal = true;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.spawnType = SpawnType::BEAST;
          c.name = CreatureName("wolf", "wolves");
          c.name->setGroup("pack");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
          c.skills.insert(SkillId::EXPLORE_NOCTURNAL);
          );    
    case CreatureId::WEREWOLF:
      return CATTR(
          c.viewId = ViewId::WEREWOLF;
          c.attr = LIST(20_dam, 9_def, 100_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.animal = true;
          c.spawnType = SpawnType::BEAST;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.insert(SkillId::STEALTH);
          c.skills.insert(SkillId::EXPLORE_NOCTURNAL);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          c.name = CreatureName("werewolf", "werewolves");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
          );
    case CreatureId::DOG: 
      return CATTR(
          c.viewId = ViewId::DOG;
          c.attr = LIST(18_dam, 7_def, 160_spd );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM).setWeight(25).setHorseBodyParts();
          c.animal = true;
          c.innocent = true;
          c.name = "dog";
          c.name->setGroup("pack");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
          );
    case CreatureId::FIRE_SPHERE: 
      return CATTR(
          c.viewId = ViewId::FIRE_SPHERE;
          c.attr = LIST(5_dam, 15_def, 100_spd );
          c.body = Body::nonHumanoid(Body::Material::FIRE, Body::Size::SMALL).setDeathSound(none);
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.name = "fire sphere";);
    case CreatureId::ELEMENTALIST: 
      return CATTR(
          c.viewId = ViewId::ELEMENTALIST;
          c.attr = LIST(5_dam, 25_def, 15_spell_dam, 120_spd );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.gender = Gender::female;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "elementalist";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_FEMALE)->getNext());
          );
    case CreatureId::FIRE_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::FIRE_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::FIRE, Body::Size::LARGE).setDeathSound(none);
          c.attr = LIST(25_dam, 30_def, 120_spd );
          c.barehandedAttack = AttackType::HIT;
          c.attackEffect = EffectId::FIRE;
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.name = "fire elemental";);
    case CreatureId::AIR_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::AIR_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::SPIRIT, Body::Size::LARGE).setDeathSound(none);
          c.attr = LIST(25_dam, 35_def, 160_spd );
          c.barehandedAttack = AttackType::HIT;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.spells->add(SpellId::CIRCULAR_BLAST);
          c.name = "air elemental";);
    case CreatureId::EARTH_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::EARTH_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::LARGE).setWeight(500)
              .setHumanoidBodyParts().setDeathSound(none);
          c.attr = LIST(25_dam, 45_def, 80_spd );
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.barehandedAttack = AttackType::HIT;
          c.name = "earth elemental";);
    case CreatureId::WATER_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::WATER_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::WATER, Body::Size::LARGE).setWeight(300).setHumanoidBodyParts()
              .setDeathSound(none);
          c.attr = LIST(45_dam, 15_def, 80_spd );
          c.barehandedAttack = AttackType::HIT;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "water elemental";);
    case CreatureId::ENT:
      return CATTR(
          c.viewId = ViewId::ENT;
          c.body = Body::nonHumanoid(Body::Material::WOOD, Body::Size::HUGE).setHumanoidBodyParts();
          c.attr = LIST(45_dam, 25_def, 30_spd );
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "tree spirit";);
    case CreatureId::ANGEL:
      return CATTR(
          c.viewId = ViewId::ANGEL;
          c.attr = LIST(22_def, 20_spell_dam, 100_spd );
          c.body = Body::nonHumanoid(Body::Material::SPIRIT, Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "angel";);
    case CreatureId::KRAKEN:
      return getKrakenAttributes(ViewId::KRAKEN_HEAD, "kraken");
    case CreatureId::BAT: 
      return CATTR(
          c.viewId = ViewId::BAT;
          c.body = Body::nonHumanoid(Body::Size::SMALL).setWeight(1).setBirdBodyParts();
          c.attr = LIST(3_dam, 16_def, 150_spd );
          c.animal = true;
          c.noChase = true;
          c.courage = 1;
          c.spawnType = SpawnType::BEAST;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.insert(SkillId::EXPLORE_NOCTURNAL);
          c.skills.insert(SkillId::EXPLORE_CAVES);
          c.name = "bat";);
    case CreatureId::DEATH: 
      return CATTR(
          c.viewId = ViewId::DEATH;
          c.attr = LIST(100_spell_dam, 35_def, 95_spd );
          c.body = Body::humanoidSpirit(Body::Size::LARGE);
          c.chatReactionFriendly = c.chatReactionHostile = "\"IN ORDER TO HAVE A CHANGE OF FORTUNE AT THE LAST MINUTE "
              "YOU HAVE TO TAKE YOUR FORTUNE TO THE LAST MINUTE.\""_s;
          c.name = "Death";);
    default: FATAL << "This is not handled here " << int(id);
  }
  FATAL << "unhandled case";
  return CreatureAttributes([](CreatureAttributes&) {});
}

ControllerFactory getController(CreatureId id, MonsterAIFactory normalFactory) {
  switch (id) {
    case CreatureId::KRAKEN:
      return ControllerFactory([=](WCreature c) {
          return makeOwner<KrakenController>(c);
          });
    case CreatureId::FIRE_SPHERE:
      return ControllerFactory([=](WCreature c) {
          return makeOwner<KamikazeController>(c, normalFactory);
          });
    default: return Monster::getFactory(normalFactory);
  }
}

PCreature CreatureFactory::get(CreatureId id, TribeId tribe, MonsterAIFactory aiFactory) {
  ControllerFactory factory = Monster::getFactory(aiFactory);
  switch (id) {
    case CreatureId::SPECIAL_BLBN:
      return getSpecial(tribe, false, true, true, false, factory);
    case CreatureId::SPECIAL_BLBW:
      return getSpecial(tribe, false, true, true, true, factory);
    case CreatureId::SPECIAL_BLGN:
      return getSpecial(tribe, false, true, false, false, factory);
    case CreatureId::SPECIAL_BLGW:
      return getSpecial(tribe, false, true, false, true, factory);
    case CreatureId::SPECIAL_BMBN:
      return getSpecial(tribe, false, false, true, false, factory);
    case CreatureId::SPECIAL_BMBW:
      return getSpecial(tribe, false, false, true, true, factory);
    case CreatureId::SPECIAL_BMGN:
      return getSpecial(tribe, false, false, false, false, factory);
    case CreatureId::SPECIAL_BMGW:
      return getSpecial(tribe, false, false, false, true, factory);
    case CreatureId::SPECIAL_HLBN:
      return getSpecial(tribe, true, true, true, false, factory);
    case CreatureId::SPECIAL_HLBW:
      return getSpecial(tribe, true, true, true, true, factory);
    case CreatureId::SPECIAL_HLGN:
      return getSpecial(tribe, true, true, false, false, factory);
    case CreatureId::SPECIAL_HLGW:
      return getSpecial(tribe, true, true, false, true, factory);
    case CreatureId::SPECIAL_HMBN:
      return getSpecial(tribe, true, false, true, false, factory);
    case CreatureId::SPECIAL_HMBW:
      return getSpecial(tribe, true, false, true, true, factory);
    case CreatureId::SPECIAL_HMGN:
      return getSpecial(tribe, true, false, false, false, factory);
    case CreatureId::SPECIAL_HMGW:
      return getSpecial(tribe, true, false, false, true, factory);
    case CreatureId::SOKOBAN_BOULDER:
      return getSokobanBoulder(tribe);
    default: return get(getAttributes(id), tribe, getController(id, aiFactory));
  }
}

PCreature CreatureFactory::getGhost(WCreature creature) {
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, "Ghost");
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  auto ret = makeOwner<Creature>(viewObject, creature->getTribeId(), getAttributes(CreatureId::LOST_SOUL));
  ret->setController(Monster::getFactory(MonsterAIFactory::monster()).get(ret.get()));
  return ret;
}

ItemType randomHealing() {
  return ItemType(ItemId::POTION, EffectId::HEAL);
}

ItemType randomBackup() {
  return Random.choose(ItemType(ItemId::SCROLL, EffectId::DECEPTION), ItemType(ItemId::SCROLL, EffectId::TELEPORT),
      randomHealing());
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

vector<ItemType> getDefaultInventory(CreatureId id) {
  switch (id) {
    case CreatureId::CYCLOPS:
      return ItemList()
        .add(ItemId::HEAVY_CLUB)
        .add(ItemId::GOLD_PIECE, Random.get(40, 80));
    case CreatureId::GREEN_DRAGON:
      return ItemList().add(ItemId::GOLD_PIECE, Random.get(60, 100));
    case CreatureId::DEMON_DWELLER:
      return ItemList().add(ItemId::GOLD_PIECE, Random.get(50, 100));
    case CreatureId::RED_DRAGON:
      return ItemList().add(ItemId::GOLD_PIECE, Random.get(120, 200));
    case CreatureId::DEMON_LORD:
    case CreatureId::ANGEL:
      return ItemList().add(ItemId::SPECIAL_SWORD);
    case CreatureId::KEEPER_F:
    case CreatureId::KEEPER:
      return ItemList()
          .add(ItemId::ROBE);
    case CreatureId::ADVENTURER_F:
    case CreatureId::ADVENTURER:
      return ItemList()
        .add(ItemId::FIRST_AID_KIT, 3)
        .add(ItemId::KNIFE)
        .add(ItemId::SWORD)
        .add(ItemId::LEATHER_GLOVES)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::LEATHER_HELM)
        .add(ItemId::GOLD_PIECE, Random.get(16, 26));
    case CreatureId::DEATH:
      return ItemList()
        .add(ItemId::SCYTHE);
    case CreatureId::GOBLIN: 
      return ItemList()
        .add(ItemId::CLUB)
        .maybe(0.3, ItemId::LEATHER_BOOTS);
    case CreatureId::WARRIOR: 
      return ItemList()
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::CLUB)
        .add(ItemId::GOLD_PIECE, Random.get(2, 5));
    case CreatureId::SHAMAN: 
      return ItemList()
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::CLUB)
        .add(ItemId::GOLD_PIECE, Random.get(80, 120));
    case CreatureId::LIZARDLORD:
      return ItemList().add(ItemId::LEATHER_ARMOR)
        .add({ItemId::POTION, EffectType(EffectId::REGROW_BODY_PART)})
        .add(ItemId::GOLD_PIECE, Random.get(50, 90));
    case CreatureId::LIZARDMAN:
      return ItemList().add(ItemId::LEATHER_ARMOR)
        .add(ItemId::GOLD_PIECE, Random.get(2, 4));
    case CreatureId::HARPY: 
      return ItemList()
        .add(ItemId::BOW);
    case CreatureId::ARCHER: 
      return ItemList()
        .add(ItemId::BOW)
        .add(ItemId::KNIFE)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.get(4, 10));
    case CreatureId::WITCHMAN:
      return ItemList()
        .add(ItemId::SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add({ItemId::POTION, EffectType{EffectId::LASTING, LastingEffect::SPEED}}, 4)
        .add(ItemId::GOLD_PIECE, Random.get(60, 80));
    case CreatureId::KNIGHT: 
      return ItemList()
        .add(ItemId::SWORD)
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::LEATHER_BOOTS)
        .add(randomHealing())
        .add(ItemId::GOLD_PIECE, Random.get(6, 16));
    case CreatureId::MINOTAUR: 
      return ItemList()
        .add(ItemId::BATTLE_AXE);
    case CreatureId::AVATAR: 
      return ItemList()
        .add(ItemId::SPECIAL_BATTLE_AXE)
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::IRON_HELM)
        .add(ItemId::IRON_BOOTS)
        .add(randomHealing(), 3)
        .add(ItemId::GOLD_PIECE, Random.get(140, 200));
    case CreatureId::OGRE: 
      return ItemList().add(ItemId::HEAVY_CLUB);
    case CreatureId::BANDIT:
      return ItemList()
        .add(ItemId::SWORD)
        .maybe(0.3, randomBackup())
        .maybe(0.05, ItemList().add(ItemId::BOW));
    case CreatureId::DWARF:
      return ItemList()
        .add(Random.choose({ItemId::BATTLE_AXE, ItemId::WAR_HAMMER}, {1, 1}))
        .maybe(0.6, randomBackup())
        .add(ItemId::CHAIN_ARMOR)
        .maybe(0.5, ItemId::IRON_HELM)
        .maybe(0.3, ItemId::IRON_BOOTS)
        .add(ItemId::GOLD_PIECE, Random.get(2, 6));
    case CreatureId::DWARF_BARON: 
      return ItemList()
        .add(Random.choose({ItemId::SPECIAL_BATTLE_AXE, ItemId::SPECIAL_WAR_HAMMER}, {1, 1}))
        .add(randomBackup())
        .add(randomHealing())
        .add(ItemId::CHAIN_ARMOR)
        .add(ItemId::IRON_BOOTS)
        .add(ItemId::IRON_HELM)
        .add(ItemId::GOLD_PIECE, Random.get(80, 120));
    case CreatureId::GNOME_CHIEF:
      return ItemList()
        .add(ItemId::SWORD)
        .add(randomBackup());
    case CreatureId::DARK_ELF_LORD: 
    case CreatureId::ELF_LORD: 
      return ItemList()
        .add(ItemId::SPECIAL_ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::ELVEN_BOW)
        .add(ItemId::GOLD_PIECE, Random.get(80, 120))
        .add(randomBackup());
    case CreatureId::DRIAD: 
      return ItemList()
        .add(ItemId::BOW);
    case CreatureId::DARK_ELF_WARRIOR: 
      return ItemList()
        .add(ItemId::ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::GOLD_PIECE, Random.get(2, 6))
        .add(randomBackup());
    case CreatureId::ELF_ARCHER: 
      return ItemList()
        .add(ItemId::ELVEN_SWORD)
        .add(ItemId::LEATHER_ARMOR)
        .add(ItemId::BOW)
        .add(ItemId::GOLD_PIECE, Random.get(2, 6))
        .add(randomBackup());
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


PCreature CreatureFactory::fromId(CreatureId id, TribeId t, const MonsterAIFactory& f) {
  return fromId(id, t, f, {});
}

PCreature CreatureFactory::fromId(CreatureId id, TribeId t, const MonsterAIFactory& factory, const vector<ItemType>& inventory) {
  auto ret = get(id, t, factory);
  addInventory(ret.get(), inventory);
  addInventory(ret.get(), getDefaultInventory(id));
  return ret;
}

