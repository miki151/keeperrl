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

#include "square_factory.h"
#include "square.h"
#include "creature.h"
#include "level.h"
#include "item_factory.h"
#include "creature_factory.h"
#include "pantheon.h"
#include "effect.h"
#include "view_object.h"
#include "view_id.h"
#include "model.h"
#include "monster_ai.h"
#include "player_message.h"
#include "square_apply_type.h"
#include "event.h"
#include "tribe.h"
#include "entity_name.h"

class Staircase : public Square {
  public:
  Staircase(const ViewObject& obj, const string& name, StairDirection dir, StairKey key) : Square(obj,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.canHide = true;
        c.strength = 10000;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);)) {
    setLandingLink(dir, key);
  }

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage("There are " + getName() + " here.");
  }

  virtual optional<SquareApplyType> getApplyType() const override {
    switch (getLandingLink()->first) {
      case StairDirection::DOWN: return SquareApplyType::DESCEND;
      case StairDirection::UP: return SquareApplyType::ASCEND;
    }
    return none;
  }

  virtual void onApply(Creature* c) override {
    auto link = getLandingLink();
    getLevel()->changeLevel(link->first, link->second, c);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Square);
  }

  SERIALIZATION_CONSTRUCTOR(Staircase);

};

class Magma : public Square {
  public:
  Magma(const ViewObject& object, const string& name) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.constructions[SquareId::BRIDGE] = 20;
        c.movementSet = MovementSet()
            .addTrait(MovementTrait::FLY)
            .addForcibleTrait(MovementTrait::WALK);)) {}

  virtual void onEnterSpecial(Creature* c) override {
    MovementType realMovement = c->getMovementType();
    realMovement.setForced(false);
    if (!canEnterEmpty(realMovement)) {
      c->you(MsgType::BURN, getName());
      c->die(nullptr, false);
    }
  }

  virtual void dropItems(vector<PItem> items) override {
    for (auto elem : Item::stackItems(extractRefs(items)))
      getLevel()->globalMessage(getPosition(), elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
            "burns", "burn") + " in the magma.");
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square);
  }

  SERIALIZATION_CONSTRUCTOR(Magma);
};

class Water : public Square {
  public:
  Water(ViewObject object, const string& name, double _depth)
      : Square(object.setAttribute(ViewObject::Attribute::WATER_DEPTH, _depth),
          CONSTRUCT(Square::Params,
            c.name = name;
            c.vision = VisionId::NORMAL;
            c.constructions[SquareId::BRIDGE] = 20;
            c.movementSet = getMovement(_depth);
          )) {}

  virtual void onEnterSpecial(Creature* c) override {
    MovementType realMovement = c->getMovementType();
    realMovement.setForced(false);
    if (!canEnterEmpty(realMovement)) {
      c->you(MsgType::DROWN, getName());
      c->die(nullptr, false);
    }
  }

  virtual void dropItems(vector<PItem> items) override {
    for (auto elem : Item::stackItems(extractRefs(items)))
      getLevel()->globalMessage(getPosition(), elem.second[0]->getPluralTheNameAndVerb(elem.second.size(),
            "sinks", "sink") + " in the water.", "You hear a splash.");
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square);
  }

  SERIALIZATION_CONSTRUCTOR(Water);
  
  private:
  static MovementSet getMovement(double depth) {
    MovementSet ret;
    ret.addTrait(MovementTrait::SWIM);
    ret.addTrait(MovementTrait::FLY);
    ret.addForcibleTrait(MovementTrait::WALK);
    if (depth < 1.5)
      ret.addTrait(MovementTrait::WADE);
    return ret;
  }
};

class Chest : public Square {
  public:
  Chest(const ViewObject& object, const ViewObject& opened, const string& name, CreatureId c,
      int numC, const string& _msgItem, const string& _msgMonster, const string& _msgGold, ItemFactory _itemFactory)
    : Square(object,
          CONSTRUCT(Square::Params,
            c.name = name;
            c.vision = VisionId::NORMAL;
            c.canHide = true;
            c.canDestroy = true;
            c.strength = 30;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.flamability = 0.5;)), creature(c), numCreatures(numC),
    msgItem(_msgItem), msgMonster(_msgMonster), msgGold(_msgGold), itemFactory(_itemFactory), openedObject(opened) {}

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage(string("There is a ") + (opened ? " opened " : "") + getName() + " here");
  }

  virtual void onConstructNewSquare(Square* s) override {
    if (!opened)
      s->dropItems(itemFactory.random());
  }

  virtual bool canApply(const Creature* c) const {
    return c->isHumanoid();
  }

  virtual optional<SquareApplyType> getApplyType() const override { 
    if (opened) 
      return none;
    else
      return SquareApplyType::USE_CHEST;
  }

  virtual void onApply(Creature* c) override {
    CHECK(!opened);
    c->playerMessage("You open the " + getName());
    opened = true;
    setViewObject(openedObject);
    if (!Random.roll(5)) {
      c->playerMessage(msgItem);
      vector<PItem> items = itemFactory.random();
      GlobalEvents.addItemsAppearedEvent(getLevel(), getPosition(), extractRefs(items));
      c->takeItems(std::move(items), nullptr);
    } else {
      c->playerMessage(msgMonster);
      int numR = numCreatures;
      for (Vec2 v : getPosition().neighbors8(true)) {
        PCreature rat = CreatureFactory::fromId(creature, getLevel()->getModel()->getPestTribe());
        if (getLevel()->getSafeSquare(v)->canEnter(rat.get())) {
          getLevel()->addCreature(v, std::move(rat));
          if (--numR == 0)
            break;
        }
      }
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(creature)
      & SVAR(numCreatures) 
      & SVAR(msgItem)
      & SVAR(msgMonster)
      & SVAR(msgGold)
      & SVAR(opened)
      & SVAR(itemFactory)
      & SVAR(openedObject);
  }

  SERIALIZATION_CONSTRUCTOR(Chest);

  private:
  CreatureId SERIAL(creature);
  int SERIAL(numCreatures);
  string SERIAL(msgItem);
  string SERIAL(msgMonster);
  string SERIAL(msgGold);
  bool SERIAL(opened) = false;
  ItemFactory SERIAL(itemFactory);
  ViewObject SERIAL(openedObject);
};

class Fountain : public Square {
  public:
  Fountain(const ViewObject& object) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "fountain";
        c.vision = VisionId::NORMAL;
        c.canHide = true;
        c.canDestroy = true;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.strength = 100;)) {}

  virtual optional<SquareApplyType> getApplyType() const override { 
    return SquareApplyType::DRINK;
  }

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage("There is a " + getName() + " here");
  }

  virtual void onApply(Creature* c) override {
    c->playerMessage("You drink from the fountain.");
    PItem potion = getOnlyElement(ItemFactory::potions().random(seed));
    potion->apply(c, getLevel());
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(seed);
  }

  SERIALIZATION_CONSTRUCTOR(Fountain);

  private:
  int SERIAL(seed) = Random.get(123456);
};

class Tree : public Square {
  public:
  Tree(const ViewObject& object, const string& name, VisionId vision, int _numWood,
      map<SquareId, int> construct, CreatureId c) : Square(object,
        CONSTRUCT(Square::Params,
          c.name = name;
          c.vision = vision;
          c.canHide = true;
          c.strength = 100;
          c.canDestroy = true;
          c.flamability = 0.4;
          c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
          c.constructions = construct;)), numWood(_numWood), creature(c) {}

  virtual void destroy() override {
    getLevel()->globalMessage(getPosition(), "The tree falls.");
    getLevel()->replaceSquare(getPosition(), SquareFactory::get(SquareId::TREE_TRUNK));
  }

  virtual void onConstructNewSquare(Square* s) override {
    if (numWood > 0) {
      s->dropItems(ItemFactory::fromId(ItemId::WOOD_PLANK, numWood));
      getLevel()->getModel()->addWoodCount(numWood);
      int numCut = getLevel()->getModel()->getWoodCount();
      if (numCut > 1500 && Random.roll(max(50, (3000 - numCut) / 10)))
        Effect::summon(getLevel(), CreatureFactory::singleType(
              getLevel()->getModel()->getKillEveryoneTribe(), creature), getPosition(), 1, 100000);
    }
  }

  virtual void burnOut() override {
    numWood = 0;
    getLevel()->replaceSquare(getPosition(), SquareFactory::get(SquareId::BURNT_TREE));
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(creature)
      & SVAR(numWood);
  }

  SERIALIZATION_CONSTRUCTOR(Tree);

  private:
  int SERIAL(numWood);
  CreatureId SERIAL(creature);
};

class TrapSquare : public Square {
  public:
  TrapSquare(const ViewObject& object, EffectType e) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "floor";
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.vision = VisionId::NORMAL;)), effect(e) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (active && c->isPlayer()) {
      c->you(MsgType::TRIGGER_TRAP, "");
      Effect::applyToCreature(c, effect, EffectStrength::NORMAL);
      active = false;
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(active)
      & SVAR(effect);
  }

  SERIALIZATION_CONSTRUCTOR(TrapSquare);

  private:
  bool SERIAL(active) = true;
  EffectType SERIAL(effect);
};

class Door : public Square {
  public:
  Door(const ViewObject& object, Square::Params params) : Square(object, params) {}

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage("You open the door.");
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Square);
  }
  
  SERIALIZATION_CONSTRUCTOR(Door);
};

class TribeDoor : public Door {
  public:

  TribeDoor(const ViewObject& object, const Tribe* t, int destStrength, Square::Params params)
    : Door(object, CONSTRUCT(Square::Params,
          c = params;
          c.movementSet
              .addTraitForTribe(t, MovementTrait::WALK)
              .removeTrait(MovementTrait::WALK); )),
      tribe(t), destructionStrength(destStrength) {
  }

  virtual void destroyBy(Creature* c) override {
    destructionStrength -= c->getAttr(AttrType::STRENGTH);
    if (destructionStrength <= 0) {
      Door::destroyBy(c);
    }
  }

  virtual bool canLock() const {
    return true;
  }

  virtual bool isLocked() const {
    return locked;
  }

  virtual void lock() {
    locked = !locked;
    if (locked) {
      modViewObject().setModifier(ViewObject::Modifier::LOCKED);
      removeTraitForTribe(tribe, MovementTrait::WALK);
    } else {
      modViewObject().removeModifier(ViewObject::Modifier::LOCKED);
      addTraitForTribe(tribe, MovementTrait::WALK);
    }
    setDirty();
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Door)
      & SVAR(tribe)
      & SVAR(destructionStrength)
      & SVAR(locked);
  }

  SERIALIZATION_CONSTRUCTOR(TribeDoor);

  private:
  const Tribe* SERIAL(tribe);
  int SERIAL(destructionStrength);
  bool SERIAL(locked) = false;
};

class Barricade : public Square {
  public:
  Barricade(const ViewObject& object, const Tribe* t, int destStrength) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "barricade";
        c.vision = VisionId::NORMAL;
        c.canDestroy = true;
        c.flamability = 0.5;
        c.owner = t;)),
      destructionStrength(destStrength) {
  }

  virtual void destroyBy(Creature* c) override {
    destructionStrength -= c->getAttr(AttrType::STRENGTH);
    if (destructionStrength <= 0) {
      Square::destroyBy(c);
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(destructionStrength);
  }

  SERIALIZATION_CONSTRUCTOR(Barricade);

  private:
  int SERIAL(destructionStrength);
};

class Furniture : public Square {
  public:
  Furniture(ViewObject object, const string& name, double flamability,
      optional<SquareApplyType> _applyType = none) 
      : Square(object.setModifier(ViewObject::Modifier::ROUND_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = name;
            c.vision = VisionId::NORMAL;
            c.canHide = true;
            c.strength = 100;
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.flamability = flamability;)), applyType(_applyType) {}

  virtual optional<SquareApplyType> getApplyType() const override {
    return applyType;
  }

  virtual void onApply(Creature* c) {}

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(applyType);
  }

  SERIALIZATION_CONSTRUCTOR(Furniture);
  
  private:
  optional<SquareApplyType> SERIAL(applyType);
};

class Bed : public Furniture {
  public:
  Bed(const ViewObject& object, const string& name, double flamability = 1) : Furniture(object, name, flamability) {}

  virtual optional<SquareApplyType> getApplyType() const override { 
    return SquareApplyType::SLEEP;
  }

  virtual void onApply(Creature* c) override {
    Effect::applyToCreature(c, {EffectId::LASTING, LastingEffect::SLEEP}, EffectStrength::STRONG);
    getLevel()->addTickingSquare(getPosition());
  }

  virtual void tickSpecial(double time) override {
    if (getCreature() && getCreature()->isAffected(LastingEffect::SLEEP))
      getCreature()->heal(0.005);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Furniture);
  }
  
  SERIALIZATION_CONSTRUCTOR(Bed);
};

class Grave : public Bed {
  public:
  Grave(const ViewObject& object, const string& name) : Bed(object, name, 0) {}

  virtual bool canApply(const Creature* c) const {
    return c->isUndead();
  }

  virtual optional<SquareApplyType> getApplyType() const override { 
    return SquareApplyType::SLEEP;
  }

  virtual void onApply(Creature* c) override {
    CHECK(c->isUndead());
    Bed::onApply(c);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Bed);
  }

  SERIALIZATION_CONSTRUCTOR(Grave);
};

class Altar : public Square {
  public:
  Altar(const ViewObject& object) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = "shrine";
        c.vision = VisionId::NORMAL;
        c.canHide = true;
        c.canDestroy = true;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.strength = 100;)) {
  }

  virtual bool canApply(const Creature* c) const {
    return c->isHumanoid();
  }

  virtual optional<SquareApplyType> getApplyType() const override { 
    return SquareApplyType::PRAY;
  }

  virtual void onKilled(Creature* victim, Creature* killer) override {
    if (killer) {
      recentKiller = killer;
      recentVictim = victim;
      killTime = killer->getTime();
    }
  }

  virtual void onPrayer(Creature* c) = 0;
  virtual void onSacrifice(Creature* c) = 0;
  virtual string getName() = 0;

  virtual void onApply(Creature* c) override {
    if (c == recentKiller && recentVictim && killTime >= c->getTime() - sacrificeTimeout)
      for (Item* it : getItems(Item::classPredicate(ItemClass::CORPSE)))
        if (it->getCorpseInfo()->victim == recentVictim->getUniqueId()) {
          c->you(MsgType::SACRIFICE, getName());
          c->globalMessage(it->getTheName() + " is consumed in a flash of light!");
          removeItem(it);
          onSacrifice(c);
          return;
        }
    c->playerMessage("You pray to " + getName());
    onPrayer(c);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(recentKiller)
      & SVAR(recentVictim)
      & SVAR(killTime);
  }

  SERIALIZATION_CONSTRUCTOR(Altar);

  private:
  const Creature* SERIAL(recentKiller) = nullptr;
  const Creature* SERIAL(recentVictim) = nullptr;
  double SERIAL(killTime) = -100;
  const double sacrificeTimeout = 50;
};

class DeityAltar : public Altar {
  public:
  DeityAltar(const ViewObject& object, Deity* d) : Altar(ViewObject(object.id(), object.layer(),
        "Shrine to " + d->getName())), deity(d) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (c->isHumanoid()) {
      c->playerMessage("This is a shrine to " + deity->getName());
      c->playerMessage(deity->getGender().he() + " lives in " + deity->getHabitatString());
      c->playerMessage(deity->getGender().he() + " is the " + deity->getGender().god() + " of "
          + deity->getEpithetsString());
    }
  }

  virtual string getName() override {
    return deity->getName();
  }

  virtual void destroyBy(Creature* c) override {
 //   GlobalEvents.addWorshipEvent(c, deity, WorshipType::DESTROY_ALTAR);
 //   Altar::destroyBy(c);
  }

  virtual void onPrayer(Creature* c) override {
 //   GlobalEvents.addWorshipEvent(c, deity, WorshipType::PRAYER);
  }

  virtual void onSacrifice(Creature* c) override {
 //   GlobalEvents.addWorshipEvent(c, deity, WorshipType::SACRIFICE);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Altar)
      & SVAR(deity);
  }

  SERIALIZATION_CONSTRUCTOR(DeityAltar);

  private:
  Deity* SERIAL(deity);
};

class CreatureAltar : public Altar {
  public:
  CreatureAltar(const ViewObject& object, const Creature* c) : Altar(ViewObject(object.id(), object.layer(),
        "Shrine to " + c->getName().bare())), creature(c) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (c->isHumanoid()) {
      c->playerMessage("This is a shrine to " + creature->getName().bare());
      c->playerMessage(creature->getDescription());
    }
  }

  virtual void destroyBy(Creature* c) override {
  //  GlobalEvents.addWorshipCreatureEvent(c, creature, WorshipType::DESTROY_ALTAR);
  //  Altar::destroyBy(c);
  }

  virtual string getName() override {
    return creature->getName().bare();
  }

  virtual void onPrayer(Creature* c) override {
//    GlobalEvents.addWorshipCreatureEvent(c, creature, WorshipType::PRAYER);
  }

  virtual void onSacrifice(Creature* c) override {
 //   GlobalEvents.addWorshipCreatureEvent(c, creature, WorshipType::SACRIFICE);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Altar)
      & SVAR(creature);
  }

  SERIALIZATION_CONSTRUCTOR(CreatureAltar);

  private:
  const Creature* SERIAL(creature);
};

class ConstructionDropItems : public Square {
  public:
  ConstructionDropItems(const ViewObject& object, const string& name,
      map<SquareId, int> constructions, vector<PItem> _items) : Square(object,
        CONSTRUCT(Square::Params,
          c.name = name;
          c.constructions = constructions;)),
      items(std::move(_items)) {}

  virtual void onConstructNewSquare(Square* s) override {
    s->dropItems(std::move(items));
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(items);
  }

  SERIALIZATION_CONSTRUCTOR(ConstructionDropItems);

  private:
  vector<PItem> SERIAL(items);
};

class Torch : public Furniture {
  public:
  Torch(const ViewObject& object, const string& name) : Furniture(object, name, 1) {}

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Furniture);
  }

  double getLightEmission() const override {
    return 8.2;
  }

  SERIALIZATION_CONSTRUCTOR(Torch);
};

class Hatchery : public Square {
  public:
  Hatchery(const ViewObject& object, const string& name, CreatureFactory c) : Square(object,
      CONSTRUCT(Square::Params,
        c.name = name;
        c.vision = VisionId::NORMAL;
        c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
        c.canDestroy = true;
        c.ticking = true;)),
    creature(c) {}

  virtual void tickSpecial(double time) override {
    if (getCreature() || !Random.roll(10) || getPoisonGasAmount() > 0)
      return;
    for (Square* s : getLevel()->getSquares(getPosition().neighbors8()))
      if (s->getCreature() && s->getCreature()->isMinionFood())
        return;
    if (Random.roll(5))
      getLevel()->addCreature(getPosition(), creature.random(
            MonsterAIFactory::stayInPigsty(getPosition(), SquareApplyType::PIGSTY)));
  }

  virtual optional<SquareApplyType> getApplyType() const override {
    return SquareApplyType::PIGSTY;
  }

  virtual void onApply(Creature* c) override {
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(creature);
  }
  
  SERIALIZATION_CONSTRUCTOR(Hatchery);

  private:
  CreatureFactory SERIAL(creature);
};

class Laboratory : public Furniture {
  public:
  Laboratory(ViewObject object, const string& name) : Furniture(object, name, 0, SquareApplyType::WORKSHOP) {}

  virtual void onApply(Creature* c) override {
    c->playerMessage("You mix the concoction.");
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Furniture);
  }
  
  SERIALIZATION_CONSTRUCTOR(Laboratory);
};

class Crops : public Square {
  public:
  using Square::Square;

  virtual optional<SquareApplyType> getApplyType() const override {
    return SquareApplyType::CROPS;
  }

  virtual void onApply(Creature* c) override {
    if (Random.roll(3))
      c->globalMessage(c->getName().the() + " scythes the field.");
  }

  virtual double getApplyTime() const {
    return 3.0;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Square);
  }
};

PSquare SquareFactory::getAltar(Deity* deity) {
  return PSquare(new DeityAltar(ViewObject(ViewId::ALTAR, ViewLayer::FLOOR, "Shrine"), deity));
}

PSquare SquareFactory::getAltar(Creature* creature) {
  return PSquare(new CreatureAltar(ViewObject(ViewId::ALTAR, ViewLayer::FLOOR, "Shrine"), creature));
}

template <class Archive>
void SquareFactory::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, Laboratory);
  REGISTER_TYPE(ar, Staircase);
  REGISTER_TYPE(ar, Magma);
  REGISTER_TYPE(ar, Water);
  REGISTER_TYPE(ar, Chest);
  REGISTER_TYPE(ar, Fountain);
  REGISTER_TYPE(ar, Tree);
  REGISTER_TYPE(ar, TrapSquare);
  REGISTER_TYPE(ar, Door);
  REGISTER_TYPE(ar, TribeDoor);
  REGISTER_TYPE(ar, Furniture);
  REGISTER_TYPE(ar, Bed);
  REGISTER_TYPE(ar, Barricade);
  REGISTER_TYPE(ar, Torch);
  REGISTER_TYPE(ar, Grave);
  REGISTER_TYPE(ar, DeityAltar);
  REGISTER_TYPE(ar, CreatureAltar);
  REGISTER_TYPE(ar, ConstructionDropItems);
  REGISTER_TYPE(ar, Hatchery);
  REGISTER_TYPE(ar, Crops);
}

REGISTER_TYPES(SquareFactory::registerTypes);

PSquare SquareFactory::get(SquareType s) {
  return PSquare(getPtr(s));
}

Square* SquareFactory::getPtr(SquareType s) {
  switch (s.getId()) {
    case SquareId::FLOOR:
        return new Square(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR_BACKGROUND, "Floor"),
            CONSTRUCT(Square::Params,
              c.name = "floor";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.constructions[SquareId::TREASURE_CHEST] = 10;
              c.constructions[SquareId::DORM] = 10;
              c.constructions[SquareId::TRIBE_DOOR] = 10;
              c.constructions[SquareId::TRAINING_ROOM] = 10;
              c.constructions[SquareId::LIBRARY] = 10;
              c.constructions[SquareId::HATCHERY] = 10;
              c.constructions[SquareId::STOCKPILE] = 1;
              c.constructions[SquareId::STOCKPILE_EQUIP] = 1;
              c.constructions[SquareId::STOCKPILE_RES] = 1;
              c.constructions[SquareId::CEMETERY] = 10;
              c.constructions[SquareId::WORKSHOP] = 10;
              c.constructions[SquareId::FORGE] = 10;
              c.constructions[SquareId::LABORATORY] = 10;
              c.constructions[SquareId::JEWELER] = 10;
              c.constructions[SquareId::PRISON] = 10;
              c.constructions[SquareId::TORTURE_TABLE] = 10;
              c.constructions[SquareId::BEAST_LAIR] = 10;
              c.constructions[SquareId::IMPALED_HEAD] = 5;
              c.constructions[SquareId::BARRICADE] = 20;
              c.constructions[SquareId::TORCH] = 5;
              c.constructions[SquareId::ALTAR] = 35;
              c.constructions[SquareId::EYEBALL] = 5;
              c.constructions[SquareId::CREATURE_ALTAR] = 35;
              c.constructions[SquareId::MINION_STATUE] = 35;
              c.constructions[SquareId::THRONE] = 100;
              c.constructions[SquareId::RITUAL_ROOM] = 10;));
    case SquareId::BLACK_FLOOR:
        return new Square(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR_BACKGROUND, "Floor"),
            CONSTRUCT(Square::Params,
              c.name = "floor";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::BRIDGE:
        return new Square(ViewObject(ViewId::BRIDGE, ViewLayer::FLOOR,"Bridge"),
            CONSTRUCT(Square::Params,
              c.name = "rope bridge";
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.vision = VisionId::NORMAL;));
    case SquareId::GRASS:
        return new Square(ViewObject(ViewId::GRASS, ViewLayer::FLOOR_BACKGROUND, "Grass"),
            CONSTRUCT(Square::Params,
              c.name = "grass";
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
              c.vision = VisionId::NORMAL;
              c.constructions[SquareId::EYEBALL] = 5;
              c.constructions[SquareId::IMPALED_HEAD] = 5;));
    case SquareId::CROPS:
        return new Crops(ViewObject(chooseRandom({ViewId::CROPS, ViewId::CROPS2}),
                ViewLayer::FLOOR_BACKGROUND, "Wheat"),
            CONSTRUCT(Square::Params,
              c.name = "wheat";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::MUD:
        return new Square(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND, "Mud"),
            CONSTRUCT(Square::Params,
              c.name = "mud";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::ROAD:
        return new Square(ViewObject(ViewId::ROAD, ViewLayer::FLOOR, "Road")
            .setModifier(ViewObject::Modifier::ROAD),
            CONSTRUCT(Square::Params,
              c.name = "road";
              c.vision = VisionId::NORMAL;
              c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::ROCK_WALL:
        return new Square(ViewObject(ViewId::WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW),
            CONSTRUCT(Square::Params,
              c.name = "wall";
              c.constructions[SquareId::FLOOR] = Random.get(3, 8);));
    case SquareId::GOLD_ORE:
        return new ConstructionDropItems(ViewObject(ViewId::GOLD_ORE, ViewLayer::FLOOR, "Gold ore")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "gold ore",
            {{SquareId::FLOOR, Random.get(30, 80)}},
            ItemFactory::fromId(ItemId::GOLD_PIECE, Random.get(18, 40)));
    case SquareId::IRON_ORE:
        return new ConstructionDropItems(ViewObject(ViewId::IRON_ORE, ViewLayer::FLOOR, "Iron ore")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "iron ore",
            {{SquareId::FLOOR, Random.get(15, 40)}},
            ItemFactory::fromId(ItemId::IRON_ORE, Random.get(18, 40)));
    case SquareId::STONE:
        return new ConstructionDropItems(ViewObject(ViewId::STONE, ViewLayer::FLOOR, "Granite")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "granite",
            {{SquareId::FLOOR, Random.get(30, 80)}},
            ItemFactory::fromId(ItemId::ROCK, Random.get(18, 40)));
    case SquareId::LOW_ROCK_WALL:
        return new Square(ViewObject(ViewId::LOW_ROCK_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::WOOD_WALL:
        return new Square(ViewObject(ViewId::WOOD_WALL, ViewLayer::FLOOR, "Wooden wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params,
              c.name = "wall";
              c.flamability = 0.4;));
    case SquareId::BLACK_WALL:
        return new Square(ViewObject(ViewId::EMPTY, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::YELLOW_WALL:
        return new Square(ViewObject(ViewId::YELLOW_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::HELL_WALL:
        return new Square(ViewObject(ViewId::HELL_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::CASTLE_WALL:
        return new Square(ViewObject(ViewId::CASTLE_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::MUD_WALL:
        return new Square(ViewObject(ViewId::MUD_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::MOUNTAIN:
        return new Square(ViewObject(ViewId::MOUNTAIN, ViewLayer::FLOOR, "Mountain"),
          CONSTRUCT(Square::Params,
            c.name = "mountain";
            c.vision = VisionId::NORMAL;));
    case SquareId::MOUNTAIN2:
        return new Square(ViewObject(ViewId::MOUNTAIN2, ViewLayer::FLOOR, "Mountain")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "mountain";
            c.constructions[SquareId::FLOOR] = Random.get(3, 8);));
    case SquareId::GLACIER:
        return new Square(ViewObject(ViewId::SNOW, ViewLayer::FLOOR, "Mountain"),
          CONSTRUCT(Square::Params,
            c.name = "mountain";
            c.vision = VisionId::NORMAL;));
    case SquareId::HILL:
        return new Square(ViewObject(ViewId::HILL, ViewLayer::FLOOR_BACKGROUND, "Hill"),
          CONSTRUCT(Square::Params,
            c.name = "hill";
            c.vision = VisionId::NORMAL;
            c.constructions[SquareId::EYEBALL] = 5;
            c.constructions[SquareId::IMPALED_HEAD] = 5;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);));
    case SquareId::WATER:
        return new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND, "Water"), "water", 100);
    case SquareId::MAGMA: 
        return new Magma(ViewObject(ViewId::MAGMA, ViewLayer::FLOOR, "Magma"), "magma");
    case SquareId::ABYSS: 
        FAIL << "Unimplemented";
    case SquareId::SAND:
        return new Square(ViewObject(ViewId::SAND, ViewLayer::FLOOR_BACKGROUND, "Sand"),
          CONSTRUCT(Square::Params,
            c.name = "sand";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::CANIF_TREE:
        return new Tree(ViewObject(ViewId::CANIF_TREE, ViewLayer::FLOOR, "Tree")
                .setModifier(ViewObject::Modifier::ROUND_SHADOW), "tree", VisionId::ELF,
            Random.get(25, 40), {{SquareId::TREE_TRUNK, 20}}, s.get<CreatureId>());
    case SquareId::DECID_TREE:
        return new Tree(ViewObject(ViewId::DECID_TREE, ViewLayer::FLOOR, "Tree")
                .setModifier(ViewObject::Modifier::ROUND_SHADOW),
            "tree", VisionId::ELF, Random.get(25, 40), {{SquareId::TREE_TRUNK, 20}},
            s.get<CreatureId>());
    case SquareId::BUSH:
        return new Tree(ViewObject(ViewId::BUSH, ViewLayer::FLOOR, "Bush"), "bush",
            VisionId::NORMAL, Random.get(5, 10), {{SquareId::TREE_TRUNK, 10}},
            s.get<CreatureId>());
    case SquareId::TREE_TRUNK:
        return new Square(ViewObject(ViewId::TREE_TRUNK, ViewLayer::FLOOR, "tree trunk"),
          CONSTRUCT(Square::Params,
            c.name = "tree trunk";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::BURNT_TREE:
        return new Square(ViewObject(ViewId::BURNT_TREE, ViewLayer::FLOOR, "burnt tree"),
          CONSTRUCT(Square::Params,
            c.name = "burnt tree";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::BED:
        return new Bed(ViewObject(ViewId::BED, ViewLayer::FLOOR, "Bed"), "bed");
    case SquareId::DORM:
        return new Square(ViewObject(ViewId::DORM, ViewLayer::FLOOR_BACKGROUND, "Dormitory"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.canDestroy = true;
            c.constructions[SquareId::BED] = 10;
            c.vision = VisionId::NORMAL;));
    case SquareId::TORCH: return new Torch(ViewObject(ViewId::TORCH, ViewLayer::FLOOR, "Torch"), "torch");
    case SquareId::STOCKPILE:
        return new Square(ViewObject(ViewId::STOCKPILE1, ViewLayer::FLOOR_BACKGROUND, "Storage (all)"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::STOCKPILE_EQUIP:
        return new Square(ViewObject(ViewId::STOCKPILE2, ViewLayer::FLOOR_BACKGROUND,
              "Storage (equipment)"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::STOCKPILE_RES:
        return new Square(ViewObject(ViewId::STOCKPILE3, ViewLayer::FLOOR_BACKGROUND,
              "Storage (resources)"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::PRISON:
        return new Square(ViewObject(ViewId::PRISON, ViewLayer::FLOOR_BACKGROUND, "Prison"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::WELL:
        return new Furniture(ViewObject(ViewId::WELL, ViewLayer::FLOOR, "Well"), "well", 0);
    case SquareId::STATUE:
        return new Furniture(
            ViewObject(chooseRandom({ViewId::STATUE1, ViewId::STATUE2}), ViewLayer::FLOOR, "Statue"), "statue", 0);
    case SquareId::TORTURE_TABLE:
        return new Furniture(ViewObject(ViewId::TORTURE_TABLE, ViewLayer::FLOOR, "Torture room"), 
            "torture room", 0.3, SquareApplyType::TORTURE);
    case SquareId::BEAST_CAGE:
        return new Bed(ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR, "Beast cage"), "beast cage");
    case SquareId::BEAST_LAIR:
        return new Square(ViewObject(ViewId::BEAST_LAIR, ViewLayer::FLOOR_BACKGROUND, "Beast lair"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.constructions[SquareId::BEAST_CAGE] = 10;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::TRAINING_ROOM:
        return new Furniture(ViewObject(ViewId::TRAINING_ROOM, ViewLayer::FLOOR, "Training room"), 
            "training room", 1, SquareApplyType::TRAIN);
    case SquareId::THRONE:
        return new Furniture(ViewObject(ViewId::THRONE, ViewLayer::FLOOR, "Throne"), 
            "throne", 0, SquareApplyType::THRONE);
    case SquareId::RITUAL_ROOM:
        return new Square(ViewObject(ViewId::RITUAL_ROOM, ViewLayer::FLOOR, "Ritual room"),
          CONSTRUCT(Square::Params,
            c.name = "ritual room";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::IMPALED_HEAD:
        return new Square(ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR, "Impaled head")
            .setModifier(ViewObject::Modifier::ROUND_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "impaled head";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::EYEBALL:
        return new Square(ViewObject(ViewId::EYEBALL, ViewLayer::FLOOR, "Eyeball")
            .setModifier(ViewObject::Modifier::ROUND_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "eyeball";
            c.canDestroy = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::LIBRARY:
        return new Furniture(ViewObject(ViewId::LIBRARY, ViewLayer::FLOOR, "Library"), 
            "library", 1, SquareApplyType::TRAIN);
    case SquareId::CAULDRON:
        return new Laboratory(ViewObject(ViewId::CAULDRON, ViewLayer::FLOOR, "cauldron"), "cauldron");
    case SquareId::LABORATORY:
      return new Laboratory(ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR, "Laboratory"), "laboratory");
    case SquareId::FORGE:
        return new Furniture(ViewObject(ViewId::FORGE, ViewLayer::FLOOR, "Forge"), "forge", 1,
            SquareApplyType::WORKSHOP);
    case SquareId::WORKSHOP:
      return new Furniture(ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR, "Workshop stand"),"workshop stand", 1,
            SquareApplyType::WORKSHOP);
    case SquareId::JEWELER:
      return new Furniture(ViewObject(ViewId::JEWELER, ViewLayer::FLOOR, "Jeweler stand"),"jeweler stand", 1,
            SquareApplyType::WORKSHOP);
    case SquareId::MINION_STATUE:
      return new Furniture(ViewObject(ViewId::CREATURE_ALTAR, ViewLayer::FLOOR, "Statue"),"statue", 0,
            SquareApplyType::STATUE);
    case SquareId::HATCHERY:
        return new Hatchery(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND, "Hatchery"), "hatchery",
            s.get<CreatureFactory::SingleCreature>());
    case SquareId::ALTAR:
        return new DeityAltar(ViewObject(ViewId::ALTAR, ViewLayer::FLOOR, "Shrine"),
              Deity::getDeity(s.get<DeityHabitat>()));
    case SquareId::CREATURE_ALTAR:
        return new CreatureAltar(ViewObject(ViewId::CREATURE_ALTAR, ViewLayer::FLOOR, "Shrine"),
              s.get<const Creature*>());
    case SquareId::ROLLING_BOULDER:
        return new TrapSquare(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR, "floor"), EffectId::ROLLING_BOULDER);
    case SquareId::POISON_GAS:
        return new TrapSquare(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR, "floor"), EffectId::EMIT_POISON_GAS);
    case SquareId::FOUNTAIN:
        return new Fountain(ViewObject(ViewId::FOUNTAIN, ViewLayer::FLOOR, "Fountain"));
    case SquareId::CHEST:
        return new Chest(ViewObject(ViewId::CHEST, ViewLayer::FLOOR, "Chest"),
            ViewObject(ViewId::OPENED_CHEST, ViewLayer::FLOOR, "Opened chest"), "chest",
            s.get<CreatureId>(), Random.get(3, 6),
            "There is an item inside", "It's full of rats!", "There is gold inside", ItemFactory::chest());
    case SquareId::TREASURE_CHEST:
        return new Furniture(ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR, "Chest"), "chest", 1);
    case SquareId::COFFIN:
        return new Chest(ViewObject(ViewId::COFFIN, ViewLayer::FLOOR, "Coffin"),
            ViewObject(ViewId::OPENED_COFFIN, ViewLayer::FLOOR, "Coffin"),"coffin",
            s.get<CreatureId>(), 1,
            "There is a rotting corpse inside. You find an item.",
            "There is a rotting corpse inside. The corpse is alive!",
            "There is a rotting corpse inside. You find some gold.", ItemFactory::chest());
    case SquareId::CEMETERY:
        return new Square(ViewObject(ViewId::CEMETERY, ViewLayer::FLOOR_BACKGROUND, "Cemetery"),
          CONSTRUCT(Square::Params,
            c.name = "floor";
            c.canDestroy = true;
            c.constructions[SquareId::GRAVE] = 10;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.vision = VisionId::NORMAL;));
    case SquareId::GRAVE:
        return new Grave(ViewObject(ViewId::GRAVE, ViewLayer::FLOOR, "Grave"), "grave");
    case SquareId::DOOR:
        return new Door(ViewObject(ViewId::DOOR, ViewLayer::FLOOR, "Door")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW),
          CONSTRUCT(Square::Params,
            c.name = "door";
            c.canHide = true;
            c.movementSet = MovementSet().addTrait(MovementTrait::WALK);
            c.strength = 100;
            c.canDestroy = true;
            c.flamability = 1;));
    case SquareId::TRIBE_DOOR:
        return new TribeDoor(ViewObject(ViewId::DOOR, ViewLayer::FLOOR, "Door - right click to lock.")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), s.get<const Tribe*>(), 100,
          CONSTRUCT(Square::Params,
            c.name = "door";
            c.canHide = true;
            c.strength = 100;
            c.flamability = 1;
            c.canDestroy = true;
            c.owner = s.get<const Tribe*>();));
    case SquareId::BARRICADE:
        return new Barricade(ViewObject(ViewId::BARRICADE, ViewLayer::FLOOR, "Barricade")
            .setModifier(ViewObject::Modifier::ROUND_SHADOW), s.get<const Tribe*>(), 200);
    case SquareId::BORDER_GUARD:
        return new Square(ViewObject(ViewId::BORDER_GUARD, ViewLayer::FLOOR, "Wall"),
          CONSTRUCT(Square::Params, c.name = "wall";));
    case SquareId::DOWN_STAIRS:
    case SquareId::UP_STAIRS: FAIL << "Stairs are not handled by this method.";
  }
  return 0;
}

PSquare SquareFactory::getStairs(StairDirection direction, StairKey key, StairLook look) {
  ViewId id1 = ViewId(0), id2 = ViewId(0);
  switch (look) {
    case StairLook::NORMAL: id1 = ViewId::UP_STAIRCASE; id2 = ViewId::DOWN_STAIRCASE; break;
    case StairLook::HELL: id1 = ViewId::UP_STAIRCASE_HELL; id2 = ViewId::DOWN_STAIRCASE_HELL; break;
    case StairLook::CELLAR: id1 = ViewId::UP_STAIRCASE_CELLAR; id2 = ViewId::DOWN_STAIRCASE_CELLAR; break;
    case StairLook::PYRAMID: id1 = ViewId::UP_STAIRCASE_PYR; id2 = ViewId::DOWN_STAIRCASE_PYR; break;
    case StairLook::DUNGEON_ENTRANCE: id1 = id2 = ViewId::DUNGEON_ENTRANCE; break;
    case StairLook::DUNGEON_ENTRANCE_MUD: id1 = id2 = ViewId::DUNGEON_ENTRANCE_MUD; break;
  }
  switch (direction) {
    case StairDirection::UP:
        return PSquare(new Staircase(ViewObject(id1, ViewLayer::FLOOR, "Stairs leading up"),
            "stairs leading up", direction, key));
    case StairDirection::DOWN:
        return PSquare(new Staircase(ViewObject(id2, ViewLayer::FLOOR, "Stairs leading down"),
            "stairs leading down", direction, key));
  }
  return nullptr;
}
  
PSquare SquareFactory::getWater(double depth) {
  return PSquare(new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR_BACKGROUND, "Water"), "water", depth));
}
