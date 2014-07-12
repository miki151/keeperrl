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

class Staircase : public Square {
  public:
  Staircase(const ViewObject& obj, const string& name, StairDirection dir, StairKey key)
      : Square(obj, name, Vision::get(VisionId::NORMAL), true, 10000) {
    setLandingLink(dir, key);
  }

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage("There are " + getName() + " here.");
  }

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const override {
    switch (getLandingLink()->first) {
      case StairDirection::DOWN: return SquareApplyType::DESCEND;
      case StairDirection::UP: return SquareApplyType::ASCEND;
    }
    return Nothing();
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

class SecretPassage : public Square {
  public:
  SecretPassage(const ViewObject& obj, const ViewObject& sec)
    : Square(obj, "secret door", nullptr), secondary(sec), uncovered(false) {
  }

  void uncover(Vec2 pos) {
    uncovered = true;
    setName("floor");
    setViewObject(secondary);
    setVision(Vision::get(VisionId::NORMAL));
    getLevel()->updateVisibility(pos);
  }

  virtual bool canDestroy() const override {
    return true;
  }

  virtual void destroy() override {
    if (uncovered)
      return;
    if (getLevel()->playerCanSee(getPosition())) {
      getLevel()->globalMessage(getPosition(), "A secret passage is destroyed!");
      uncover(getPosition());
    }
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (uncovered)
      return;
    if (c->isPlayer()) {
      c->playerMessage("You found a secret passage!");
      uncover(c->getPosition());
    } else 
    if (getLevel()->playerCanSee(c->getPosition())) {
      getLevel()->globalMessage(getPosition(), c->getTheName() + " uncovers a secret passage!");
      uncover(c->getPosition());
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(secondary)
      & SVAR(uncovered);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(SecretPassage);

  private:
  ViewObject SERIAL(secondary);
  bool SERIAL(uncovered);
};

class Magma : public Square {
  public:
  Magma(const ViewObject& object, const string& name, const string& itemMsg, const string& noSee)
      : Square(object, name, Vision::get(VisionId::NORMAL), false, 0, 0, {{SquareType::BRIDGE, 20}}),
    itemMessage(itemMsg), noSeeMsg(noSee) {}

  virtual bool canEnterSpecial(const Creature* c) const override {
    return c->isAffected(LastingEffect::FLYING) || c->isBlind() || c->isHeld();
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (!c->isAffected(LastingEffect::FLYING)) {
      c->you(MsgType::BURN, getName());
      c->die(nullptr, false);
    }
  }

  virtual void dropItem(PItem item) override {
    getLevel()->globalMessage(getPosition(), item->getTheName() + " " + itemMessage, noSeeMsg);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(itemMessage)
      & SVAR(noSeeMsg);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Magma);

  private:
  string SERIAL(itemMessage);
  string SERIAL(noSeeMsg);
};

class Water : public Square {
  public:
  Water(ViewObject object, const string& name, const string& itemMsg, const string& noSee, double _depth)
      : Square(object.setAttribute(ViewObject::Attribute::WATER_DEPTH, _depth),
          name, Vision::get(VisionId::NORMAL), false, 0, 0, {{SquareType::BRIDGE, 20}}),
        itemMessage(itemMsg), noSeeMsg(noSee), depth(_depth) {}

  bool canWalk(const Creature* c) const {
    switch (c->getSize()) {
      case CreatureSize::HUGE: return depth < 3;
      case CreatureSize::LARGE: return depth < 1.5;
      case CreatureSize::MEDIUM: return depth < 1;
      case CreatureSize::SMALL: return depth < 0.3;
    }
    return false;
  }

  virtual bool canEnterSpecial(const Creature* c) const override {
    bool can = canWalk(c) || c->canSwim() || c->isAffected(LastingEffect::FLYING) || c->isBlind() || c->isHeld();
 /*   if (!can)
      c->playerMessage("The water is too deep.");*/
    return can;
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (!c->isAffected(LastingEffect::FLYING) && !c->canSwim() && !canWalk(c)) {
      c->you(MsgType::DROWN, getName());
      c->die(nullptr, false);
    }
  }

  virtual void dropItem(PItem item) override {
    getLevel()->globalMessage(getPosition(), item->getTheName() + " " + itemMessage, noSeeMsg);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(itemMessage)
      & SVAR(noSeeMsg) 
      & SVAR(depth);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Water);
  
  private:
  string SERIAL(itemMessage);
  string SERIAL(noSeeMsg);
  double SERIAL(depth);
};

class Chest : public Square {
  public:
  Chest(const ViewObject& object, const ViewObject& opened, const string& name, CreatureId id, int minC, int maxC,
        const string& _msgItem, const string& _msgMonster, const string& _msgGold,
        ItemFactory _itemFactory) : 
      Square(object, name, Vision::get(VisionId::NORMAL), true, 30, 0.5), creatureId(id), minCreatures(minC), maxCreatures(maxC), msgItem(_msgItem), msgMonster(_msgMonster), msgGold(_msgGold), itemFactory(_itemFactory), openedObject(opened) {}

  virtual void onEnterSpecial(Creature* c) override {
    c->playerMessage(string("There is a ") + (opened ? " opened " : "") + getName() + " here");
  }

  virtual bool canDestroy() const override {
    return true;
  }

  virtual void onConstructNewSquare(Square* s) override {
    if (opened)
      return;
    vector<PItem> item;
    if (!Random.roll(10))
      append(item, itemFactory.random());
    else {
      for (int i : Range(Random.getRandom(minCreatures, maxCreatures)))
        item.push_back(ItemFactory::corpse(creatureId));
    }
    s->dropItems(std::move(item));
  }

  virtual Optional<SquareApplyType> getApplyType(const Creature* c) const override { 
    if (opened || !c->isHumanoid()) 
      return Nothing();
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
      EventListener::addItemsAppearedEvent(getLevel(), getPosition(), extractRefs(items));
      c->takeItems(std::move(items), nullptr);
    } else {
      c->playerMessage(msgMonster);
      int numR = Random.getRandom(minCreatures, maxCreatures);
      for (Vec2 v : getPosition().neighbors8(true)) {
        PCreature rat = CreatureFactory::fromId(creatureId,  Tribe::get(TribeId::PEST));
        if (getLevel()->getSquare(v)->canEnter(rat.get())) {
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
      & SVAR(creatureId)
      & SVAR(minCreatures)
      & SVAR(maxCreatures) 
      & SVAR(msgItem)
      & SVAR(msgMonster)
      & SVAR(msgGold)
      & SVAR(opened)
      & SVAR(itemFactory)
      & SVAR(openedObject);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Chest);

  private:
  CreatureId SERIAL(creatureId);
  int SERIAL(minCreatures);
  int SERIAL(maxCreatures);
  string SERIAL(msgItem);
  string SERIAL(msgMonster);
  string SERIAL(msgGold);
  bool SERIAL2(opened, false);
  ItemFactory SERIAL(itemFactory);
  ViewObject SERIAL(openedObject);
};

class Fountain : public Square {
  public:
  Fountain(const ViewObject& object) : Square(object, "fountain", Vision::get(VisionId::NORMAL), true, 100) {}

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const override { 
    return SquareApplyType::DRINK;
  }

  virtual bool canDestroy() const override {
    return true;
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
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Fountain);

  private:
  int SERIAL2(seed, Random.getRandom(123456));
};

class Tree : public Square {
  public:
  Tree(const ViewObject& object, const string& name, Vision* vision, int _numWood,
      map<SquareType::Id, int> construct)
      : Square(object, name, vision, true, 100, 0.5, construct), numWood(_numWood) {}

  virtual bool canDestroy() const override {
    return true;
  }

  virtual void destroy() override {
    if (destroyed)
      return;
    getLevel()->globalMessage(getPosition(), "The tree falls.");
    destroyed = true;
    setVision(Vision::get(VisionId::NORMAL));
    getLevel()->updateVisibility(getPosition());
    setViewObject(ViewObject(ViewId::FALLEN_TREE, ViewLayer::FLOOR, "Fallen tree"));
  }

  virtual void onConstructNewSquare(Square* s) override {
    s->dropItems(ItemFactory::fromId(ItemId::WOOD_PLANK, numWood));
  }

  virtual void burnOut() override {
    setVision(Vision::get(VisionId::NORMAL));
    getLevel()->updateVisibility(getPosition());
    setViewObject(ViewObject(ViewId::BURNT_TREE, ViewLayer::FLOOR, "Burnt tree"));
  }

  virtual void onEnterSpecial(Creature* c) override {
 /*   c->playerMessage(isBurnt() ? "There is a burnt tree here." : 
        destroyed ? "There is fallen tree here." : "You pass beneath a tree");*/
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(destroyed)
      & SVAR(numWood);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Tree);

  private:
  bool SERIAL2(destroyed, false);
  int SERIAL(numWood);
};

class TrapSquare : public Square {
  public:
  TrapSquare(const ViewObject& object, EffectType e) : Square(object, "floor", Vision::get(VisionId::NORMAL)), effect(e) {
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
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(TrapSquare);

  private:
  bool SERIAL2(active, true);
  EffectType SERIAL(effect);
};

class Door : public Square {
  public:
  Door(const ViewObject& object) : Square(object, "door", nullptr, true, 100, 1) {}

  virtual bool canDestroy() const override {
    return true;
  }

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
  TribeDoor(const ViewObject& object, int destStrength) : Door(object), destructionStrength(destStrength) {
  }

  virtual void destroy(const Creature* c) override {
    destructionStrength -= c->getAttr(AttrType::STRENGTH);
    if (destructionStrength <= 0) {
      Door::destroy(c);
    }
  }

  virtual bool canDestroyBy(const Creature* c) const override {
    return c->getTribe() != Tribe::get(TribeId::KEEPER)
      || c->isInvincible(); // hack to make boulders destroy doors
  }

  virtual bool canEnterSpecial(const Creature* c) const override {
    return !locked && c->canWalk() && c->getTribe() == Tribe::get(TribeId::KEEPER);
  }

  virtual bool canLock() const {
    return true;
  }

  virtual bool isLocked() const {
    return locked;
  }

  virtual void lock() {
    locked = !locked;
    if (locked)
      modViewObject().setModifier(ViewObject::Modifier::LOCKED);
    else
      modViewObject().removeModifier(ViewObject::Modifier::LOCKED);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Door)
      & SVAR(destructionStrength)
      & SVAR(locked);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(TribeDoor);

  private:
  int SERIAL(destructionStrength);
  bool SERIAL2(locked, false);
};

class Barricade : public SolidSquare {
  public:
  Barricade(const ViewObject& object, int destStrength)
    : SolidSquare(object, "barricade", Vision::get(VisionId::NORMAL), {}, false, 0.5),
    destructionStrength(destStrength) {
  }

  virtual void destroy(const Creature* c) override {
    destructionStrength -= c->getAttr(AttrType::STRENGTH);
    if (destructionStrength <= 0) {
      SolidSquare::destroy(c);
    }
  }

  virtual bool canDestroyBy(const Creature* c) const override {
    return c->getTribe() != Tribe::get(TribeId::KEEPER)
      || c->isInvincible(); // hack to make boulders destroy doors
  }

  virtual bool canDestroy() const override {
    return true;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(SolidSquare)
      & SVAR(destructionStrength);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Barricade);

  private:
  int SERIAL(destructionStrength);
};

class Furniture : public Square {
  public:
  Furniture(ViewObject object, const string& name, double flamability,
      Optional<SquareApplyType> _applyType = Nothing()) 
      : Square(object.setModifier(ViewObject::Modifier::ROUND_SHADOW), name, Vision::get(VisionId::NORMAL),
        true, 100, flamability), applyType(_applyType) {}

  virtual bool canDestroy() const override {
    return true;
  }

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const override {
    return applyType;
  }

  virtual void onApply(Creature* c) {}

  virtual void onEnterSpecial(Creature* c) override {
   // c->playerMessage("There is a " + getName() + " here.");
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square)
      & SVAR(applyType);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Furniture);
  
  private:
  Optional<SquareApplyType> SERIAL(applyType);
};

class DestroyableSquare : public Square {
  public:
  using Square::Square;

  virtual bool canDestroy() const override {
    return true;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Square);
  }
};

class Bed : public Furniture {
  public:
  Bed(const ViewObject& object, const string& name, double flamability = 1) : Furniture(object, name, flamability) {}

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const override { 
    return SquareApplyType::SLEEP;
  }

  virtual void onApply(Creature* c) override {
    Effect::applyToCreature(c, EffectType::SLEEP, EffectStrength::STRONG);
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

  virtual Optional<SquareApplyType> getApplyType(const Creature* c) const override { 
    if (c->isUndead())
      return SquareApplyType::SLEEP;
    else
      return Nothing();
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

class Altar : public Square, public EventListener {
  public:
  Altar(const ViewObject& object)
      : Square(object, "shrine", Vision::get(VisionId::NORMAL) , true, 100, 0) {
  }

  virtual bool canDestroy() const override {
    return true;
  }

  virtual Optional<SquareApplyType> getApplyType(const Creature* c) const override { 
    if (c->isHumanoid())
      return SquareApplyType::PRAY;
    else
      return Nothing();
  }

  virtual const Level* getListenerLevel() const override {
    return getLevel();
  }

  virtual void onKillEvent(const Creature* victim, const Creature* killer) override {
    if (victim->getPosition() == getPosition()) {
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
      for (Item* it : getItems(Item::typePredicate(ItemType::CORPSE)))
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
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Altar);

  private:
  const Creature* SERIAL2(recentKiller, nullptr);
  const Creature* SERIAL2(recentVictim, nullptr);
  double SERIAL2(killTime, -100);
  const double sacrificeTimeout = 50;
};

class DeityAltar : public Altar {
  public:
  DeityAltar(const ViewObject& object, Deity* d) : Altar(object), deity(d) {
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

  virtual void onPrayer(Creature* c) override {
    EventListener::addWorshipEvent(c, deity, WorshipType::PRAYER);
  }

  virtual void onSacrifice(Creature* c) override {
    EventListener::addWorshipEvent(c, deity, WorshipType::SACRIFICE);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Altar)
      & SVAR(deity);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(DeityAltar);

  private:
  Deity* SERIAL(deity);
};

class CreatureAltar : public Altar {
  public:
  CreatureAltar(const ViewObject& object, const Creature* c) : Altar(object), creature(c) {
  }

  virtual void onEnterSpecial(Creature* c) override {
    if (c->isHumanoid()) {
      c->playerMessage("This is a shrine to " + creature->getName());
      c->playerMessage(creature->getDescription());
    }
  }

  virtual string getName() override {
    return creature->getName();
  }

  virtual void onPrayer(Creature* c) override {
    EventListener::addWorshipCreatureEvent(c, creature, WorshipType::PRAYER);
  }

  virtual void onSacrifice(Creature* c) override {
    EventListener::addWorshipCreatureEvent(c, creature, WorshipType::SACRIFICE);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Altar)
      & SVAR(creature);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(CreatureAltar);

  private:
  const Creature* SERIAL(creature);
};

class ConstructionDropItems : public SolidSquare {
  public:
  ConstructionDropItems(const ViewObject& object, const string& name,
      map<SquareType::Id, int> constructions, vector<PItem> _items)
      : SolidSquare(object, name, nullptr, constructions), items(std::move(_items)) {}

  virtual void onConstructNewSquare(Square* s) override {
    s->dropItems(std::move(items));
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(SolidSquare)
      & SVAR(items);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(ConstructionDropItems);

  private:
  vector<PItem> SERIAL(items);
};

class TrainingDummy : public Furniture {
  public:
  TrainingDummy(const ViewObject& object, const string& name) : Furniture(object, name, 1) {}

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const override { 
    return SquareApplyType::TRAIN;
  }

  virtual void onApply(Creature* c) override {
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Furniture);
  }

  SERIALIZATION_CONSTRUCTOR(TrainingDummy);
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

class Workshop : public Furniture {
  public:
  using Furniture::Furniture;

  virtual Optional<SquareApplyType> getApplyType(const Creature*) const override { 
    return SquareApplyType::WORKSHOP;
  }

  virtual void onApply(Creature* c) override {
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Furniture);
  }
};

class Hatchery : public Square {
  public:
  Hatchery(const ViewObject& object, const string& name)
    : Square(object, name, Vision::get(VisionId::NORMAL), false, 0, 0, {}, true) {}

  virtual void tickSpecial(double time) override {
    if (getCreature() || !Random.roll(10))
      return;
    for (Vec2 v : getPosition().neighbors8())
      if (Creature* c = getLevel()->getSquare(v)->getCreature())
        if (c->getName() == "chicken")
          return;
    getLevel()->addCreature(getPosition(), CreatureFactory::fromId(CreatureId::CHICKEN,
          Tribe::get(TribeId::PEACEFUL), MonsterAIFactory::moveRandomly()));
  }

  virtual bool canEnterSpecial(const Creature* c) const override {
    return c->canWalk() || c->getName() == "chicken" || c->getName() == "pig";
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Square);
  }
  
  SERIALIZATION_CONSTRUCTOR(Hatchery);
};

class Laboratory : public Workshop {
  public:
  using Workshop::Workshop;

  virtual void onApply(Creature* c) override {
    c->playerMessage("You mix the concoction.");
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Workshop);
  }
};

PSquare SquareFactory::getAltar(Deity* deity) {
  return PSquare(new DeityAltar(ViewObject(ViewId::ALTAR, ViewLayer::FLOOR, "Shrine"), deity));
}

PSquare SquareFactory::getAltar(Creature* creature) {
  return PSquare(new CreatureAltar(ViewObject(ViewId::ALTAR, ViewLayer::FLOOR, "Shrine"), creature));
}

template <class Archive>
void SquareFactory::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Laboratory);
  REGISTER_TYPE(ar, Staircase);
  REGISTER_TYPE(ar, SecretPassage);
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
  REGISTER_TYPE(ar, DestroyableSquare);
  REGISTER_TYPE(ar, Grave);
  REGISTER_TYPE(ar, DeityAltar);
  REGISTER_TYPE(ar, CreatureAltar);
  REGISTER_TYPE(ar, ConstructionDropItems);
  REGISTER_TYPE(ar, TrainingDummy);
  REGISTER_TYPE(ar, Workshop);
  REGISTER_TYPE(ar, Hatchery);
}

REGISTER_TYPES(SquareFactory);

PSquare SquareFactory::get(SquareType s) {
  return PSquare(getPtr(s));
}

Square* SquareFactory::getPtr(SquareType s) {
  switch (s.id) {
    case SquareType::PATH:
    case SquareType::FLOOR:
        return new Square(ViewObject(ViewId::PATH, ViewLayer::FLOOR_BACKGROUND, "Floor"), "floor", Vision::get(VisionId::NORMAL),
            false, 0, 0, 
            {{SquareType::TREASURE_CHEST, 10}, {SquareType::DORM, 10}, {SquareType::TRIBE_DOOR, 10},
            {SquareType::TRAINING_ROOM, 10}, {SquareType::LIBRARY, 10},
            {SquareType::STOCKPILE, 1}, {SquareType::STOCKPILE_EQUIP, 1}, {SquareType::STOCKPILE_RES, 1},
            {SquareType::CEMETERY, 10}, {SquareType::WORKSHOP, 10}, {SquareType::PRISON, 10},
            {SquareType::TORTURE_TABLE, 10}, {SquareType::LABORATORY, 10}, {SquareType::BEAST_LAIR, 10},
            {SquareType::IMPALED_HEAD, 5}, {SquareType::BARRICADE, 20}, {SquareType::TORCH, 5},
            {SquareType::ALTAR, 35}, {SquareType::CREATURE_ALTAR, 35}});
    case SquareType::BRIDGE:
        return new Square(ViewObject(ViewId::BRIDGE, ViewLayer::FLOOR,"Rope bridge"), "rope bridge",
            Vision::get(VisionId::NORMAL));
    case SquareType::GRASS:
        return new Square(ViewObject(ViewId::GRASS, ViewLayer::FLOOR_BACKGROUND, "Grass"), "grass",
            Vision::get(VisionId::NORMAL), false, 0, 0, {{SquareType::IMPALED_HEAD, 5}});
    case SquareType::CROPS:
        return new Square(ViewObject(ViewId::CROPS, ViewLayer::FLOOR_BACKGROUND, "Potatoes"),
            "potatoes", Vision::get(VisionId::NORMAL), false, 0, 0);
    case SquareType::MUD:
        return new Square(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND, "Mud"), "mud", Vision::get(VisionId::NORMAL));
    case SquareType::ROAD:
        return new Square(ViewObject(ViewId::ROAD, ViewLayer::FLOOR, "Road"), "road", Vision::get(VisionId::NORMAL));
    case SquareType::ROCK_WALL:
        return new SolidSquare(ViewObject(ViewId::WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall", nullptr,
            {{SquareType::FLOOR, Random.getRandom(3, 8)}});
    case SquareType::GOLD_ORE:
        return new ConstructionDropItems(ViewObject(ViewId::GOLD_ORE, ViewLayer::FLOOR, "Gold ore")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "gold ore",
            {{SquareType::FLOOR, Random.getRandom(30, 80)}},
            ItemFactory::fromId(ItemId::GOLD_PIECE, Random.getRandom(15, 30)));
    case SquareType::IRON_ORE:
        return new ConstructionDropItems(ViewObject(ViewId::IRON_ORE, ViewLayer::FLOOR, "Iron ore")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "iron ore",
            {{SquareType::FLOOR, Random.getRandom(15, 40)}},
            ItemFactory::fromId(ItemId::IRON_ORE, Random.getRandom(12, 27)));
    case SquareType::STONE:
        return new ConstructionDropItems(ViewObject(ViewId::STONE, ViewLayer::FLOOR, "Stone")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "stone",
            {{SquareType::FLOOR, Random.getRandom(30, 80)}},
            ItemFactory::fromId(ItemId::ROCK, Random.getRandom(5, 20)));
    case SquareType::LOW_ROCK_WALL:
        return new SolidSquare(ViewObject(ViewId::LOW_ROCK_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall");
    case SquareType::WOOD_WALL:
        return new SolidSquare(ViewObject(ViewId::WOOD_WALL, ViewLayer::FLOOR, "Wooden wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall", nullptr,{}, false, 0.4);
    case SquareType::BLACK_WALL:
        return new SolidSquare(ViewObject(ViewId::BLACK_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall");
    case SquareType::YELLOW_WALL:
        return new SolidSquare(ViewObject(ViewId::YELLOW_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall");
    case SquareType::HELL_WALL:
        return new SolidSquare(ViewObject(ViewId::HELL_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall");
    case SquareType::CASTLE_WALL:
        return new SolidSquare(ViewObject(ViewId::CASTLE_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall");
    case SquareType::MUD_WALL:
        return new SolidSquare(ViewObject(ViewId::MUD_WALL, ViewLayer::FLOOR, "Wall")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "wall");
    case SquareType::MOUNTAIN:
        return new SolidSquare(ViewObject(ViewId::MOUNTAIN, ViewLayer::FLOOR, "Mountain"), "mountain",
            Vision::get(VisionId::NORMAL));
    case SquareType::MOUNTAIN2:
        return new SolidSquare(ViewObject(ViewId::WALL, ViewLayer::FLOOR, "Mountain")
            .setModifier(ViewObject::Modifier::CASTS_SHADOW), "mountain", nullptr,
            {{SquareType::FLOOR, Random.getRandom(3, 8)}});
    case SquareType::GLACIER:
        return new SolidSquare(ViewObject(ViewId::SNOW, ViewLayer::FLOOR, "Mountain"), "mountain",
            Vision::get(VisionId::NORMAL));
    case SquareType::HILL:
        return new Square(ViewObject(ViewId::HILL, ViewLayer::FLOOR_BACKGROUND, "Hill"), "hill",
            Vision::get(VisionId::NORMAL), false, 0, 0, { {SquareType::IMPALED_HEAD, 5}});
    case SquareType::SECRET_PASS:
        return new SecretPassage(ViewObject(ViewId::SECRETPASS, ViewLayer::FLOOR, "Wall"),
                                 ViewObject(ViewId::FLOOR, ViewLayer::FLOOR, "Floor"));
    case SquareType::WATER:
        return new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR, "Water"), "water",
            "sinks in the water", "You hear a splash", 100);
    case SquareType::MAGMA: 
        return new Magma(ViewObject(ViewId::MAGMA, ViewLayer::FLOOR, "Magma"),
            "magma", "burns in the magma", "");
    case SquareType::ABYSS: 
        FAIL << "Unimplemented";
    case SquareType::SAND:
        return new Square(ViewObject(ViewId::SAND, ViewLayer::FLOOR_BACKGROUND, "Sand"),
            "sand", Vision::get(VisionId::NORMAL));
    case SquareType::CANIF_TREE:
        return new Tree(ViewObject(ViewId::CANIF_TREE, ViewLayer::FLOOR, "Tree"),
            "tree", Vision::get(VisionId::ELF),
            Random.getRandom(15, 30), {{SquareType::TREE_TRUNK, 20}});
    case SquareType::DECID_TREE:
        return new Tree(ViewObject(ViewId::DECID_TREE, ViewLayer::FLOOR, "Tree"),
            "tree", Vision::get(VisionId::ELF), Random.getRandom(15, 30), {{SquareType::TREE_TRUNK, 20}});
    case SquareType::BUSH:
        return new Tree(ViewObject(ViewId::BUSH, ViewLayer::FLOOR, "Bush"), "bush",
            Vision::get(VisionId::NORMAL), Random.getRandom(5, 10), {{SquareType::TREE_TRUNK, 10}});
    case SquareType::TREE_TRUNK:
        return new Square(ViewObject(ViewId::TREE_TRUNK, ViewLayer::FLOOR, "tree trunk"),
            "tree trunk", Vision::get(VisionId::NORMAL));
    case SquareType::BED: return new Bed(ViewObject(ViewId::BED, ViewLayer::FLOOR, "Bed"), "bed");
    case SquareType::DORM: return new DestroyableSquare(ViewObject(ViewId::DORM, ViewLayer::FLOOR_BACKGROUND,
                              "Dormitory"), "floor", Vision::get(VisionId::NORMAL));
    case SquareType::TORCH: return new Torch(ViewObject(ViewId::TORCH, ViewLayer::FLOOR, "Torch"), "torch");
    case SquareType::STOCKPILE:
        return new DestroyableSquare(ViewObject(ViewId::STOCKPILE1, ViewLayer::FLOOR_BACKGROUND, "Storage (all)"),
            "floor", Vision::get(VisionId::NORMAL));
    case SquareType::STOCKPILE_EQUIP:
        return new DestroyableSquare(ViewObject(ViewId::STOCKPILE2, ViewLayer::FLOOR_BACKGROUND,
              "Storage (equipment)"), "floor", Vision::get(VisionId::NORMAL));
    case SquareType::STOCKPILE_RES:
        return new DestroyableSquare(ViewObject(ViewId::STOCKPILE3, ViewLayer::FLOOR_BACKGROUND,
              "Storage (resources)"), "floor", Vision::get(VisionId::NORMAL));
    case SquareType::PRISON:
        return new DestroyableSquare(ViewObject(ViewId::PRISON, ViewLayer::FLOOR_BACKGROUND, "Prison"),
            "floor", Vision::get(VisionId::NORMAL));
    case SquareType::WELL:
        return new Furniture(ViewObject(ViewId::WELL, ViewLayer::FLOOR, "Well"), 
            "well", 0);
    case SquareType::STATUE:
        return new Furniture(
            ViewObject(chooseRandom({ViewId::STATUE1, ViewId::STATUE2}), ViewLayer::FLOOR, "Statue"), "statue", 0);
    case SquareType::TORTURE_TABLE:
        return new Furniture(ViewObject(ViewId::TORTURE_TABLE, ViewLayer::FLOOR, "Torture table"), 
            "torture table", 0.3, SquareApplyType::TORTURE);
    case SquareType::BEAST_CAGE:
        return new Furniture(ViewObject(ViewId::BEAST_CAGE, ViewLayer::FLOOR, "Beast cage"), "beast cage", 0.3);
    case SquareType::BEAST_LAIR: return new DestroyableSquare(ViewObject(ViewId::BEAST_LAIR,
                              ViewLayer::FLOOR_BACKGROUND, "Beast lair"), "floor", Vision::get(VisionId::NORMAL));
    case SquareType::TRAINING_ROOM:
        return new TrainingDummy(ViewObject(ViewId::TRAINING_ROOM, ViewLayer::FLOOR, "Training post"), 
            "training post");
    case SquareType::IMPALED_HEAD:
        return new Square(ViewObject(ViewId::IMPALED_HEAD, ViewLayer::FLOOR, "Impaled head")
            .setModifier(ViewObject::Modifier::ROUND_SHADOW), "impaled head", Vision::get(VisionId::NORMAL));
    case SquareType::LIBRARY:
        return new TrainingDummy(ViewObject(ViewId::LIBRARY, ViewLayer::FLOOR, "Book shelf"), 
            "book shelf");
    case SquareType::LABORATORY: return new Laboratory(ViewObject(ViewId::LABORATORY, ViewLayer::FLOOR, "cauldron"),
                                   "cauldron", 0);
    case SquareType::WORKSHOP:
        return new Workshop(ViewObject(ViewId::WORKSHOP, ViewLayer::FLOOR, "Workshop stand"), 
            "workshop stand", 1);
    case SquareType::HATCHERY:
        return new Hatchery(ViewObject(ViewId::MUD, ViewLayer::FLOOR_BACKGROUND, "Hatchery"), "hatchery");
    case SquareType::ALTAR:
        return new DeityAltar(ViewObject(ViewId::ALTAR, ViewLayer::FLOOR, "Shrine"),
              Deity::getDeity(s.altarInfo.habitat));
    case SquareType::CREATURE_ALTAR:
        return new CreatureAltar(ViewObject(ViewId::CREATURE_ALTAR, ViewLayer::FLOOR, "Shrine"),
              s.creatureAltarInfo.creature);
    case SquareType::ROLLING_BOULDER: return new TrapSquare(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR, "floor"),
                                          EffectType::ROLLING_BOULDER);
    case SquareType::POISON_GAS: return new TrapSquare(ViewObject(ViewId::FLOOR, ViewLayer::FLOOR, "floor"),
                                          EffectType::EMIT_POISON_GAS);
    case SquareType::FOUNTAIN:
        return new Fountain(ViewObject(ViewId::FOUNTAIN, ViewLayer::FLOOR, "Fountain"));
    case SquareType::CHEST:
        return new Chest(ViewObject(ViewId::CHEST, ViewLayer::FLOOR, "Chest"), ViewObject(ViewId::OPENED_CHEST, ViewLayer::FLOOR, "Opened chest"), "chest", CreatureId::RAT, 3, 6, "There is an item inside", "It's full of rats!", "There is gold inside", ItemFactory::chest());
    case SquareType::TREASURE_CHEST:
        return new Furniture(ViewObject(ViewId::TREASURE_CHEST, ViewLayer::FLOOR, "Chest"), "chest", 1);
    case SquareType::COFFIN:
        return new Chest(ViewObject(ViewId::COFFIN, ViewLayer::FLOOR, "Coffin"), ViewObject(ViewId::OPENED_COFFIN, ViewLayer::FLOOR, "Coffin"),"coffin", CreatureId::VAMPIRE, 1, 2, "There is a rotting corpse inside. You find an item.", "There is a rotting corpse inside. The corpse is alive!", "There is a rotting corpse inside. You find some gold.", ItemFactory::chest());
    case SquareType::CEMETERY: return new DestroyableSquare(ViewObject(ViewId::CEMETERY, ViewLayer::FLOOR_BACKGROUND,
                              "Cemetery"), "floor", Vision::get(VisionId::NORMAL));
    case SquareType::GRAVE:
        return new Grave(ViewObject(ViewId::GRAVE, ViewLayer::FLOOR, "Grave"), "grave");
    case SquareType::IRON_BARS:
        FAIL << "Unimplemented";
    case SquareType::DOOR: return new Door(ViewObject(ViewId::DOOR, ViewLayer::FLOOR, "Door")
                               .setModifier(ViewObject::Modifier::CASTS_SHADOW));
    case SquareType::TRIBE_DOOR: return new TribeDoor(ViewObject(ViewId::DOOR, ViewLayer::FLOOR,
                                       "Door - click to lock.").setModifier(ViewObject::Modifier::CASTS_SHADOW), 100);
    case SquareType::BARRICADE: return new Barricade(ViewObject(ViewId::BARRICADE, ViewLayer::FLOOR, "Barricade")
                                    .setModifier(ViewObject::Modifier::ROUND_SHADOW), 200);
    case SquareType::BORDER_GUARD:
        return new SolidSquare(ViewObject(ViewId::BORDER_GUARD, ViewLayer::FLOOR, "Wall"), "wall");
    case SquareType::DOWN_STAIRS:
    case SquareType::UP_STAIRS: FAIL << "Stairs are not handled by this method.";
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
  return PSquare(new Water(ViewObject(ViewId::WATER, ViewLayer::FLOOR, "Water"), "water",
      "sinks in the water", "You hear a splash", depth));
}
