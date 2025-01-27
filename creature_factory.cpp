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
#include "creature_debt.h"
#include "effect.h"
#include "game_event.h"
#include "game_config.h"
#include "creature_inventory.h"
#include "effect_type.h"
#include "item_types.h"
#include "content_factory.h"
#include "automaton_part.h"

SERIALIZE_DEF(CreatureFactory, nameGenerator, attributes, spellSchools, spells)
SERIALIZATION_CONSTRUCTOR_IMPL(CreatureFactory)

class BoulderController : public Monster {
  public:
  BoulderController(Creature* c, Vec2 dir) : Monster(c, MonsterAIFactory::idle()), direction(dir) {
    CHECK(direction.length4() == 1);
  }

  virtual void makeMove() override {
    Position nextPos = creature->getPosition().plus(direction);
    if (Creature* c = nextPos.getCreature()) {
      if (!c->getBody().isKilledByBoulder(creature->getGame()->getContentFactory())) {
        if (nextPos.canEnterEmpty(creature)) {
          creature->swapPosition(direction);
          return;
        }
      } else {
        health -= c->getBody().getBoulderDamage();
        if (health <= 0) {
          nextPos.globalMessage(TSentence("OBJECT_CRASHES_ON", creature->getName().the(), c->getName().the()));
          nextPos.unseenMessage("You hear a crash");
          creature->dieNoReason();
          //c->takeDamage(Attack(creature, AttackLevel::MIDDLE, AttackType::HIT, 1000, AttrType("DAMAGE")));
          return;
        } else {
          c->you(MsgType::KILLED_BY, creature->getName().the());
          c->dieWithAttacker(creature);
        }
      }
    }
    if (auto furniture = nextPos.getFurniture(FurnitureLayer::MIDDLE))
      if (furniture->canDestroy(nextPos, creature->getMovementType(), DestroyAction::Type::BOULDER) &&
          *furniture->getStrength(DestroyAction::Type::BOULDER) <
          health * creature->getAttr(AttrType("DAMAGE"))) {
        health -= *furniture->getStrength(DestroyAction::Type::BOULDER) /
            (double) creature->getAttr(AttrType("DAMAGE"));
        creature->destroyImpl(direction, DestroyAction::Type::BOULDER);
      }
    if (auto action = creature->move(direction))
      action.perform(creature);
    else {
      nextPos.globalMessage(TSentence("OBJECT_CRASHES_ON", creature->getName().the(), nextPos.getName()));
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

PCreature CreatureFactory::getRollingBoulder(Vec2 direction) {
  auto tribe = TribeId::getHostile();
  ViewObject viewObject(ViewId("boulder"), ViewLayer::CREATURE);
  viewObject.setModifier(ViewObjectModifier::NO_UP_MOVEMENT);
  auto ret = makeOwner<Creature>(viewObject, tribe, CATTR(
            c.viewId = ViewId("boulder");
            c.attr[AttrType("DAMAGE")] = 250;
            c.attr[AttrType("DEFENSE")] = 250;
            c.body = Body::nonHumanoid(BodyMaterialId("ROCK"), Body::Size::HUGE);
            c.body->setDeathSound(none);
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.boulder = true;
            c.name = TString(TStringId("BOULDER"));
            ), SpellMap{});
  ret->setController(makeOwner<BoulderController>(ret.get(), direction));
  return ret;
}

PCreature CreatureFactory::getAnimatedItem(const ContentFactory* factory, PItem item, TribeId tribe, int attrBonus) {
  auto ret = makeOwner<Creature>(tribe, CATTR(
            c.viewId = item->getViewObject().id();
            c.attr[AttrType("DEFENSE")] = item->getModifier(AttrType("DEFENSE")) + attrBonus;
            for (auto& attr : factory->attrInfo)
              if (attr.second.isAttackAttr && item->getModifier(attr.first) > 0)
                c.attr[attr.first] = item->getModifier(attr.first) + attrBonus;
            c.body = Body::nonHumanoid(BodyMaterialId("SPIRIT"), Body::Size::SMALL);
            c.body->setDeathSound(none);
            c.name = TString(TSentence("ANIMATED_OBJECT", item->getName()));
            c.gender = Gender::IT;
            auto weaponInfo = item->getWeaponInfo();
            weaponInfo.itselfMessage = true;
            c.body->addIntrinsicAttack(BodyPart::TORSO, IntrinsicAttack(ItemType(
                ItemTypes::Intrinsic{item->getViewObject().id(), item->getName(), 0, std::move(weaponInfo)})));
            c.permanentEffects[LastingEffect::FLYING] = 1;
            ), SpellMap{});
  ret->setController(Monster::getFactory(MonsterAIFactory::monster()).get(ret.get()));
  ret->take(std::move(item), factory);
  initializeAttributes(none, ret->getAttributes());
  return ret;
}

class SokobanController : public Monster {
  public:
  SokobanController(Creature* c) : Monster(c, MonsterAIFactory::idle()) {}

  virtual MessageGenerator& getMessageGenerator() const override {
    static MessageGenerator g(MessageGenerator::BOULDER);
    return g;
  }

  SERIALIZE_ALL(SUBCLASS(Monster))
  SERIALIZATION_CONSTRUCTOR(SokobanController)

  private:
};

PCreature CreatureFactory::getSokobanBoulder(TribeId tribe) {
  ViewObject viewObject(ViewId("boulder"), ViewLayer::CREATURE);
  viewObject.setModifier(ViewObjectModifier::NO_UP_MOVEMENT).setModifier(ViewObjectModifier::REMEMBER);
  auto ret = makeOwner<Creature>(viewObject, tribe, CATTR(
            c.viewId = ViewId("boulder");
            c.attr[AttrType("DAMAGE")] = 250;
            c.attr[AttrType("DEFENSE")] = 250;
            c.body = Body::nonHumanoid(BodyMaterialId("ROCK"), Body::Size::HUGE);
            c.body->setDeathSound(none);
            c.body->setMinPushSize(Body::Size::LARGE);
            c.permanentEffects[LastingEffect::BLIND] = 1;
            c.boulder = true;
            c.name = TString(TStringId("BOULDER"));
  ), SpellMap{});
  ret->setController(makeOwner<SokobanController>(ret.get()));
  return ret;
}

CreatureAttributes CreatureFactory::getKrakenAttributes(ViewId id, const TString& name) {
  return CATTR(
      c.viewId = id;
      c.body = Body::nonHumanoid(Body::Size::LARGE);
      c.body->setDeathSound(none);
      c.body->setCanBeCaptured(false);
      c.attr[AttrType("DAMAGE")] = 28;
      c.attr[AttrType("DEFENSE")] = 28;
      c.permanentEffects[LastingEffect::POISON_RESISTANT] = 1;
      c.permanentEffects[LastingEffect::NIGHT_VISION] = 1;
      c.permanentBuffs.push_back(BuffId("SWIMMING_SKILL"));
      c.name = name;);
}

static string getSpeciesName(bool humanoid, bool large, bool living, bool wings) {
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
  return names[humanoid * 8 + (!large) * 4 + (!living) * 2 + wings];
}

static ViewId getSpecialViewId(bool humanoid, bool large, bool body, bool wings) {
  static vector<ViewId> specialViewIds {
    ViewId("special_blbn"),
    ViewId("special_blbw"),
    ViewId("special_blgn"),
    ViewId("special_blgw"),
    ViewId("special_bmbn"),
    ViewId("special_bmbw"),
    ViewId("special_bmgn"),
    ViewId("special_bmgw"),
    ViewId("special_hlbn"),
    ViewId("special_hlbw"),
    ViewId("special_hlgn"),
    ViewId("special_hlgw"),
    ViewId("special_hmbn"),
    ViewId("special_hmbw"),
    ViewId("special_hmgn"),
    ViewId("special_hmgw"),
  };
  return specialViewIds[humanoid * 8 + (!large) * 4 + (!body) * 2 + wings];
}

ViewIdList CreatureFactory::getViewId(CreatureId id) const {
  if (auto a = getReferenceMaybe(attributes, id))
    return {a->viewId};
  if (auto p = getReferenceMaybe(getSpecialParams(), id))
    return {getSpecialViewId(p->humanoid, p->large, p->living, p->wings)};
  return {ViewId("knight")};
}

TString CreatureFactory::getName(CreatureId id) const {
  if (auto a = getReferenceMaybe(attributes, id))
    return a->name.bare();
  if (auto p = getReferenceMaybe(getSpecialParams(), id))
    return getSpeciesName(p->humanoid, p->large, p->living, p->wings);
  return TStringId("KNIGHT_NAME");
}

TString CreatureFactory::getNamePlural(CreatureId id) const {
  return attributes.at(id).name.plural();
}

vector<CreatureId> CreatureFactory::getAllCreatures() const {
  return getKeys(attributes);
}

NameGenerator* CreatureFactory::getNameGenerator() {
  return &*nameGenerator;
}

const map<SpellSchoolId, SpellSchool>& CreatureFactory::getSpellSchools() const {
  return spellSchools;
}

const vector<Spell>& CreatureFactory::getSpells() const {
  return spells;
}

CreatureFactory::CreatureFactory(NameGenerator n, map<CreatureId, CreatureAttributes> a,
    map<SpellSchoolId, SpellSchool> s, vector<Spell> sp)
  : nameGenerator(std::move(n)), attributes(a), spellSchools(s), spells(sp) {}

CreatureFactory::~CreatureFactory() {
}

void CreatureFactory::merge(CreatureFactory f) {
  mergeMap(std::move(f.attributes), attributes);
  mergeMap(std::move(f.spellSchools), spellSchools);
  append(spells, std::move(f.spells));
  nameGenerator->merge(std::move(*f.nameGenerator));
}

void CreatureFactory::setContentFactory(const ContentFactory* f) const {
  contentFactory = f;
}

CreatureFactory::CreatureFactory(CreatureFactory&&) noexcept = default;
CreatureFactory& CreatureFactory::operator =(CreatureFactory&&) = default;

constexpr int maxKrakenLength = 15;

class KrakenController : public Monster {
  public:
  KrakenController(Creature* c) : Monster(c, MonsterAIFactory::monster()) {
  }

  KrakenController(Creature* c, WeakPointer<KrakenController> father, int length)
      : Monster(c, MonsterAIFactory::monster()), length(length), father(father) {
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

  virtual void onKilled(Creature* attacker) override {
    if (attacker) {
      if (father)
        attacker->secondPerson("You cut the kraken's tentacle");
      else
        attacker->secondPerson("You kill the kraken!");
    }
    for (Creature* c : spawns)
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

  void pullEnemy(Creature* held) {
    if (Random.roll(3)) {
      held->verb(TStringId("ENEMY_PULLS_YOU"), TStringId("ENEMY_PULLS_CREATURE"), creature->getName().the());
      if (father) {
        held->setHeld(father->creature);
        Vec2 pullDir = held->getPosition().getDir(creature->getPosition());
        creature->dieNoReason(Creature::DropType::NOTHING);
        held->displace(pullDir);
      } else {
        held->verb(TStringId("YOU_ARE_EATEN_BY"), TStringId("IS_EATEN_BY"), creature->getName().the());
        held->dieNoReason();
      }
    }
  }

  Creature* getHeld() {
    for (auto pos : creature->getPosition().neighbors8())
      if (auto other = pos.getCreature())
        if (other->getHoldingCreature() == creature)
          return other;
    return nullptr;
  }

  Creature* getVisibleEnemy() {
    const int radius = 10;
    Creature* ret = nullptr;
    auto myPos = creature->getPosition();
    for (Position pos : creature->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), radius)))
      if (Creature* c = pos.getCreature())
        if (c->getAttributes().getCreatureId() != creature->getAttributes().getCreatureId() &&
            (!ret || *ret->getPosition().dist8(myPos) > *c->getPosition().dist8(myPos)) &&
            creature->canSee(c) && creature->isEnemy(c) && !c->getHoldingCreature())
          ret = c;
    return ret;
  }

  void considerAttacking(Creature* c) {
    auto pos = c->getPosition();
    Vec2 v = creature->getPosition().getDir(pos);
    if (v.length8() == 1) {
      c->verb(TStringId("ENEMY_SWINGS_ITSELF_AROUND_YOU"), TStringId("ENEMY_SWINGS_ITSELF_AROUND_CREATURE"),
          creature->getName().the());
      c->setHeld(creature);
    } else if (length < maxKrakenLength && Random.roll(2)) {
      pair<Vec2, Vec2> dirs = v.approxL1();
      vector<Vec2> moves;
      if (creature->getPosition().plus(dirs.first).canEnter(
            {{MovementTrait::WALK, MovementTrait::SWIM}}))
        moves.push_back(dirs.first);
      if (creature->getPosition().plus(dirs.second).canEnter(
            {{MovementTrait::WALK, MovementTrait::SWIM}}))
        moves.push_back(dirs.second);
      if (!moves.empty()) {
        Vec2 move = Random.choose(moves);
        ViewId viewId = creature->getPosition().plus(move).canEnter({MovementTrait::SWIM})
          ? ViewId("kraken_water") : ViewId("kraken_land");
        auto spawn = makeOwner<Creature>(creature->getTribeId(),
            CreatureFactory::getKrakenAttributes(viewId, TStringId("KRAKEN_TENTACLE")), SpellMap{});
        spawn->setController(makeOwner<KrakenController>(spawn.get(), getThis().dynamicCast<KrakenController>(),
            length + 1));
        spawns.push_back(spawn.get());
        creature->getPosition().plus(move).addCreature(std::move(spawn));
      }
    }
  }

  virtual void makeMove() override {
    for (Creature* c : spawns)
      if (c->isDead()) {
        spawns.removeElement(c);
        break;
      }
    if (spawns.empty()) {
      if (auto held = getHeld()) {
        pullEnemy(held);
      } else if (auto c = getVisibleEnemy()) {
        considerAttacking(c);
      } else if (father && Random.roll(5)) {
        creature->dieNoReason(Creature::DropType::NOTHING);
        return;
      }
    }
    creature->wait().perform(creature);
  }

  SERIALIZE_ALL(SUBCLASS(Monster), ready, spawns, father, length)
  SERIALIZATION_CONSTRUCTOR(KrakenController)

  private:
  int SERIAL(length) = 0;
  bool SERIAL(ready) = false;
  vector<Creature*> SERIAL(spawns);
  WeakPointer<KrakenController> SERIAL(father);
};

namespace {
class ShopkeeperController : public Monster, public EventListener<ShopkeeperController> {
  public:
  ShopkeeperController(Creature* c, vector<Vec2> area)
      : Monster(c, MonsterAIFactory::stayInLocation(area)), shopArea(area) {
    CHECK(!area.empty());
  }

  virtual bool dontReplaceInCollective() override {
    return true;
  }

  vector<Position> getAllShopPositions() const {
    return shopArea.transform([this](Vec2 v){ return Position(v, myLevel); });
  }

  bool isShopPosition(const Position& pos) {
    return pos.isSameLevel(myLevel) && shopArea.contains(pos.getCoord());
  }

  virtual void makeMove() override {
    if (firstMove) {
      myLevel = creature->getLevel();
      subscribeTo(creature->getPosition().getModel());
      for (Position v : getAllShopPositions()) {
        for (Item* item : v.getItems())
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
      if (Creature* c = v.getCreature()) {
        creatures.push_back(c->getUniqueId());
        if (!prevCreatures.contains(c) && !thieves.contains(c) && !creature->isEnemy(c)) {
          if (!debtors.contains(c))
            c->secondPerson(TSentence("WELCOME_TO_SHOP", creature->getName().firstOrBare()));
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
                creature->getGame()->addEvent(EventInfo::ItemStolen{debtor, Position(shopArea.front(), myLevel)});
                thiefCount.erase(debtor);
                debtors.erase(debtor);
                thieves.insert(debtor);
                for (Item* item : debtor->getEquipment().getItems()) {
                  item->setShopkeeper(nullptr);
                  debtor->getDebt().add(creature, -item->getPrice());
                }
                break;
              }
            }
    prevCreatures.clear();
    for (auto c : creatures)
      prevCreatures.insert(c);
    Monster::makeMove();
  }

  virtual void onItemsGiven(vector<Item*> items, Creature* from) override {
    int paid = items.filter(Item::classPredicate(ItemClass::GOLD)).size();
    from->getDebt().add(creature, -paid);
    if (from->getDebt().getAmountOwed(creature) <= 0)
      debtors.erase(from);
  }

  void onEvent(const GameEvent& event) {
    using namespace EventInfo;
    event.visit<void>(
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

  SERIALIZATION_CONSTRUCTOR(ShopkeeperController)

  private:
  EntitySet<Creature> SERIAL(prevCreatures);
  EntitySet<Creature> SERIAL(debtors);
  EntityMap<Creature, int> SERIAL(thiefCount);
  EntitySet<Creature> SERIAL(thieves);
  vector<Vec2> SERIAL(shopArea);
  Level* SERIAL(myLevel) = nullptr;
  bool SERIAL(firstMove) = true;
};
}

void CreatureFactory::addInventory(Creature* c, const vector<ItemType>& items) {
  for (ItemType item : items)
    c->take(item.get(contentFactory), contentFactory);
}

PController CreatureFactory::getShopkeeper(vector<Vec2> shopArea, Creature* c) {
  return makeOwner<ShopkeeperController>(c, std::move(shopArea));
}

class IllusionController : public DoNothingController {
  public:
  IllusionController(Creature* c, GlobalTime deathT) : DoNothingController(c), deathTime(deathT) {}

  virtual void onKilled(Creature* attacker) override {
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

PCreature CreatureFactory::getIllusion(Creature* creature) {
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, TStringId("ILLUSION"));
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  auto ret = makeOwner<Creature>(viewObject, creature->getTribeId(), CATTR(
          c.viewId = ViewId("rock"); //overriden anyway
          c.illusionViewObject = creature->getViewObject();
          c.illusionViewObject->setModifier(ViewObject::Modifier::INVISIBLE, false);
          c.body = Body::nonHumanoidSpirit(Body::Size::LARGE);
          c.body->setDeathSound(SoundId("MISSED_ATTACK"));
          c.attr[AttrType("DAMAGE")] = 20; // just so it's not ignored by creatures
          c.attr[AttrType("DEFENSE")] = 1;
          c.permanentEffects[LastingEffect::FLYING] = 1;
          c.noAttackSound = true;
          c.canJoinCollective = false;
          c.name = creature->getName();), SpellMap{});
  ret->setController(makeOwner<IllusionController>(ret.get(), *creature->getGlobalTime()
      + TimeInterval(Random.get(5, 10))));
  return ret;
}

REGISTER_TYPE(BoulderController)
REGISTER_TYPE(SokobanController)
REGISTER_TYPE(KrakenController)
REGISTER_TYPE(ShopkeeperController)
REGISTER_TYPE(IllusionController)
REGISTER_TYPE(ListenerTemplate<ShopkeeperController>)

PCreature CreatureFactory::get(CreatureAttributes attr, TribeId tribe, const ControllerFactory& factory, SpellMap spells) {
  auto ret = makeOwner<Creature>(tribe, std::move(attr), std::move(spells));
  ret->setController(factory.get(ret.get()));
  return ret;
}

static pair<optional<LastingOrBuff>, ItemType> getSpecialBeastAttack(bool large, bool living, bool wings) {
  static vector<pair<optional<LastingOrBuff>, ItemType>> attacks {
    {none, ItemType(ItemType::fangs(7))},
    {LastingOrBuff(BuffId("FIRE_RESISTANT")), ItemType(ItemType::fangs(7,
        ItemPrefixes::VictimEffect{0.7, EffectType(Effects::Fire{})}))},
    {LastingOrBuff(BuffId("FIRE_RESISTANT")), ItemType(ItemType::fangs(7,
        ItemPrefixes::VictimEffect{0.7, EffectType(Effects::Fire{})}))},
    {none, ItemType(ItemType::fists(7))},
    {LastingOrBuff(LastingEffect::POISON_RESISTANT),
        ItemType(ItemType::fangs(7,
            ItemPrefixes::VictimEffect{0.3, EffectType(Effects::Lasting{none, LastingEffect::POISON})}))},
    {none, ItemType(ItemType::fangs(7))},
    {LastingOrBuff(LastingEffect::POISON_RESISTANT),
        ItemType(ItemType::fangs(7, 
            ItemPrefixes::VictimEffect{0.3, EffectType(Effects::Lasting{none, LastingEffect::POISON})}))},
    {none, ItemType(ItemType::fists(7))},
  };
  return attacks[(!large) * 4 + (!living) * 2 + wings];
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

static vector<BuffId> getResistanceAndVulnerability(RandomGen& random) {
  vector<BuffId> resistances {
      BuffId("MAGIC_RESISTANCE"),
      BuffId("MELEE_RESISTANCE"),
      BuffId("RANGED_RESISTANCE")
  };
  vector<BuffId> vulnerabilities {
      BuffId("MAGIC_VULNERABILITY"),
      BuffId("MELEE_VULNERABILITY"),
      BuffId("RANGED_VULNERABILITY")
  };
  vector<BuffId> ret;
  ret.push_back(Random.choose(resistances));
  vulnerabilities.removeIndex(*resistances.findElement(ret[0]));
  ret.push_back(Random.choose(vulnerabilities));
  return ret;
}

PCreature CreatureFactory::getSpecial(CreatureId id, TribeId tribe, SpecialParams p, const ControllerFactory& factory) {
  Body body = Body(p.humanoid, p.living ? BodyMaterialId("FLESH") : BodyMaterialId("SPIRIT"),
      p.large ? Body::Size::LARGE : Body::Size::MEDIUM);
  if (p.wings)
    body.addWithoutUpdatingPermanentEffects(BodyPart::WING, 2);
  string name = getSpeciesName(p.humanoid, p.large, p.living, p.wings);
  auto attributes = CATTR(
        c.viewId = getSpecialViewId(p.humanoid, p.large, p.living, p.wings);
        c.isSpecial = true;
        c.body = std::move(body);
        c.attr[AttrType("DAMAGE")] = Random.get(28, 34);
        c.attr[AttrType("DEFENSE")] = Random.get(28, 34);
        c.attr[AttrType("SPELL_DAMAGE")] = Random.get(28, 34);
        c.attr[AttrType("MULTI_WEAPON")] = Random.get(0, 50);
        c.permanentEffects[p.humanoid ? LastingEffect::RIDER : LastingEffect::STEED] = true;
        for (auto effect : getResistanceAndVulnerability(Random))
          c.permanentBuffs.push_back(effect);
        if (p.large) {
          c.attr[AttrType("DAMAGE")] += 6;
          c.attr[AttrType("DEFENSE")] += 2;
          c.attr[AttrType("SPELL_DAMAGE")] -= 6;
        }
        if (p.humanoid) {
          for (auto& elem : contentFactory->workshopInfo)
            c.attr[elem.second.attr] = Random.get(0, 50);
          c.maxLevelIncrease[AttrType("DAMAGE")] = 10;
          c.maxLevelIncrease[AttrType("SPELL_DAMAGE")] = 10;
          c.spellSchools = LIST(SpellSchoolId("mage"));
        }
        if (p.humanoid) {
          c.chatReactionFriendly = TString(TSentence("SPECIAL_HUMANOID_CHAT_REACTION", name));
          c.chatReactionFriendly = TString(TSentence("SPECIAL_HUMANOID_HOSTILE_CHAT_REACTION", name));
        } else {
          c.chatReactionFriendly = c.chatReactionHostile = c.petReaction = TString(TStringId("SNARLS"));
        }
        c.name = TString(name);
        c.name.setStack(p.humanoid ? TStringId("LEGENDARY_HUMANOID") : TStringId("LEGENDARY_BEAST"));
        c.name.setFirst(nameGenerator->getNext(NameGeneratorId("DEMON")));
        if (!p.humanoid) {
          c.body->setBodyParts(getSpecialBeastBody(p.large, p.living, p.wings));
          c.attr[AttrType("DAMAGE")] += 5;
          c.attr[AttrType("DEFENSE")] += 5;
          auto attack = getSpecialBeastAttack(p.large, p.living, p.wings);
          c.body->addIntrinsicAttack(BodyPart::HEAD, attack.second);
          if (attack.first)
            attack.first->visit(
                [&](LastingEffect e) { c.permanentEffects[e] = 1; },
                [&](BuffId e) { c.permanentBuffs.push_back(e); }
            );
        }
        if (Random.roll(3))
          c.permanentBuffs.push_back(BuffId("SWIMMING_SKILL"));
        );
  initializeAttributes(id, attributes);
  auto spells = getSpellMap(attributes);
  PCreature c = get(std::move(attributes), tribe, factory, std::move(spells));
  if (body.isHumanoid()) {
    if (Random.roll(4))
      c->take(ItemType(CustomItemId("Bow")).get(contentFactory), contentFactory);
    c->take(Random.choose(
          ItemType(CustomItemId("Sword")).setPrefixChance(1),
          ItemType(CustomItemId("BattleAxe")).setPrefixChance(1),
          ItemType(CustomItemId("WarHammer")).setPrefixChance(1))
        .get(contentFactory), contentFactory);
  }
  return c;
}

void CreatureFactory::initializeAttributes(optional<CreatureId> id, CreatureAttributes& attr) {
  if (id)
    attr.setCreatureId(*id);
  attr.randomize();
  attr.getBody().initializeIntrinsicAttack(contentFactory);
}

CreatureAttributes CreatureFactory::getAttributesFromId(CreatureId id) {
  auto ret = [this, id] {
    if (auto ret = getValueMaybe(attributes, id)) {
      ret->name.generateFirst(&*nameGenerator);
      return std::move(*ret);
    } else if (id == "KRAKEN") {
      auto ret = getKrakenAttributes(ViewId("kraken_head"), TStringId("KRAKEN"));
      ret.killedAchievement = AchievementId("killed_kraken");
      return ret;
    }
    FATAL << "Unrecognized creature type: \"" << id << "\"";
    fail();
  }();
  initializeAttributes(id, ret);
  return ret;
}

ControllerFactory getController(CreatureId id, MonsterAIFactory normalFactory) {
  if (id == "KRAKEN")
    return ControllerFactory([=](Creature* c) {
        return makeOwner<KrakenController>(c);
        });
  else
    return Monster::getFactory(normalFactory);
}

const map<CreatureId, CreatureFactory::SpecialParams>& CreatureFactory::getSpecialParams() {
  static map<CreatureId, CreatureFactory::SpecialParams> ret = {
    { CreatureId("SPECIAL_BLBN"), {false, true, true, false, {}}},
    { CreatureId("SPECIAL_BLBW"), {false, true, true, true, {}}},
    { CreatureId("SPECIAL_BLGN"), {false, true, false, false, {}}},
    { CreatureId("SPECIAL_BLGW"), {false, true, false, true, {}}},
    { CreatureId("SPECIAL_BMBN"), {false, false, true, false, {}}},
    { CreatureId("SPECIAL_BMBW"), {false, false, true, true, {}}},
    { CreatureId("SPECIAL_BMGN"), {false, false, false, false, {}}},
    { CreatureId("SPECIAL_BMGW"), {false, false, false, true, {}}},
    { CreatureId("SPECIAL_HLBN"), {true, true, true, false, {}}},
    { CreatureId("SPECIAL_HLBW"), {true, true, true, true, {}}},
    { CreatureId("SPECIAL_HLGN"), {true, true, false, false, {}}},
    { CreatureId("SPECIAL_HLGW"), {true, true, false, true, {}}},
    { CreatureId("SPECIAL_HMBN"), {true, false, true, false, {}}},
    { CreatureId("SPECIAL_HMBW"), {true, false, true, true, {}}},
    { CreatureId("SPECIAL_HMGN"), {true, false, false, false, {}}},
    { CreatureId("SPECIAL_HMGW"), {true, false, false, true, {}}},
  };
  return ret;
}

SpellMap CreatureFactory::getSpellMap(const CreatureAttributes& attr) {
  SpellMap spellMap;
  for (auto& schoolName : attr.spellSchools) {
    auto& school = spellSchools.at(schoolName);
    for (auto& spell : school.spells)
      spellMap.add(*getSpell(spell.first), school.expType, spell.second);
  }
  for (auto& spell : attr.spells)
    spellMap.add(*getSpell(spell), AttrType("SPELL_DAMAGE"), 0);
  return spellMap;
}

PCreature CreatureFactory::getSpirit(TribeId tribe, MonsterAIFactory aiFactory) {
  auto orig = [&] {
    for (auto id : Random.permutation(getAllCreatures())) {
      auto orig = fromId(id, tribe);
      if (orig->getBody().hasBrain(contentFactory))
        return orig;
    }
    fail();
  }();
  auto id = CreatureId("SPIRIT");
  auto attr = getAttributesFromId(id);
  auto spells = getSpellMap(attr);
  auto ret = get(std::move(attr), tribe, getController(id, aiFactory), std::move(spells));
  ret->modViewObject().setModifier(ViewObject::Modifier::ILLUSION);
  ret->getAttributes().getName() = orig->getAttributes().getName();
  ret->getAttributes().getName().setFirst(none);
  ret->getAttributes().getName().useFullTitle(false);
  ret->modViewObject().setId(orig->getViewObject().id());
  ret->getName().addBareSuffix(TStringId("SPIRIT"));
  return ret;
}

PCreature CreatureFactory::get(CreatureId id, TribeId tribe, MonsterAIFactory aiFactory) {
  ControllerFactory factory = Monster::getFactory(aiFactory);
  auto& special = getSpecialParams();
  if (special.count(id))
    return getSpecial(id, tribe, special.at(id), factory);
  else if (id == "SOKOBAN_BOULDER")
    return getSokobanBoulder(tribe);
  else if (id == "ROLLING_BOULDER_N")
    return getRollingBoulder(Vec2(0, -1));
  else if (id == "ROLLING_BOULDER_S")
    return getRollingBoulder(Vec2(0, 1));
  else if (id == "ROLLING_BOULDER_E")
    return getRollingBoulder(Vec2(-1, 0));
  else if (id == "ROLLING_BOULDER_W")
    return getRollingBoulder(Vec2(1, 0));
  else if (id == "SPIRIT")
    return getSpirit(tribe, aiFactory);
  else {
    auto attr = getAttributesFromId(id);
    auto spells = getSpellMap(attr);
    return get(std::move(attr), tribe, getController(id, aiFactory), std::move(spells));
  }
}

const Spell* CreatureFactory::getSpell(SpellId id) const {
  for (auto& spell : spells)
    if (spell.getId() == id)
      return &spell;
  return nullptr;
}

PCreature CreatureFactory::getGhost(Creature* creature) {
  ViewObject viewObject(creature->getViewObject().id(), ViewLayer::CREATURE, TStringId("GHOST"));
  viewObject.setModifier(ViewObject::Modifier::ILLUSION);
  auto ret = makeOwner<Creature>(viewObject, creature->getTribeId(), getAttributesFromId(CreatureId("LOST_SOUL")), SpellMap{});
  ret->setController(Monster::getFactory(MonsterAIFactory::monster()).get(ret.get()));
  return ret;
}

vector<ItemType> CreatureFactory::getDefaultInventory(CreatureId id) const {
  CreatureInventory empty;
  auto& inventoryGen = getSpecialParams().count(id)
      ? getSpecialParams().at(id).inventory
      : attributes.count(id) ? attributes.at(id).inventory
      : empty;
  vector<ItemType> items;
  for (auto& elem : inventoryGen)
    if (Random.chance(elem.chance)) {
      CHECK(elem.countMin <= elem.countMax) << id.data();
      for (int i : Range(Random.get(elem.countMin, elem.countMax + 1)))
        items.push_back(ItemType(elem.type).setPrefixChance(elem.prefixChance));
    }
  return items;
}

PCreature CreatureFactory::fromId(CreatureId id, TribeId t) {
  return fromId(id, t, MonsterAIFactory::monster());
}

PCreature CreatureFactory::makeCopy(Creature* c, const MonsterAIFactory& aiFactory) {
  auto attributes = c->getAttributes();
  initializeAttributes(*c->getAttributes().getCreatureId(), attributes);
  auto ret = makeOwner<Creature>(c->getTribeId(), std::move(attributes), c->getSpellMap());
  ret->modViewObject() = c->getViewObject();
  ret->setController(Monster::getFactory(aiFactory).get(ret.get()));
  return ret;
}

PCreature CreatureFactory::makeCopy(Creature* c) {
  return makeCopy(c, MonsterAIFactory::monster());
}


PCreature CreatureFactory::fromId(CreatureId id, TribeId t, const MonsterAIFactory& f) {
  return fromId(id, t, f, {});
}

PCreature CreatureFactory::fromIdNoInventory(CreatureId id, TribeId t, const MonsterAIFactory& f) {
  return get(id, t, f);
}

PCreature CreatureFactory::fromId(CreatureId id, TribeId t, const MonsterAIFactory& factory, const vector<ItemType>& inventory) {
  auto ret = get(id, t, factory);
  addInventory(ret.get(), inventory);
  addInventory(ret.get(), getDefaultInventory(id));
  return ret;
}

PCreature CreatureFactory::getHumanForTests() {
  auto attributes = CATTR(
      c.viewId = ViewId("keeper1");
      c.attr[AttrType("DAMAGE")] = 12;
      c.attr[AttrType("DEFENSE")] = 12;
      c.attr[AttrType("RANGED_DAMAGE")] = 12;
      c.body = Body::humanoid(Body::Size::LARGE);
      c.name = TString(TStringId("WIZARD"));
      c.viewIdUpgrades = LIST(ViewId("keeper2"), ViewId("keeper3"), ViewId("keeper4"));
      c.name.setFirst("keeper"_s);
      c.name.useFullTitle();
      //c.skills.setValue(WorkshopType("LABORATORY"), 0.2);
      c.maxLevelIncrease[AttrType("DAMAGE")] = 7;
      c.maxLevelIncrease[AttrType("SPELL_DAMAGE")] = 12;
      //c.spells->add(SpellId::HEAL_SELF);
  );
  return get(std::move(attributes), TribeId::getMonster(), Monster::getFactory(MonsterAIFactory::idle()), SpellMap{});
}
