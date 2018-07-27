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
#include "minion_activity_map.h"
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
#include "furniture.h"
#include "experience_type.h"
#include "creature_debt.h"
#include "effect.h"

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
    Position nextPos = creature->getPosition().plus(direction);
    if (WCreature c = nextPos.getCreature()) {
      if (!c->getBody().isKilledByBoulder()) {
        if (nextPos.canEnterEmpty(creature)) {
          creature->swapPosition(direction);
          return;
        }
      } else {
        health -= c->getBody().getBoulderDamage();
        if (health <= 0) {
          nextPos.globalMessage(creature->getName().the() + " crashes on " + c->getName().the());
          nextPos.unseenMessage("You hear a crash");
          creature->dieNoReason();
          //c->takeDamage(Attack(creature, AttackLevel::MIDDLE, AttackType::HIT, 1000, AttrType::DAMAGE));
          return;
        } else {
          c->you(MsgType::KILLED_BY, creature->getName().the());
          c->dieWithAttacker(creature);
        }
      }
    }
    if (auto furniture = nextPos.getFurniture(FurnitureLayer::MIDDLE))
      if (furniture->canDestroy(creature->getMovementType(), DestroyAction::Type::BOULDER) &&
          *furniture->getStrength(DestroyAction::Type::BOULDER) <
          health * creature->getAttr(AttrType::DAMAGE)) {
        health -= *furniture->getStrength(DestroyAction::Type::BOULDER) /
            (double) creature->getAttr(AttrType::DAMAGE);
        creature->destroyImpl(direction, DestroyAction::Type::BOULDER);
      }
    if (auto action = creature->move(direction))
      action.perform(creature);
    else {
      nextPos.globalMessage(creature->getName().the() + " crashes on the " + nextPos.getName());
      nextPos.unseenMessage("You hear a crash");
      creature->dieNoReason();
      return;
    }
    health -= 0.2;
    if (health <= 0 && !creature->isDead())
      creature->dieNoReason();
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
            c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::HUGE);
            c.body->setDeathSound(none);
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
            c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::HUGE);
            c.body->setDeathSound(none);
            c.body->setCanAlwaysPush();
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.boulder = true;
            c.name = "boulder";));
  ret->setController(makeOwner<SokobanController>(ret.get()));
  return ret;
}

CreatureAttributes CreatureFactory::getKrakenAttributes(ViewId id, const char* name) {
  return CATTR(
      c.viewId = id;
      c.body = Body::nonHumanoid(Body::Size::LARGE);
      c.body->setDeathSound(none);
      c.attr[AttrType::DAMAGE] = 28;
      c.attr[AttrType::DEFENSE] = 28;
      c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
      c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
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

  virtual bool dontReplaceInCollective() override {
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
    held->you(MsgType::HAPPENS_TO, creature->getName().the() + " pulls");
    if (father) {
      held->setHeld(father->creature);
      Vec2 pullDir = held->getPosition().getDir(creature->getPosition());
      creature->dieNoReason(Creature::DropType::NOTHING);
      held->displace(pullDir);
    } else {
      held->you(MsgType::ARE, "eaten by " + creature->getName().the());
      held->dieNoReason();
    }
  }

  WCreature getHeld() {
    for (auto pos : creature->getPosition().neighbors8())
      if (auto other = pos.getCreature())
        if (other->getHoldingCreature() == creature)
          return other;
    return nullptr;
  }

  WCreature getVisibleEnemy() {
    const int radius = 10;
    WCreature ret = nullptr;
    auto myPos = creature->getPosition();
    for (Position pos : creature->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), radius)))
      if (WCreature c = pos.getCreature())
        if (c->getAttributes().getCreatureId() != creature->getAttributes().getCreatureId() &&
            (!ret || ret->getPosition().dist8(myPos) > c->getPosition().dist8(myPos)) &&
            creature->canSee(c) && creature->isEnemy(c) && !c->getHoldingCreature())
          ret = c;
    return ret;
  }

  void considerAttacking(WCreature c) {
    auto pos = c->getPosition();
    Vec2 v = creature->getPosition().getDir(pos);
    if (v.length8() == 1) {
      c->you(MsgType::HAPPENS_TO, creature->getName().the() + " swings itself around");
      c->setHeld(creature);
    } else {
      pair<Vec2, Vec2> dirs = v.approxL1();
      vector<Vec2> moves;
      if (creature->getPosition().plus(dirs.first).canEnter(
            {{MovementTrait::WALK, MovementTrait::SWIM}}))
        moves.push_back(dirs.first);
      if (creature->getPosition().plus(dirs.second).canEnter(
            {{MovementTrait::WALK, MovementTrait::SWIM}}))
        moves.push_back(dirs.second);
      if (!moves.empty() && Random.roll(2)) {
        Vec2 move = Random.choose(moves);
        ViewId viewId = creature->getPosition().plus(move).canEnter({MovementTrait::SWIM})
          ? ViewId::KRAKEN_WATER : ViewId::KRAKEN_LAND;
        auto spawn = makeOwner<Creature>(creature->getTribeId(),
              CreatureFactory::getKrakenAttributes(viewId, "kraken tentacle"));
        spawn->setController(makeOwner<KrakenController>(spawn.get(), getThis().dynamicCast<KrakenController>()));
        spawns.push_back(spawn.get());
        creature->getPosition().plus(move).addCreature(std::move(spawn));
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
        creature->dieNoReason(Creature::DropType::NOTHING);
        return;
      }
    }
    creature->wait().perform(creature);
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
    for (Position pos : creature->getPosition().neighbors8())
      if (WCreature c = pos.getCreature())
        if (creature->isEnemy(c) && creature->canSee(c)) {
          creature->thirdPerson(creature->getName().the() + " explodes!");
          for (Position v : c->getPosition().neighbors8())
            v.fireDamage(1);
          c->getPosition().fireDamage(1);
          creature->dieNoReason(Creature::DropType::ONLY_INVENTORY);
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
      myLevel = creature->getLevel();
      subscribeTo(creature->getPosition().getModel());
      for (Position v : getAllShopPositions()) {
        for (WItem item : v.getItems())
          item->setShopkeeper(creature);
        v.clearItemIndex(ItemIndex::FOR_SALE);
      }
      firstMove = false;
    }
    if (!creature->getPosition().isSameLevel(myLevel)) {
      Monster::makeMove();
      return;
    }
    vector<Creature::Id> creatures;
    for (Position v : getAllShopPositions())
      if (WCreature c = v.getCreature()) {
        creatures.push_back(c->getUniqueId());
        if (!prevCreatures.contains(c) && !thieves.contains(c) && !creature->isEnemy(c)) {
          if (!debtors.contains(c))
            c->secondPerson("\"Welcome to " + *creature->getName().first() + "'s shop!\"");
          else {
            c->secondPerson("\"Pay your debt or... !\"");
            thiefCount.erase(c);
          }
        }
      }
    for (auto debtorId : copyOf(debtors))
      if (!creatures.contains(debtorId))
        for (auto pos : creature->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), 30)))
          if (auto debtor = pos.getCreature())
            if (debtor->getUniqueId() == debtorId) {
              debtor->privateMessage("\"Come back, you owe me " + toString(debtor->getDebt().getAmountOwed(creature)) +
                  " gold!\"");
              if (++thiefCount.getOrInit(debtor) == 4) {
                debtor->privateMessage("\"Thief! Thief!\"");
                creature->getTribe()->onItemsStolen(debtor);
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
    from->getDebt().add(creature, -paid);
    if (from->getDebt().getAmountOwed(creature) <= 0)
      debtors.erase(from);
  }
  
  void onEvent(const GameEvent& event) {
    using namespace EventInfo;
    event.visit(
        [&](const ItemsAppeared& info) {
          if (isShopPosition(info.position)) {
            for (auto& it : info.items) {
              it->setShopkeeper(creature);
              info.position.clearItemIndex(ItemIndex::FOR_SALE);
            }
          }
        },
        [&](const ItemsPickedUp& info) {
          if (isShopPosition(info.creature->getPosition())) {
            for (auto& item : info.items)
              if (item->isShopkeeper(creature)) {
                info.creature->getDebt().add(creature, item->getPrice());
                debtors.insert(info.creature);
              }
          }
        },
        [&](const ItemsDropped& info) {
          if (isShopPosition(info.creature->getPosition())) {
            for (auto& item : info.items)
              if (item->isShopkeeper(creature)) {
                info.creature->getDebt().add(creature, -item->getPrice());
                if (info.creature->getDebt().getAmountOwed(creature) == 0)
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
    c->take(item.get());
}

PCreature CreatureFactory::getShopkeeper(Rectangle shopArea, TribeId tribe) {
  auto ret = makeOwner<Creature>(tribe,
      CATTR(
        c.viewId = ViewId::SHOPKEEPER;
        c.body = Body::humanoid(Body::Size::LARGE);
        c.attr[AttrType::DAMAGE] = 17;
        c.attr[AttrType::DEFENSE] = 20;
        c.chatReactionFriendly = "complains about high import tax"_s;
        c.chatReactionHostile = "\"Die!\""_s;
        c.name = "shopkeeper";
        c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());));
  ret->setController(makeOwner<ShopkeeperController>(ret.get(), shopArea));
  vector<ItemType> inventory(Random.get(20, 60), ItemType::GoldPiece{});
  inventory.push_back(ItemType::Sword{});
  inventory.push_back(ItemType::LeatherArmor{});
  inventory.push_back(ItemType::LeatherBoots{});
  inventory.push_back(ItemType::Potion{Effect::Heal{}});
  inventory.push_back(ItemType::Potion{Effect::Heal{}});
  addInventory(ret.get(), inventory);
  return ret;
}

class IllusionController : public DoNothingController {
  public:
  IllusionController(WCreature c, GlobalTime deathT) : DoNothingController(c), deathTime(deathT) {}

  virtual void onKilled(WConstCreature attacker) override {
    if (attacker)
      attacker->message("It was just an illusion!");
  }

  virtual void makeMove() override {
    if (*creature->getGlobalTime() >= deathTime) {
      creature->message("The illusion disappears.");
      creature->dieNoReason();
    } else
      creature->wait().perform(creature);
  }

  SERIALIZE_ALL(SUBCLASS(DoNothingController), deathTime)
  SERIALIZATION_CONSTRUCTOR(IllusionController)

  private:
  GlobalTime SERIAL(deathTime);
};

PCreature CreatureFactory::getIllusion(WCreature creature) {
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, "Illusion");
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  auto ret = makeOwner<Creature>(viewObject, creature->getTribeId(), CATTR(
          c.viewId = ViewId::ROCK; //overriden anyway
          c.illusionViewObject = creature->getViewObject();
          c.illusionViewObject->setModifier(ViewObject::Modifier::INVISIBLE, false);
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.body->setDeathSound(SoundId::MISSED_ATTACK);
          c.attr[AttrType::DAMAGE] = 20; // just so it's not ignored by creatures
          c.attr[AttrType::DEFENSE] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.noAttackSound = true;
          c.canJoinCollective = false;
          c.name = creature->getName();));
  ret->setController(makeOwner<IllusionController>(ret.get(), *creature->getGlobalTime()
      + TimeInterval(Random.get(5, 10))));
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

PCreature CreatureFactory::get(CreatureAttributes attr, TribeId tribe, const ControllerFactory& factory) {
  auto ret = makeOwner<Creature>(tribe, std::move(attr));
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

CreatureFactory CreatureFactory::forrest(TribeId tribe) {
  return CreatureFactory(tribe,
      { CreatureId::DEER, CreatureId::FOX, CreatureId::BOAR },
      { 4, 2, 2}, {});
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

static optional<ItemType> getSpecialBeastAttack(bool large, bool body, bool wings) {
  static vector<optional<ItemType>> attacks {
    none,
    ItemType(ItemType::fangs(7, Effect::Fire{})),
    ItemType(ItemType::fangs(7, Effect::Fire{})),
    none,
    ItemType(ItemType::fangs(7, Effect::Lasting{LastingEffect::POISON})),
    none,
    ItemType(ItemType::fangs(7, Effect::Lasting{LastingEffect::POISON})),
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
        c.body = std::move(body);
        c.attr[AttrType::DAMAGE] = Random.get(18, 24);
        c.attr[AttrType::DEFENSE] = Random.get(18, 24);
        c.attr[AttrType::SPELL_DAMAGE] = Random.get(18, 24);
        for (auto effect : getResistanceAndVulnerability(Random))
          c.permanentEffects[effect] = 1;
        if (large) {
          c.attr[AttrType::DAMAGE] += 6;
          c.attr[AttrType::DEFENSE] += 2;
          c.attr[AttrType::SPELL_DAMAGE] -= 6;
        }
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
          c.chatReactionFriendly = c.chatReactionHostile = c.petReaction = "snarls."_s;
        }
        c.name = name;
        c.name->setStack(humanoid ? "legendary humanoid" : "legendary beast");
        c.name->setFirst(NameGenerator::get(NameGeneratorId::DEMON)->getNext());
        if (!humanoid) {
          c.body->setBodyParts(getSpecialBeastBody(large, living, wings));
          c.attr[AttrType::DAMAGE] += 5;
          c.attr[AttrType::DEFENSE] += 5;
          if (auto attack = getSpecialBeastAttack(large, living, wings))
            c.body->setIntrinsicAttack(BodyPart::HEAD, *attack);
        }
        if (Random.roll(3))
          c.skills.insert(SkillId::SWIMMING);
        ), tribe, factory);
  if (body.isHumanoid()) {
    if (Random.roll(4))
      c->take(ItemType(ItemType::Bow{}).get());
    c->take(Random.choose(
          ItemType(ItemType::Sword{}).setPrefixChance(1),
          ItemType(ItemType::BattleAxe{}).setPrefixChance(1),
          ItemType(ItemType::WarHammer{}).setPrefixChance(1))
        .get());
  }
  return c;
}

CreatureAttributes CreatureFactory::getAttributes(CreatureId id) {
  auto ret = getAttributesFromId(id);
  ret.setCreatureId(id);
  return ret;
}

#define CREATE_LITERAL(NAME, SHORT) \
static pair<AttrType, int> operator "" _##SHORT(unsigned long long value) {\
  return {AttrType::NAME, value};\
}

CREATE_LITERAL(DAMAGE, dam)
CREATE_LITERAL(DEFENSE, def)
CREATE_LITERAL(SPELL_DAMAGE, spell_dam)
CREATE_LITERAL(RANGED_DAMAGE, ranged_dam)

#undef CREATE_LITERAL

CreatureAttributes CreatureFactory::getAttributesFromId(CreatureId id) {
  switch (id) {
    case CreatureId::KEEPER_MAGE:
      return CATTR(
          c.viewId = ViewId::KEEPER;
          c.retiredViewId = ViewId::RETIRED_KEEPER;
          c.attr = LIST(12_dam, 12_def, 20_spell_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = "Keeper";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());
          c.name->useFullTitle();
          c.skills.setValue(SkillId::SORCERY, 0.2);
          c.skills.setValue(SkillId::LABORATORY, 0.2);
          c.maxLevelIncrease[ExperienceType::MELEE] = 7;
          c.maxLevelIncrease[ExperienceType::SPELL] = 12;
          c.spells->add(SpellId::HEAL_SELF);
      );
    case CreatureId::KEEPER_MAGE_F:
      return CATTR(
          c.viewId = ViewId::KEEPER_F;
          c.retiredViewId = ViewId::RETIRED_KEEPER_F;
          c.attr = LIST(12_dam, 12_def, 20_spell_dam );
          c.gender = Gender::female;
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = "Keeper";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_FEMALE)->getNext());
          c.name->useFullTitle();
          c.skills.setValue(SkillId::SORCERY, 0.2);
          c.skills.setValue(SkillId::LABORATORY, 0.2);
          c.maxLevelIncrease[ExperienceType::MELEE] = 7;
          c.maxLevelIncrease[ExperienceType::SPELL] = 12;
          c.spells->add(SpellId::HEAL_SELF);
      );
    case CreatureId::KEEPER_KNIGHT:
      return CATTR(
          c.viewId = ViewId::KEEPER_KNIGHT;
          c.retiredViewId = ViewId::RETIRED_KEEPER_KNIGHT;
          c.attr = LIST(20_dam, 16_def);
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = "Keeper";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_MALE)->getNext());
          c.name->useFullTitle();
          c.skills.setValue(SkillId::FORGE, 0.2);
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          c.maxLevelIncrease[ExperienceType::SPELL] = 3;
          c.spells->add(SpellId::HEAL_SELF);
      );
    case CreatureId::KEEPER_KNIGHT_F:
      return CATTR(
          c.viewId = ViewId::KEEPER_KNIGHT_F;
          c.retiredViewId = ViewId::RETIRED_KEEPER_KNIGHT_F;
          c.attr = LIST(20_dam, 16_def);
          c.body = Body::humanoid(Body::Size::LARGE);
          c.name = "Keeper";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::FIRST_FEMALE)->getNext());
          c.name->useFullTitle();
          c.skills.setValue(SkillId::FORGE, 0.2);
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          c.maxLevelIncrease[ExperienceType::SPELL] = 3;
          c.spells->add(SpellId::HEAL_SELF);
      );
    case CreatureId::ADVENTURER:
      return CATTR(
          c.viewId = ViewId::PLAYER;
          c.attr = LIST(15_dam, 20_def );
          c.body = Body::humanoid(Body::Size::LARGE);
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
          c.attr = LIST(15_dam, 20_def );
          c.body = Body::humanoid(Body::Size::LARGE);
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
        c.attr = LIST(16_dam, 20_def, 20_spell_dam);
        c.body = Body::nonHumanoid(Body::Size::LARGE);
        c.body->setWeight(500);
        c.body->setHorseBodyParts(8);
        //Ideally, you should club them to death or chop them up with a sword.
        c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
        c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
        c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::UnicornHorn{}));
        c.courage = 100;
        //They heal up and summon friends.
        c.spells->add(SpellId::HEAL_SELF);
        c.spells->add(SpellId::HEAL_OTHER);
        c.spells->add(SpellId::SUMMON_SPIRIT);
        c.chatReactionFriendly = "\"mhhhhhrrrr!\""_s;
        c.chatReactionHostile = "\"mhhhhhrrrr!\""_s;
        c.petReaction = "neighs"_s;
        c.name = "unicorn";
        //Pet names like dogs would have.
        c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
        c.name->setGroup("herd");
        c.animal = true;
        );
    case CreatureId::BANDIT:
      return CATTR(
          c.viewId = ViewId::BANDIT;
          c.attr = LIST(15_dam, 13_def);
          c.body = Body::humanoid(Body::Size::LARGE);
          c.chatReactionFriendly = "curses all law enforcement"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.maxLevelIncrease[ExperienceType::MELEE] = 2;
 //       c.skills.insert(SkillId::DISARM_TRAPS);
          c.name = "bandit";);
    case CreatureId::GHOST: 
      return CATTR(
          c.viewId = ViewId::GHOST;
          c.attr = LIST(35_def, 30_spell_dam );
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.chatReactionFriendly = "\"Wouuuouuu!!!\""_s;
          c.chatReactionHostile = "\"Wouuuouuu!!!\""_s;
          c.name = "ghost";);
    case CreatureId::SPIRIT:
      return CATTR(
          c.viewId = ViewId::SPIRIT;
          c.attr = LIST(35_def, 30_spell_dam );
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.body->setIntrinsicAttack(BodyPart::TORSO, IntrinsicAttack(ItemType::spellHit(10)));
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.chatReactionFriendly = "\"Wouuuouuu!!!\""_s;
          c.chatReactionHostile = "\"Wouuuouuu!!!\""_s;
          c.name = "ancient spirit";);
    case CreatureId::LOST_SOUL:
      return CATTR(
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.body->setDeathSound(none);
          c.viewId = ViewId::GHOST;
          c.attr = LIST(25_def, 5_spell_dam );
          c.courage = 100;
          c.body->setIntrinsicAttack(BodyPart::TORSO, IntrinsicAttack(
              ItemType::touch(Effect(Effect::Lasting{LastingEffect::INSANITY}), Effect(Effect::Suicide{}))));
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.chatReactionFriendly = "\"Wouuuouuu!!!\""_s;
          c.chatReactionHostile = "\"Wouuuouuu!!!\""_s;
          c.name = "ghost";);
    case CreatureId::SUCCUBUS:
      return CATTR(
          c.attr = LIST(25_def, 5_spell_dam );
          c.viewId = ViewId::SUCCUBUS;
          c.body = Body::humanoidSpirit(Body::Size::LARGE);
          c.body->addWings();
          c.skills.insert(SkillId::COPULATION);
          c.body->getIntrinsicAttacks().clear();
          c.body->setIntrinsicAttack(BodyPart::ARM, IntrinsicAttack(
              ItemType::touch(Effect::Lasting{LastingEffect::PEACEFULNESS})));
          c.gender = Gender::female;
          c.courage = -1;
          c.name = CreatureName("succubus", "succubi");
          );
    case CreatureId::DOPPLEGANGER:
      return CATTR(
          c.viewId = ViewId::DOPPLEGANGER;
          c.attr = LIST(25_def, 5_spell_dam );
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.skills.insert(SkillId::CONSUMPTION);
          c.name = "doppelganger";
          );
    case CreatureId::WITCH: 
      return CATTR(
          c.viewId = ViewId::WITCH;
          c.attr = LIST(14_dam, 14_def, 20_spell_dam );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.name = CreatureName("witch", "witches");
          c.name->setFirst("Cornelia");
          c.gender = Gender::female;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.skills.setValue(SkillId::LABORATORY, 0.7);
          c.maxLevelIncrease[ExperienceType::SPELL] = 4;
          );
    case CreatureId::WITCHMAN: 
      return CATTR(
          c.viewId = ViewId::WITCHMAN;
          c.attr = LIST(30_dam, 30_def, 20_spell_dam );
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
          c.attr = LIST(34_dam, 40_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.name = CreatureName("cyclops", "cyclopes");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::CYCLOPS)->getNext());
          c.maxLevelIncrease[ExperienceType::MELEE] = 5;
          );
    case CreatureId::DEMON_DWELLER:
      return CATTR(
          c.viewId = ViewId::DEMON_DWELLER;
          c.attr = LIST(25_dam, 30_def, 35_spell_dam );
          c.body = Body::humanoidSpirit(Body::Size::LARGE);
          c.body->addWings();
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.courage = 100;
          c.gender = Gender::male;
          c.spells->add(SpellId::BLAST);
          c.chatReactionFriendly = "\"Kneel before us!\""_s;
          c.chatReactionHostile = "\"Face your death!\""_s;
          c.name = "demon dweller";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DEMON)->getNext());
          c.name->setGroup("pack");
          c.maxLevelIncrease[ExperienceType::MELEE] = 4;
          c.maxLevelIncrease[ExperienceType::SPELL] = 4;
        );
    case CreatureId::DEMON_LORD:
      return CATTR(
          c.viewId = ViewId::DEMON_LORD;
          c.attr = LIST(40_dam, 45_def, 50_spell_dam );
          c.body = Body::humanoidSpirit(Body::Size::LARGE);
          c.body->addWings();
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.courage = 100;
          c.gender = Gender::male;
          c.spells->add(SpellId::BLAST);
          c.chatReactionFriendly = "\"Kneel before us!\""_s;
          c.chatReactionHostile = "\"Face your death!\""_s;
          c.name = "Demon Lord";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DEMON)->getNext());
          c.name->setGroup("pack");
          c.maxLevelIncrease[ExperienceType::SPELL] = 7;
      );
    case CreatureId::MINOTAUR: 
      return CATTR(
          c.viewId = ViewId::MINOTAUR;
          c.attr = LIST(35_dam, 45_def );
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.body = Body::humanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.maxLevelIncrease[ExperienceType::MELEE] = 5;
          c.name = "minotaur";);
    case CreatureId::SOFT_MONSTER:
      return CATTR(
          c.viewId = ViewId::SOFT_MONSTER;
          c.attr = LIST(45_dam, 25_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.courage = -1;
          c.name = "soft monster";);
    case CreatureId::HYDRA:
      return CATTR(
          c.viewId = ViewId::HYDRA;
          c.attr = LIST(27_dam, 45_def );
          c.body = Body::nonHumanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(8, Effect::Lasting{LastingEffect::POISON})));
          c.skills.insert(SkillId::SWIMMING);
          c.name = "hydra";);
    case CreatureId::SHELOB:
      return CATTR(
          c.viewId = ViewId::SHELOB;
          c.attr = LIST(40_dam, 38_def );
          c.body = Body::nonHumanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.body->setBodyParts({{BodyPart::LEG, 8}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.body->setIntrinsicAttack(BodyPart::TORSO, IntrinsicAttack(
              ItemType::fangs(8, Effect::Lasting{LastingEffect::POISON})));
          c.skills.insert(SkillId::SPIDER);
          c.name = "giant spider";
          );
    case CreatureId::GREEN_DRAGON: 
      return CATTR(
          c.viewId = ViewId::GREEN_DRAGON;
          c.attr = LIST(40_dam, 40_def );
          c.body = Body::nonHumanoid(Body::Size::HUGE);
          c.body->setHorseBodyParts(7);
          c.body->addWings();
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(12, Effect::Lasting{LastingEffect::POISON})));
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
          c.attr = LIST(40_dam, 42_def );
          c.body = Body::nonHumanoid(Body::Size::HUGE);
          c.body->setHorseBodyParts(8);
          c.body->addWings();
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(15, Effect::Fire{})));
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
          c.attr = LIST(36_dam, 28_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.maxLevelIncrease[ExperienceType::MELEE] = 4;
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "knight";);
    case CreatureId::JESTER:
      return CATTR(
          c.viewId = ViewId::JESTER;
          c.attr = LIST(8_dam, 8_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "jester";);
    case CreatureId::AVATAR:
      return CATTR(
          c.viewId = ViewId::DUKE;
          c.attr = LIST(43_dam, 32_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.courage = 1;
          c.name = "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext(););
    case CreatureId::ARCHER:
      return CATTR(
          c.viewId = ViewId::ARCHER;
          c.attr = LIST(17_dam, 22_def, 30_ranged_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 4;
          c.name = "archer";);
    case CreatureId::PRIEST:
      return CATTR(
          c.viewId = ViewId::PRIEST;
          c.attr = LIST(15_dam, 15_def, 27_spell_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::SPEED_SELF);
          c.spells->add(SpellId::DEF_BONUS);
          c.spells->add(SpellId::BLAST);
          c.spells->add(SpellId::HEAL_OTHER);
          c.maxLevelIncrease[ExperienceType::SPELL] = 2;
          c.name = "priest";);
    case CreatureId::WARRIOR:
      return CATTR(
          c.viewId = ViewId::WARRIOR;
          c.attr = LIST(27_dam, 19_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.maxLevelIncrease[ExperienceType::MELEE] = 5;
          c.skills.setValue(SkillId::WORKSHOP, 0.3);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "warrior";);
    case CreatureId::SHAMAN:
      return CATTR(
          c.viewId = ViewId::SHAMAN;
          c.attr = LIST(27_dam, 19_def, 30_spell_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.courage = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.spells->add(SpellId::SPEED_SELF);
          c.spells->add(SpellId::DEF_BONUS);
          c.spells->add(SpellId::SUMMON_SPIRIT);
          c.spells->add(SpellId::BLAST);
          c.spells->add(SpellId::HEAL_OTHER);
          c.skills.setValue(SkillId::SORCERY, 1);
          c.maxLevelIncrease[ExperienceType::SPELL] = 5;
          c.name = "shaman";);
    case CreatureId::PESEANT: 
      return CATTR(
          if (Random.roll(2)) {
            c.viewId = ViewId::PESEANT_WOMAN;
            c.gender = Gender::female;
          } else
            c.viewId = ViewId::PESEANT;
          c.attr = LIST(14_dam, 12_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Heeelp!\""_s;
          c.skills.insert(SkillId::CROPS);
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.name = "peasant";);
    case CreatureId::CHILD: 
      return CATTR(
          c.viewId = ViewId::CHILD;
          c.attr = LIST(8_dam, 8_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "\"plaaaaay!\""_s;
          c.chatReactionHostile = "\"Heeelp!\""_s;
          c.skills.insert(SkillId::CROPS);
          c.skills.insert(SkillId::STEALTH);
          c.name = CreatureName("child", "children"););
    case CreatureId::SPIDER_FOOD: 
      return CATTR(
          c.viewId = ViewId::CHILD;
          c.attr = LIST(2_dam, 2_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.permanentEffects[LastingEffect::ENTANGLED] = 1;
          c.permanentEffects[LastingEffect::BLIND] = 1;
          c.chatReactionFriendly = "\"Put me out of my misery PLEASE!\""_s;
          c.chatReactionHostile = "\"End my torture!\""_s;
          c.deathDescription = "dead, released from unthinkable agony"_s;
          c.name = CreatureName("child", "children"););
    case CreatureId::PESEANT_PRISONER:
      return CATTR(
          if (Random.roll(2)) {
            c.viewId = ViewId::PESEANT_WOMAN;
            c.gender = Gender::female;
          } else
            c.viewId = ViewId::PESEANT;
          c.attr = LIST(14_dam, 12_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Heeelp!\""_s;
          c.skills.setValue(SkillId::DIGGING, 0.3);
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.name = "peasant";);
    case CreatureId::HALLOWEEN_KID:
      return CATTR(
          c.viewId = Random.choose(ViewId::HALLOWEEN_KID1,
              ViewId::HALLOWEEN_KID2, ViewId::HALLOWEEN_KID3,ViewId::HALLOWEEN_KID4);
          c.attr = LIST(8_dam, 8_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "\"Trick or treat!\""_s;
          c.chatReactionHostile = "\"Trick or treat!\""_s;
          c.name = CreatureName("child", "children"););
    case CreatureId::CLAY_GOLEM:
      return CATTR(
          c.viewId = ViewId::CLAY_GOLEM;
          c.attr = LIST(17_dam, 19_def );
          c.body = Body::nonHumanoid(Body::Material::CLAY, Body::Size::LARGE);
          c.body->setHumanoidBodyParts(2);
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.name = "clay golem";);
    case CreatureId::STONE_GOLEM: 
      return CATTR(
          c.viewId = ViewId::STONE_GOLEM;
          c.attr = LIST(19_dam, 23_def );
          c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::LARGE);
          c.body->setHumanoidBodyParts(4);
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.name = "stone golem";);
    case CreatureId::IRON_GOLEM: 
      return CATTR(
          c.viewId = ViewId::IRON_GOLEM;
          c.attr = LIST(23_dam, 30_def );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.body = Body::nonHumanoid(Body::Material::IRON, Body::Size::LARGE);
          c.body->setHumanoidBodyParts(7);
          c.name = "iron golem";);
    case CreatureId::LAVA_GOLEM: 
      return CATTR(
          c.viewId = ViewId::LAVA_GOLEM;
          c.attr = LIST(26_dam, 36_def );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.body = Body::nonHumanoid(Body::Material::LAVA, Body::Size::LARGE);
          c.body->setHumanoidBodyParts(8);
          c.body->setIntrinsicAttack(BodyPart::ARM, IntrinsicAttack(ItemType::fists(10, Effect::Fire{})));
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.name = "lava golem";);
    case CreatureId::ADA_GOLEM:
      return CATTR(
          c.viewId = ViewId::ADA_GOLEM;
          c.attr = LIST(36_dam, 36_def );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.body = Body::nonHumanoid(Body::Material::ADA, Body::Size::LARGE);
          c.body->setHumanoidBodyParts(8);
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.name = "adamantine golem";);
    case CreatureId::AUTOMATON:
      return CATTR(
          c.viewId = ViewId::AUTOMATON;
          c.attr = LIST(40_dam, 40_def );
          c.permanentEffects[LastingEffect::MELEE_RESISTANCE] = 1;
          c.body = Body::nonHumanoid(Body::Material::IRON, Body::Size::LARGE);
          c.body->setHumanoidBodyParts(10);
          c.name = "automaton";);
    case CreatureId::ZOMBIE: 
      return CATTR(
          c.viewId = ViewId::ZOMBIE;
          c.attr = LIST(14_dam, 17_def );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.name = "zombie";);
    case CreatureId::SKELETON: 
      return CATTR(
          c.viewId = ViewId::SKELETON;
          c.attr = LIST(17_dam, 13_def, 5_ranged_dam);
          c.body = Body::humanoid(Body::Material::BONE, Body::Size::LARGE);
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 4;
          c.name = "skeleton";);
    case CreatureId::VAMPIRE: 
      return CATTR(
          c.viewId = ViewId::VAMPIRE;
          c.attr = LIST(17_dam, 17_def, 17_spell_dam );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.chatReactionFriendly = "\"All men be cursed!\""_s;
          c.chatReactionHostile = "\"Die!\""_s;
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
          c.attr = LIST(17_dam, 23_def, 27_spell_dam );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
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
              SpellId::DAM_BONUS, SpellId::DECEPTION, SpellId::DECEPTION, SpellId::TELEPORT}))
            c.spells->add(id);
          c.chatReactionFriendly = c.chatReactionHostile =
              "\"There are times when you simply cannot refuse a drink!\""_s;
          );
    case CreatureId::MUMMY: 
      return CATTR(
          c.viewId = ViewId::MUMMY;
          c.attr = LIST(15_dam, 14_def, 10_spell_dam );
          c.body = Body::humanoid(Body::Material::UNDEAD_FLESH, Body::Size::LARGE);
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 3;
          c.maxLevelIncrease[ExperienceType::SPELL] = 3;
          c.name = CreatureName("mummy", "mummies"););
    case CreatureId::ORC:
      return CATTR(
          c.viewId = ViewId::ORC;
          c.attr = LIST(16_dam, 14_def );
          c.body = Body::humanoid(Body::Size::LARGE);
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
          c.attr = LIST(12_dam, 8_def, 16_spell_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
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
          c.attr = LIST(13_dam, 16_def, 15_ranged_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.body->addWings();
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
          c.attr = LIST(14_dam, 16_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.skills.insert(SkillId::SWIMMING);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "kobold";);
    case CreatureId::GNOME: 
      return CATTR(
          c.viewId = ViewId::GNOME;
          c.attr = LIST(12_dam, 13_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.skills.setValue(SkillId::JEWELER, 0.5);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "gnome";);
    case CreatureId::GNOME_CHIEF:
      return CATTR(
          c.viewId = ViewId::GNOME_BOSS;
          c.attr = LIST(15_dam, 16_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.skills.setValue(SkillId::JEWELER, 1);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "gnome chief";);
    case CreatureId::GOBLIN: 
      return CATTR(
          c.viewId = ViewId::GOBLIN;
          c.attr = LIST(12_dam, 13_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "talks about crafting"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.skills.insert(SkillId::DISARM_TRAPS);
          c.skills.setValue(SkillId::LABORATORY, 0.3);
          c.skills.setValue(SkillId::WORKSHOP, 0.6);
          c.skills.setValue(SkillId::FORGE, 0.6);
          c.skills.setValue(SkillId::JEWELER, 0.6);
          c.skills.setValue(SkillId::FURNACE, 0.6);
          c.name = "goblin";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          );
    case CreatureId::IMP: 
      return CATTR(
          c.viewId = ViewId::IMP;
          c.attr = LIST(5_dam, 15_def );
          c.body = Body::humanoidSpirit(Body::Size::SMALL);
          c.body->setNoCarryLimit();
          c.courage = -1;
          c.noChase = true;
          c.cantEquip = true;
          c.skills.setValue(SkillId::DIGGING, 0.4);
          c.chatReactionFriendly = "talks about digging"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::SPEED] = 1;
          c.name = "imp";
      );
    case CreatureId::OGRE: 
      return CATTR(
          c.viewId = ViewId::OGRE;
          c.attr = LIST(18_dam, 18_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.body->setWeight(140);
          c.name = "ogre";
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::ORC)->getNext());
          c.skills.setValue(SkillId::WORKSHOP, 0.5);
          c.skills.setValue(SkillId::FORGE, 0.5);
          c.skills.setValue(SkillId::FURNACE, 0.9);
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          );
    case CreatureId::CHICKEN: 
      return CATTR(
          c.viewId = ViewId::CHICKEN;
          c.attr = LIST(2_dam, 2_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(3);
          c.body->setMinionFood();
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.name = "chicken";);
    case CreatureId::DWARF: 
      return CATTR(
          c.viewId = ViewId::DWARF;
          c.attr = LIST(21_dam, 25_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.body->setWeight(90);
          c.name = CreatureName("dwarf", "dwarves");
          c.skills.insert(SkillId::NAVIGATION_DIGGING);
          c.skills.setValue(SkillId::FORGE, 0.8);
          c.skills.setValue(SkillId::FURNACE, 0.8);
          c.maxLevelIncrease[ExperienceType::MELEE] = 2;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DWARF)->getNext());
          c.chatReactionFriendly = "curses all orcs"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          );
    case CreatureId::DWARF_FEMALE:
      return CATTR(
          c.viewId = ViewId::DWARF_FEMALE;
          c.attr = LIST(21_dam, 25_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.body->setWeight(90);
          c.name = CreatureName("dwarf", "dwarves");
          c.skills.insert(SkillId::NAVIGATION_DIGGING);
          c.skills.setValue(SkillId::WORKSHOP, 0.5);
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DWARF)->getNext());
          c.chatReactionFriendly = "curses all orcs"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.gender = Gender::female;);
    case CreatureId::DWARF_BARON: 
      return CATTR(
          c.viewId = ViewId::DWARF_BARON;
          c.attr = LIST(28_dam, 32_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.body->setWeight(120);
          c.chatReactionFriendly = "curses all orcs"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.skills.insert(SkillId::NAVIGATION_DIGGING);
          c.maxLevelIncrease[ExperienceType::MELEE] = 10;
          c.permanentEffects[LastingEffect::MAGIC_VULNERABILITY] = 1;
          c.courage = 1;
          c.name = "dwarf baron";
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DWARF)->getNext());
          );
    case CreatureId::LIZARDMAN: 
      return CATTR(
          c.viewId = ViewId::LIZARDMAN;
          c.attr = LIST(20_dam, 14_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(5, Effect::Lasting{LastingEffect::POISON})));
          c.permanentEffects[LastingEffect::SPEED] = 1;
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 5;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = CreatureName("lizardman", "lizardmen"););
    case CreatureId::LIZARDLORD: 
      return CATTR(
          c.viewId = ViewId::LIZARDLORD;
          c.attr = LIST(30_dam, 16_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(8, Effect::Lasting{LastingEffect::POISON})));
          c.permanentEffects[LastingEffect::SPEED] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 10;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.courage = 1;
          c.name = "lizardman chief";);
    case CreatureId::ELF: 
      return CATTR(
          c.viewId = Random.choose(ViewId::ELF, ViewId::ELF_WOMAN);
          c.attr = LIST(14_dam, 6_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.skills.setValue(SkillId::JEWELER, 0.9);
          c.maxLevelIncrease[ExperienceType::SPELL] = 1;
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.name = CreatureName("elf", "elves"););
    case CreatureId::ELF_ARCHER: 
      return CATTR(
          c.viewId = ViewId::ELF_ARCHER;
          c.attr = LIST(18_dam, 12_def, 25_ranged_dam );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 3;
          c.name = "elven archer";);
    case CreatureId::ELF_CHILD: 
      return CATTR(
          c.viewId = ViewId::ELF_CHILD;
          c.attr = LIST(6_dam, 6_def );
          c.body = Body::humanoid(Body::Size::SMALL);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.skills.insert(SkillId::STEALTH);
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.name = CreatureName("elf child", "elf children"););
    case CreatureId::ELF_LORD: 
      return CATTR(
          c.viewId = ViewId::ELF_LORD;
          c.attr = LIST(22_dam, 14_def, 16_spell_dam, 30_ranged_dam );
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
          c.spells->add(SpellId::BLAST);
          c.maxLevelIncrease[ExperienceType::SPELL] = 4;
          c.maxLevelIncrease[ExperienceType::MELEE] = 4;
          c.name = "elf lord";);
    case CreatureId::DARK_ELF:
      return CATTR(
          c.viewId = Random.choose(ViewId::DARK_ELF, ViewId::DARK_ELF_WOMAN);
          c.attr = LIST(14_dam, 6_def );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.skills.insert(SkillId::SWIMMING);
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.name = CreatureName("dark elf", "dark elves"););
    case CreatureId::DARK_ELF_WARRIOR:
      return CATTR(
          c.viewId = ViewId::DARK_ELF_WARRIOR;
          c.attr = LIST(18_dam, 12_def, 6_spell_dam );
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
          c.attr = LIST(6_dam, 6_def );
          c.body = Body::humanoid(Body::Size::SMALL);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all dwarves"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.insert(SkillId::STEALTH);
          c.name = CreatureName("dark elf child", "dark elf children"););
    case CreatureId::DARK_ELF_LORD:
      return CATTR(
          c.viewId = ViewId::DARK_ELF_LORD;
          c.attr = LIST(22_dam, 14_def, 16_spell_dam );
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
          c.spells->add(SpellId::BLAST);
          c.name = "dark elf lord";);
    case CreatureId::DRIAD: 
      return CATTR(
          c.viewId = ViewId::DRIAD;
          c.attr = LIST(6_dam, 14_def, 25_ranged_dam );
          c.body = Body::humanoid(Body::Size::MEDIUM);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.chatReactionFriendly = "curses all humans"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.spells->add(SpellId::HEAL_SELF);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.maxLevelIncrease[ExperienceType::ARCHERY] = 4;
          c.name = "driad";);
    case CreatureId::HORSE: 
      return CATTR(
          c.viewId = ViewId::HORSE;
          c.attr = LIST(16_dam, 7_def );
          c.body = Body::nonHumanoid(Body::Size::LARGE);
          c.body->setWeight(500);
          c.body->setHorseBodyParts(2);
          c.animal = true;
          c.noChase = true;
          c.petReaction = "neighs"_s;
          c.name = "horse";);
    case CreatureId::COW: 
      return CATTR(
          c.viewId = ViewId::COW;
          c.attr = LIST(10_dam, 7_def );
          c.body = Body::nonHumanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.body->setHorseBodyParts(2);
          c.animal = true;
          c.noChase = true;
          c.petReaction = "\"Mooooooooooooooooooooooooooo!\""_s;
          c.name = "cow";);
    case CreatureId::DONKEY: 
      return CATTR(
          c.viewId = ViewId::DONKEY;
          c.attr = LIST(10_dam, 7_def );
          c.body = Body::nonHumanoid(Body::Size::LARGE);
          c.body->setWeight(200);
          c.body->setHorseBodyParts(2);
          c.body->setDeathSound(SoundId::DYING_DONKEY);
          c.animal = true;
          c.noChase = true;
          c.name = "donkey";);
    case CreatureId::PIG: 
      return CATTR(
          c.viewId = ViewId::PIG;
          c.attr = LIST(5_dam, 2_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(150);
          c.body->setHorseBodyParts(2);
          c.body->setMinionFood();
          c.body->setDeathSound(SoundId::DYING_PIG);
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.noChase = true;
          c.animal = true;
          c.name = "pig";);
    case CreatureId::GOAT:
      return CATTR(
          c.viewId = ViewId::GOAT;
          c.attr = LIST(10_dam, 7_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setHorseBodyParts(2);
          c.body->setMinionFood();
          c.petReaction = "\"Meh-eh-eh!\""_s;
          c.noChase = true;
          c.animal = true;
          c.name = "goat";);
    case CreatureId::JACKAL: 
      return CATTR(
          c.viewId = ViewId::JACKAL;
          c.attr = LIST(15_dam, 10_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(10);
          c.body->setHorseBodyParts(1);
          c.animal = true;
          c.name = "jackal";);
    case CreatureId::DEER: 
      return CATTR(
          c.viewId = ViewId::DEER;
          c.attr = LIST(10_dam, 10_def );
          c.body = Body::nonHumanoid(Body::Size::LARGE);
          c.body->setWeight(400);
          c.body->setHorseBodyParts(2);
          c.permanentEffects[LastingEffect::SPEED] = 1;
          c.animal = true;
          c.noChase = true;
          c.name = CreatureName("deer", "deer"););
    case CreatureId::BOAR: 
      return CATTR(
          c.viewId = ViewId::BOAR;
          c.attr = LIST(10_dam, 10_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(200);
          c.body->setHorseBodyParts(5);
          c.animal = true;
          c.noChase = true;
          c.name = "boar";);
    case CreatureId::FOX: 
      return CATTR(
          c.viewId = ViewId::FOX;
          c.attr = LIST(10_dam, 5_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(10);
          c.body->setHorseBodyParts(1);
          c.animal = true;
          c.noChase = true;
          c.name = CreatureName("fox", "foxes"););
    case CreatureId::CAVE_BEAR:
      return CATTR(
          c.viewId = ViewId::BEAR;
          c.attr = LIST(20_dam, 18_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(250);
          c.body->setHorseBodyParts(8);
          c.body->setIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::claws(10)));
          c.animal = true;
          c.skills.insert(SkillId::EXPLORE_CAVES);
          c.name = "cave bear";);
    case CreatureId::RAT: 
      return CATTR(
          c.viewId = ViewId::RAT;
          c.attr = LIST(2_dam, 2_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(1);
          c.body->setHorseBodyParts(1);
          c.permanentEffects[LastingEffect::SPEED] = 1;
          c.animal = true;
          c.noChase = true;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "rat";);
    case CreatureId::SPIDER: 
      return CATTR(
          c.viewId = ViewId::SPIDER;
          c.attr = LIST(9_dam, 13_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(0.3);
          c.body->setBodyParts({{BodyPart::LEG, 8}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.body->setIntrinsicAttack(BodyPart::TORSO, IntrinsicAttack(
              ItemType::fangs(1, Effect::Lasting{LastingEffect::POISON})));
          c.animal = true;
          c.name = "spider";);
    case CreatureId::FLY: 
      return CATTR(
          c.viewId = ViewId::FLY;
          c.attr = LIST(2_dam, 12_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(0.1);
          c.body->setBodyParts({{BodyPart::LEG, 6}, {BodyPart::WING, 2}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.courage = 1;
          c.noChase = true;
          c.animal = true;
          c.name = CreatureName("fly", "flies"););
    case CreatureId::ANT_WORKER:
      return CATTR(
          c.viewId = ViewId::ANT_WORKER;
          c.attr = LIST(16_dam, 16_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(10);
          c.body->setBodyParts({{BodyPart::LEG, 6}, {BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.animal = true;
          c.name = "giant ant";);
    case CreatureId::ANT_SOLDIER:
      return CATTR(
          c.viewId = ViewId::ANT_SOLDIER;
          c.attr = LIST(30_dam, 20_def );
          c.skills.insert(SkillId::NAVIGATION_DIGGING);
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(10);
          c.body->setBodyParts({{BodyPart::LEG, 6}, {BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(6, Effect::Lasting{LastingEffect::POISON})));
          c.animal = true;
          c.name = "giant ant soldier";);
    case CreatureId::ANT_QUEEN:      
      return CATTR(
          c.viewId = ViewId::ANT_QUEEN;
          c.attr = LIST(30_dam, 26_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(10);
          c.body->setBodyParts({{BodyPart::LEG, 6}, {BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(12, Effect::Lasting{LastingEffect::POISON})));
          c.animal = true;
          c.name = "ant queen";);
    case CreatureId::SNAKE: 
      return CATTR(
          c.viewId = ViewId::SNAKE;
          c.attr = LIST(14_dam, 14_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(2);
          c.body->setBodyParts({{BodyPart::HEAD, 1}, {BodyPart::TORSO, 1}});
          c.body->setDeathSound(none);
          c.animal = true;
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(
              ItemType::fangs(1, Effect::Lasting{LastingEffect::POISON})));
          c.skills.insert(SkillId::SWIMMING);
          c.name = "snake";);
    case CreatureId::RAVEN: 
      return CATTR(
          c.viewId = ViewId::RAVEN;
          c.attr = LIST(2_dam, 12_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(0.5);
          c.body->setBirdBodyParts(1);
          c.body->setDeathSound(none);
          c.animal = true;
          c.noChase = true;
          c.courage = 1;
          c.permanentEffects[LastingEffect::SPEED] = 1;
          c.skills.insert(SkillId::EXPLORE);
          c.name = "raven";
          c.name->setGroup("flock");
          );
    case CreatureId::VULTURE: 
      return CATTR(
          c.viewId = ViewId::VULTURE;
          c.attr = LIST(2_dam, 12_def );
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(5);
          c.body->setBirdBodyParts(1);
          c.body->setDeathSound(none);
          c.animal = true;
          c.noChase = true;
          c.courage = 1;
          c.name = "vulture";);
    case CreatureId::WOLF: 
      return CATTR(
          c.viewId = ViewId::WOLF;
          c.attr = LIST(18_dam, 11_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(35);
          c.body->setHorseBodyParts(7);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(8)));
          c.body->setIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::claws(7)));
          c.animal = true;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.name = CreatureName("wolf", "wolves");
          c.name->setGroup("pack");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
          c.skills.insert(SkillId::EXPLORE_NOCTURNAL);
          );    
    case CreatureId::WEREWOLF:
      return CATTR(
          c.viewId = ViewId::WEREWOLF;
          c.attr = LIST(20_dam, 7_def );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(8)));
          c.body->setIntrinsicAttack(BodyPart::LEG, IntrinsicAttack(ItemType::claws(7)));
          c.animal = true;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.insert(SkillId::STEALTH);
          c.skills.insert(SkillId::EXPLORE_NOCTURNAL);
          c.permanentEffects[LastingEffect::MAGIC_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::REGENERATION] = 1;
          c.maxLevelIncrease[ExperienceType::MELEE] = 12;
          c.name = CreatureName("werewolf", "werewolves");
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(8)));
          );
    case CreatureId::DOG: 
      return CATTR(
          c.viewId = ViewId::DOG;
          c.attr = LIST(18_dam, 7_def );
          c.body = Body::nonHumanoid(Body::Size::MEDIUM);
          c.body->setWeight(25);
          c.body->setHorseBodyParts(2);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(4)));
          c.animal = true;
          c.name = "dog";
          c.name->setGroup("pack");
          c.petReaction = "\"WOOF!\""_s;
          c.name->setFirst(NameGenerator::get(NameGeneratorId::DOG)->getNext());
          );
    case CreatureId::FIRE_SPHERE: 
      return CATTR(
          c.viewId = ViewId::FIRE_SPHERE;
          c.attr = LIST(5_dam, 15_def );
          c.body = Body::nonHumanoid(Body::Material::FIRE, Body::Size::SMALL);
          c.body->setDeathSound(none);
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.name = "fire sphere";);
    case CreatureId::ELEMENTALIST: 
      return CATTR(
          c.viewId = ViewId::ELEMENTALIST;
          c.attr = LIST(15_dam, 20_def, 15_spell_dam );
          c.body = Body::humanoid(Body::Size::LARGE);
          c.skills.setValue(SkillId::LABORATORY, 1);
          c.maxLevelIncrease[ExperienceType::SPELL] = 9;
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
          c.body = Body::nonHumanoid(Body::Material::FIRE, Body::Size::LARGE);
          c.body->setDeathSound(none);
          c.attr = LIST(20_dam, 30_def);
          c.body->setIntrinsicAttack(BodyPart::TORSO, IntrinsicAttack(ItemType::fists(5, Effect::Fire{})));
          c.permanentEffects[LastingEffect::FIRE_RESISTANT] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.name = "fire elemental";);
    case CreatureId::AIR_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::AIR_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::SPIRIT, Body::Size::LARGE);
          c.body->setDeathSound(none);
          c.attr = LIST(25_dam, 35_def );
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.spells->add(SpellId::CIRCULAR_BLAST);
          c.name = "air elemental";);
    case CreatureId::EARTH_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::EARTH_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::ROCK, Body::Size::LARGE);
          c.body->setWeight(500);
          c.body->setHumanoidBodyParts(5);
          c.body->setDeathSound(none);
          c.attr = LIST(30_dam, 25_def );
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.name = "earth elemental";);
    case CreatureId::WATER_ELEMENTAL:
      return CATTR(
          c.viewId = ViewId::WATER_ELEMENTAL;
          c.body = Body::nonHumanoid(Body::Material::WATER, Body::Size::LARGE);
          c.body->setWeight(300);
          c.body->setHumanoidBodyParts(5);
          c.body->setDeathSound(none);
          c.attr = LIST(40_dam, 15_def );
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.skills.insert(SkillId::SWIMMING);
          c.name = "water elemental";);
    case CreatureId::ENT:
      return CATTR(
          c.viewId = ViewId::ENT;
          c.body = Body::nonHumanoid(Body::Material::WOOD, Body::Size::HUGE);
          c.body->setHumanoidBodyParts(10);
          c.attr = LIST(35_dam, 25_def);
          c.permanentEffects[LastingEffect::ELF_VISION] = 1;
          c.permanentEffects[LastingEffect::RANGED_RESISTANCE] = 1;
          c.permanentEffects[LastingEffect::SLOWED] = 1;
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "tree spirit";);
    case CreatureId::ANGEL:
      return CATTR(
          c.viewId = ViewId::ANGEL;
          c.attr = LIST(22_def, 20_spell_dam );
          c.body = Body::nonHumanoid(Body::Material::SPIRIT, Body::Size::LARGE);
          c.chatReactionFriendly = "curses all dungeons"_s;
          c.chatReactionHostile = "\"Die!\""_s;
          c.name = "angel";);
    case CreatureId::KRAKEN:
      return getKrakenAttributes(ViewId::KRAKEN_HEAD, "kraken");
    case CreatureId::BAT: 
      return CATTR(
          c.viewId = ViewId::BAT;
          c.body = Body::nonHumanoid(Body::Size::SMALL);
          c.body->setWeight(1);
          c.body->setBirdBodyParts(1);
          c.attr = LIST(3_dam, 16_def);
          c.body->setIntrinsicAttack(BodyPart::HEAD, IntrinsicAttack(ItemType::fangs(3)));
          c.animal = true;
          c.noChase = true;
          c.courage = 1;
          c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
          c.skills.insert(SkillId::EXPLORE_NOCTURNAL);
          c.skills.insert(SkillId::EXPLORE_CAVES);
          c.name = "bat";);
    case CreatureId::DEATH: 
      return CATTR(
          c.viewId = ViewId::DEATH;
          c.attr = LIST(100_spell_dam, 35_def);
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
  return ItemType::Potion{Effect::Heal{}};
}

ItemType randomBackup() {
  return Random.choose(
      ItemType(ItemType::Scroll{Effect::Teleport{}}),
      randomHealing());
}

ItemType randomArmor() {
  return Random.choose({ItemType(ItemType::LeatherArmor{}), ItemType(ItemType::ChainArmor{})}, {4, 1});
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

static vector<ItemType> getDefaultInventory(CreatureId id) {
  switch (id) {
    case CreatureId::KEEPER_MAGE_F:
    case CreatureId::KEEPER_MAGE:
      return ItemList()
        .add(ItemType(ItemType::Robe{}).setPrefixChance(1));
    case CreatureId::KEEPER_KNIGHT_F:
    case CreatureId::KEEPER_KNIGHT:
      return ItemList()
        .add(ItemType::LeatherArmor{})
        .add(ItemType::LeatherHelm{})
        .add(ItemType::Sword{});
    case CreatureId::CYCLOPS:
      return ItemList()
        .add(ItemType::HeavyClub{})
        .add(ItemType::GoldPiece{}, Random.get(40, 80));
    case CreatureId::GREEN_DRAGON:
      return ItemList().add(ItemType::GoldPiece{}, Random.get(60, 100));
    case CreatureId::DEMON_DWELLER:
      return ItemList().add(ItemType::GoldPiece{}, Random.get(5, 10));
    case CreatureId::RED_DRAGON:
      return ItemList().add(ItemType::GoldPiece{}, Random.get(120, 200));
    case CreatureId::DEMON_LORD:
    case CreatureId::ANGEL:
      return ItemList().add(ItemType(ItemType::Sword{}).setPrefixChance(1));
    case CreatureId::ADVENTURER_F:
    case CreatureId::ADVENTURER:
      return ItemList()
        .add(ItemType::FirstAidKit{}, 3)
        .add(ItemType::Knife{})
        .add(ItemType::Sword{})
        .add(ItemType::LeatherGloves{})
        .add(ItemType::LeatherArmor{})
        .add(ItemType::LeatherHelm{})
        .add(ItemType::GoldPiece{}, Random.get(16, 26));
    case CreatureId::ELEMENTALIST:
      return ItemList()
          .add(ItemType::IronStaff{})
          .add(ItemType::Torch{});
    case CreatureId::DEATH:
      return ItemList()
        .add(ItemType::Scythe{});
    case CreatureId::KOBOLD:
      return ItemList()
        .add(ItemType::Spear{});
    case CreatureId::GOBLIN:
      return ItemList()
        .add(ItemType::Club{})
        .maybe(0.3, ItemType::LeatherBoots{});
    case CreatureId::WARRIOR: 
      return ItemList()
        .add(ItemType::LeatherArmor{})
        .add(ItemType::Club{})
        .add(ItemType::GoldPiece{}, Random.get(2, 5));
    case CreatureId::SHAMAN: 
      return ItemList()
        .add(ItemType::LeatherArmor{})
        .add(ItemType::Club{})
        .add(ItemType::GoldPiece{}, Random.get(80, 120));
    case CreatureId::LIZARDLORD:
      return ItemList().add(ItemType::LeatherArmor{})
        .add(ItemType::Potion{Effect::RegrowBodyPart{}})
        .add(ItemType::GoldPiece{}, Random.get(50, 90));
    case CreatureId::LIZARDMAN:
      return ItemList().add(ItemType::LeatherArmor{})
        .add(ItemType::GoldPiece{}, Random.get(2, 4));
    case CreatureId::HARPY: 
      return ItemList()
        .add(ItemType::Bow{});
    case CreatureId::ARCHER: 
      return ItemList()
        .add(ItemType::Bow{})
        .add(ItemType::Knife{})
        .add(ItemType::LeatherArmor{})
        .add(ItemType::LeatherBoots{})
        .maybe(0.3, ItemType::Torch{})
        .add(randomHealing())
        .add(ItemType::GoldPiece{}, Random.get(4, 10));
    case CreatureId::WITCHMAN:
      return ItemList()
        .add(ItemType::Sword{})
        .add(ItemType::LeatherArmor{})
        .add(ItemType::LeatherBoots{})
        .add(randomHealing())
        .add(ItemType::Potion{Effect::Lasting{LastingEffect::SPEED}}, 4)
        .add(ItemType::GoldPiece{}, Random.get(60, 80));
    case CreatureId::PRIEST:
      return ItemList()
        .add(ItemType::IronStaff{})
        .add(ItemType::LeatherBoots{})
        .add(ItemType(ItemType::Robe{}).setPrefixChance(1));
    case CreatureId::KNIGHT:
      return ItemList()
        .add(ItemType::Sword{})
        .add(ItemType::ChainArmor{})
        .add(ItemType::LeatherBoots{})
        .maybe(0.3, ItemType::Torch{})
        .add(randomHealing())
        .add(ItemType::GoldPiece{}, Random.get(6, 16));
    case CreatureId::MINOTAUR: 
      return ItemList()
        .add(ItemType::BattleAxe{});
    case CreatureId::AVATAR: 
      return ItemList()
        .add(ItemType(ItemType::BattleAxe{}).setPrefixChance(1))
        .add(ItemType::ChainArmor{})
        .add(ItemType::IronHelm{})
        .add(ItemType::IronBoots{})
        .add(randomHealing(), 3)
        .maybe(0.3, ItemType::Torch{})
        .add(ItemType::GoldPiece{}, Random.get(140, 200));
    case CreatureId::OGRE: 
      return ItemList().add(ItemType::HeavyClub{});
    case CreatureId::BANDIT:
      return ItemList()
        .add(ItemType::Sword{})
        .maybe(0.3, randomBackup())
        .maybe(0.3, ItemType::Torch{})
        .maybe(0.05, ItemType::Bow{});
    case CreatureId::DWARF:
      return ItemList()
        .add(Random.choose({ItemType(ItemType::BattleAxe{}), ItemType(ItemType::WarHammer{})}, {1, 1}))
        .maybe(0.6, randomBackup())
        .add(ItemType::ChainArmor{})
        .maybe(0.5, ItemType::IronHelm{})
        .maybe(0.3, ItemType::IronBoots{})
        .maybe(0.3, ItemType::Torch{})
        .add(ItemType::GoldPiece{}, Random.get(2, 6));
    case CreatureId::DWARF_BARON: 
      return ItemList()
        .add(Random.choose(
            ItemType(ItemType::BattleAxe{}).setPrefixChance(1),
            ItemType(ItemType::WarHammer{}).setPrefixChance(1)))
        .add(randomBackup())
        .add(randomHealing())
        .add(ItemType::ChainArmor{})
        .add(ItemType::IronBoots{})
        .add(ItemType::IronHelm{})
        .maybe(0.3, ItemType::Torch{})
        .add(ItemType::GoldPiece{}, Random.get(80, 120));
    case CreatureId::GNOME_CHIEF:
      return ItemList()
        .add(ItemType::Sword{})
        .add(randomBackup());
    case CreatureId::VAMPIRE_LORD:
      return ItemList()
        .add(ItemType(ItemType::Robe{}).setPrefixChance(0.3))
        .add(ItemType(ItemType::IronStaff{}).setPrefixChance(0.3));
    case CreatureId::DARK_ELF_LORD: 
    case CreatureId::ELF_LORD: 
      return ItemList()
        .add(ItemType(ItemType::ElvenSword{}).setPrefixChance(1))
        .add(ItemType::LeatherArmor{})
        .add(ItemType::ElvenBow{})
        .add(ItemType::GoldPiece{}, Random.get(80, 120))
        .add(randomBackup());
    case CreatureId::DRIAD: 
      return ItemList()
        .add(ItemType::Bow{});
    case CreatureId::DARK_ELF_WARRIOR: 
      return ItemList()
        .add(ItemType::ElvenSword{})
        .add(ItemType::LeatherArmor{})
        .add(ItemType::GoldPiece{}, Random.get(2, 6))
        .add(randomBackup());
    case CreatureId::ELF_ARCHER: 
      return ItemList()
        .add(ItemType::ElvenSword{})
        .add(ItemType::LeatherArmor{})
        .add(ItemType::Bow{})
        .add(ItemType::GoldPiece{}, Random.get(2, 6))
        .add(randomBackup());
    case CreatureId::WITCH:
      return ItemList()
        .add(ItemType::Knife{})
        .add({
            ItemType::Potion{Effect::Heal{}},
            ItemType::Potion{Effect::Lasting{LastingEffect::SLEEP}},
            ItemType::Potion{Effect::Lasting{LastingEffect::SLOWED}},
            ItemType::Potion{Effect::Lasting{LastingEffect::BLIND}},
            ItemType::Potion{Effect::Lasting{LastingEffect::INVISIBLE}},
            ItemType::Potion{Effect::Lasting{LastingEffect::POISON}},
            ItemType::Potion{Effect::Lasting{LastingEffect::SPEED}}});
    case CreatureId::HALLOWEEN_KID:
      return ItemList()
        .add(ItemType::BagOfCandies{})
        .add(ItemType::HalloweenCostume{});
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

