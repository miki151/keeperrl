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

#include "item_factory.h"
#include "level.h"
#include "creature_factory.h"
#include "util.h"
#include "ranged_weapon.h"
#include "enemy_check.h"
#include "technology.h"
#include "effect.h"
#include "square.h"
#include "view_object.h"
#include "view_id.h"
#include "trigger.h"

template <class Archive> 
void ItemFactory::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(items)
    & SVAR(weights)
    & SVAR(minCount)
    & SVAR(maxCount)
    & SVAR(unique);
  CHECK_SERIAL;
}

SERIALIZABLE(ItemFactory);

SERIALIZATION_CONSTRUCTOR_IMPL(ItemFactory);

class FireScroll : public Item {
  public:
  FireScroll(const ItemAttributes& attr) : Item(attr) {}

  virtual void apply(Creature* c, Level* l) override {
    if (l->playerCanSee(c->getPosition()))
      identify();
    set = true;
  }

  virtual void specialTick(double time, Level* level, Vec2 position) override {
    if (set) {
      setOnFire(0.03, level, position);
      set = false;
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(set);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(FireScroll);

  private:
  bool SERIAL2(set, false);
};

class AmuletOfWarning : public Item {
  public:
  AmuletOfWarning(const ItemAttributes& attr, int r) : Item(attr), radius(r) {}

  virtual void specialTick(double time, Level* level, Vec2 position) override {
    Creature* owner = level->getSquare(position)->getCreature();
    if (owner && owner->getEquipment().isEquiped(this)) {
      Rectangle rect = Rectangle(position.x - radius, position.y - radius,
          position.x + radius + 1, position.y + radius + 1);
      bool isDanger = false;
      bool isBigDanger = false;
      for (Vec2 v : rect) {
        if (!level->inBounds(v))
          continue;
        for (Trigger* t : level->getSquare(v)->getTriggers())
          if (t->isDangerous(owner)) {
            if (v.dist8(position) <= 1)
              isBigDanger = true;
            else
              isDanger = true;
          }
        if (Creature* c = level->getSquare(v)->getCreature()) {
          if (!owner->canSee(c) && c->isEnemy(owner)) {
            int diff = c->getAttr(AttrType::DAMAGE) - owner->getAttr(AttrType::DAMAGE);
            if (diff > 5)
              isBigDanger = true;
            else
              if (diff > 0)
                isDanger = true;
          }
        }
      }
      if (isBigDanger)
        owner->playerMessage(getTheName() + " vibrates vigorously");
      else
      if (isDanger)
        owner->playerMessage(getTheName() + " vibrates");
    }
  }
 
  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(radius);
    CHECK_SERIAL;
  }
 
  SERIALIZATION_CONSTRUCTOR(AmuletOfWarning);

  private:
  int SERIAL(radius);
};

class AmuletOfHealing : public Item {
  public:
  AmuletOfHealing(const ItemAttributes& attr) : Item(attr) {}

  virtual void specialTick(double time, Level* level, Vec2 position) override {
    Creature* owner = level->getSquare(position)->getCreature();
    if (owner && owner->getEquipment().isEquiped(this)) {
      if (lastTick == -1)
        lastTick = time;
      else {
        owner->heal((time - lastTick) / 20);
      }
      lastTick = time;
    } else 
      lastTick = -1;
  }
 
  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(lastTick);
    CHECK_SERIAL;
  }
 
  SERIALIZATION_CONSTRUCTOR(AmuletOfHealing);

  private:
  double SERIAL2(lastTick, -1);
};

class AmuletOfEnemyCheck : public Item {
  public:
  AmuletOfEnemyCheck(const ItemAttributes& attr, EnemyCheck* c) : Item(attr), check(c) {}

  virtual void onEquipSpecial(Creature* c) {
    c->addEnemyCheck(check);
  }

  virtual void onUnequipSpecial(Creature* c) {
    c->removeEnemyCheck(check);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(check);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(AmuletOfEnemyCheck);

  private:
  EnemyCheck* SERIAL(check);
};

class Telepathy : public CreatureVision {
  public:
  virtual bool canSee(const Creature* c1, const Creature* c2) override {
    return c1->getPosition().dist8(c2->getPosition()) < 5 && c2->hasBrain();
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(CreatureVision);
  }
};

class ItemOfCreatureVision : public Item {
  public:
  ItemOfCreatureVision(const ItemAttributes& attr, CreatureVision* v) : Item(attr), vision(v) {}

  virtual void onEquipSpecial(Creature* c) {
    c->addCreatureVision(vision.get());
  }

  virtual void onUnequipSpecial(Creature* c) {
    c->removeCreatureVision(vision.get());
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(vision);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(ItemOfCreatureVision);

  private:
  unique_ptr<CreatureVision> SERIAL(vision);
};

class LastingEffectItem : public Item {
  public:
  LastingEffectItem(const ItemAttributes& attr, LastingEffect e) : Item(attr), effect(e) {}

  virtual void onEquipSpecial(Creature* c) {
    c->addPermanentEffect(effect);
  }

  virtual void onUnequipSpecial(Creature* c) {
    c->removePermanentEffect(effect);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(effect);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(LastingEffectItem);

  private:
  LastingEffect SERIAL(effect);
};

class Corpse : public Item {
  public:
  Corpse(const ViewObject& obj2, const ItemAttributes& attr, const string& rottenN, 
      double rottingT, Item::CorpseInfo info) : 
      Item(attr), 
      object2(obj2), 
      rottingTime(rottingT), 
      rottenName(rottenN),
      corpseInfo(info) {
  }

  virtual void apply(Creature* c, Level* l) override {
    Item* it = c->getWeapon();
    if (it && it->getAttackType() == AttackType::CUT) {
      c->you(MsgType::DECAPITATE, getTheName());
      setName("decapitated " + getName());
    } else {
      c->playerMessage("You need something sharp to decapitate the corpse.");
    }
  }

  virtual void specialTick(double time, Level* level, Vec2 position) override {
    if (rottenTime == -1)
      rottenTime = time + rottingTime;
    if (time >= rottenTime && !rotten) {
      setName(rottenName);
      setViewObject(object2);
      corpseInfo.isSkeleton = true;
    } else {
      if (!rotten && getWeight() > 10 && Random.roll(20 + (rottenTime - time) / 10))
        Effect::applyToPosition(level, position, EffectType::EMIT_POISON_GAS, EffectStrength::WEAK);
      if (getWeight() > 10 && !corpseInfo.isSkeleton && 
        !level->getCoverInfo(position).covered && Random.roll(35)) {
      for (Vec2 v : position.neighbors8(true))
        if (level->inBounds(v)) {
          PCreature vulture = CreatureFactory::fromId(
              CreatureId::VULTURE, Tribe::get(TribeId::PEST), MonsterAIFactory::scavengerBird(v));
          if (level->getSquare(v)->canEnter(vulture.get())) {
            level->addCreature(v, std::move(vulture));
            level->globalMessage(v, "A vulture lands near " + getTheName());
            rottenTime -= 40;
            break;
          }
        }
      }
    }
  }

  virtual Optional<CorpseInfo> getCorpseInfo() const override { 
    return corpseInfo;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item) 
      & SVAR(object2) 
      & SVAR(rotten)
      & SVAR(rottenTime)
      & SVAR(rottingTime)
      & SVAR(rottenName)
      & SVAR(corpseInfo);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Corpse);

  private:
  ViewObject SERIAL(object2);
  bool SERIAL2(rotten, false);
  double SERIAL2(rottenTime, -1);
  double SERIAL(rottingTime);
  string SERIAL(rottenName);
  CorpseInfo SERIAL(corpseInfo);
};

PItem ItemFactory::corpse(CreatureId id, ItemClass type, Item::CorpseInfo corpseInfo) {
  PCreature c = CreatureFactory::fromId(id, Tribe::get(TribeId::MONSTER));
  return corpse(c->getName() + " corpse", c->getName() + " skeleton", c->getWeight(), type, corpseInfo);
}

PItem ItemFactory::corpse(const string& name, const string& rottenName, double weight, ItemClass itemClass,
    Item::CorpseInfo corpseInfo) {
  const double rotTime = 300;
  return PItem(new Corpse(
        ViewObject(ViewId::BONE, ViewLayer::ITEM, rottenName),
        ITATTR(
          i.viewId = ViewId::BODY_PART;
          i.name = name;
          i.itemClass = itemClass;
          i.weight = weight;),
        rottenName,
        rotTime,
        corpseInfo));
}

class Potion : public Item {
  public:
  Potion(const ItemAttributes& attr) : Item(attr) {}

  virtual void setOnFire(double amount, const Level* level, Vec2 position) override {
    heat += amount;
    Debug() << getName() << " heat " << heat;
    if (heat > 0.1) {
      level->globalMessage(position, getAName() + " boils and explodes!");
      discarded = true;
    }
  }

  virtual void specialTick(double time, Level* level, Vec2 position) override {
    heat = max(0., heat - 0.005);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item) 
      & SVAR(heat);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Potion);

  private:
  double SERIAL2(heat, 0);
};

class SkillBook : public Item {
  public:
  SkillBook(const ItemAttributes& attr, Skill* s) : Item(attr), skill(s) {}

  virtual void apply(Creature* c, Level* l) override {
    c->addSkill(skill);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(skill);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(SkillBook);

  private:
  Skill* SERIAL(skill);
};

class TechBook : public Item {
  public:
  TechBook(const ItemAttributes& attr, Technology* t = nullptr) : Item(attr), tech(t) {}

  virtual void apply(Creature* c, Level* l) override {
    if (!read || tech != nullptr) {
      GlobalEvents.addTechBookEvent(tech);
      read = true;
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(tech)
      & SVAR(read);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(TechBook);

  private:
  Technology* SERIAL2(tech, nullptr);
  bool SERIAL2(read, false);
};

class TrapItem : public Item {
  public:
  TrapItem(ViewObject _trapObject, const ItemAttributes& attr, EffectType _effect)
      : Item(attr), effect(_effect), trapObject(_trapObject) {
  }

  virtual void apply(Creature* c, Level* l) override {
    c->you(MsgType::SET_UP_TRAP, "");
    c->getSquare()->addTrigger(Trigger::getTrap(trapObject, l, c->getPosition(), effect, c->getTribe()));
    discarded = true;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Item)
      & SVAR(effect)
      & SVAR(trapObject);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(TrapItem);

  private:
  EffectType SERIAL(effect);
  ViewObject SERIAL(trapObject);
};

template <class Archive>
void ItemFactory::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, TrapItem);
  REGISTER_TYPE(ar, SkillBook);
  REGISTER_TYPE(ar, TechBook);
  REGISTER_TYPE(ar, Potion);
  REGISTER_TYPE(ar, FireScroll);
  REGISTER_TYPE(ar, AmuletOfWarning);
  REGISTER_TYPE(ar, AmuletOfHealing);
  REGISTER_TYPE(ar, AmuletOfEnemyCheck);
  REGISTER_TYPE(ar, Telepathy);
  REGISTER_TYPE(ar, ItemOfCreatureVision);
  REGISTER_TYPE(ar, Corpse);
  REGISTER_TYPE(ar, LastingEffectItem);
}

REGISTER_TYPES(ItemFactory);

static vector<string> amulet_looks = { "steel", "copper", "crystal", "wooden", "amber"};
static vector<ViewId> amulet_ids = { ViewId::STEEL_AMULET, ViewId::COPPER_AMULET, ViewId::CRYSTAL_AMULET, ViewId::WOODEN_AMULET, ViewId::AMBER_AMULET};

static vector<string> potion_looks = { "effervescent", "murky", "swirly", "violet", "puce", "smoky", "fizzy", "milky" };
static vector<ViewId> potion_ids = { ViewId::EFFERVESCENT_POTION, ViewId::MURKY_POTION, ViewId::SWIRLY_POTION, ViewId::VIOLET_POTION, ViewId::PUCE_POTION, ViewId::SMOKY_POTION, ViewId::FIZZY_POTION, ViewId::MILKY_POTION};
static vector<string> scroll_looks;

ItemFactory::ItemFactory(const vector<ItemInfo>& _items,
      const vector<ItemType>& _unique) : unique(_unique) {
  for (auto elem : _items)
    addItem(elem);
}

ItemFactory& ItemFactory::addItem(ItemInfo elem) {
  items.push_back(elem.id);
  weights.push_back(elem.weight);
  minCount.push_back(elem.minCount);
  maxCount.push_back(elem.maxCount);
  return *this;
}

ItemFactory& ItemFactory::addUniqueItem(ItemType id) {
  unique.push_back(id);
  return *this;
}

vector<PItem> ItemFactory::random(Optional<int> seed) {
  if (unique.size() > 0) {
    ItemType id = unique.back();
    unique.pop_back();
    return fromId(id, 1);
  }
  int index;
  if (seed) {
    RandomGen gen;
    gen.init(*seed);
    index = gen.getRandom(weights);
  } else
    index = Random.getRandom(weights);
  return fromId(items[index], Random.getRandom(minCount[index], maxCount[index]));
}

vector<PItem> ItemFactory::getAll() {
  vector<PItem> ret;
  for (ItemType id : unique)
    ret.push_back(fromId(id));
  for (ItemType id : items)
    ret.push_back(fromId(id));
  return ret;
}

ItemFactory ItemFactory::villageShop() {
  return ItemFactory({
      {{ItemId::SCROLL, EffectType::TELEPORT}, 5 },
      {{ItemId::SCROLL, EffectType::PORTAL}, 5 },
      {{ItemId::SCROLL, EffectType::IDENTIFY}, 5 },
      {ItemId::FIRE_SCROLL, 5 },
      {{ItemId::SCROLL, EffectType::FIRE_SPHERE_PET}, 5 },
      {{ItemId::SCROLL, EffectType::WORD_OF_POWER}, 1 },
      {{ItemId::SCROLL, EffectType::DECEPTION}, 2 },
      {{ItemId::SCROLL, EffectType::SUMMON_INSECTS}, 5 },
      {{ItemId::POTION, EffectType::HEAL}, 7 },
      {{ItemId::POTION, EffectType::SLEEP}, 5 },
      {{ItemId::POTION, EffectType::SLOW}, 5 },
      {{ItemId::POTION, EffectType::SPEED},5 },
      {{ItemId::POTION, EffectType::BLINDNESS}, 5 },
      {{ItemId::POTION, EffectType::INVISIBLE}, 2 },
      {ItemId::WARNING_AMULET, 0.5 },
      {ItemId::HEALING_AMULET, 0.5 },
      {ItemId::DEFENSE_AMULET, 0.5 },
      {ItemId::FRIENDLY_ANIMALS_AMULET, 0.5},
      {{ItemId::RING, LastingEffect::POISON_RESISTANT}, 0.5},
      {{ItemId::RING, LastingEffect::FIRE_RESISTANT}, 0.5},
      {ItemId::FIRST_AID_KIT, 5},
      {ItemId::SPEED_BOOTS, 2},
      {ItemId::LEVITATION_BOOTS, 2},
      {ItemId::TELEPATHY_HELM, 2}});
}

ItemFactory ItemFactory::dwarfShop() {
  return armory().addUniqueItem({ItemId::SCROLL, EffectType::PORTAL});
}

ItemFactory ItemFactory::armory() {
  return ItemFactory({
      {ItemId::KNIFE, 5 },
      {ItemId::SWORD, 2 },
      {ItemId::BATTLE_AXE, 2 },
      {ItemId::WAR_HAMMER, 2 },
      {ItemId::BOW, 4 },
      {ItemId::ARROW, 8, 20, 30 },
      {ItemId::LEATHER_ARMOR, 2 },
      {ItemId::CHAIN_ARMOR, 1 },
      {ItemId::LEATHER_HELM, 2 },
      {ItemId::TELEPATHY_HELM, 0.1 },
      {ItemId::IRON_HELM, 1},
      {ItemId::LEATHER_BOOTS, 2 },
      {ItemId::SPEED_BOOTS, 0.5 },
      {ItemId::LEVITATION_BOOTS, 0.5 },
      {ItemId::LEATHER_GLOVES, 2 },
      {ItemId::STRENGTH_GLOVES, 0.5 },
      {ItemId::DEXTERITY_GLOVES, 0.5 },
      {ItemId::IRON_BOOTS, 1} });
}

ItemFactory ItemFactory::goblinShop() {
  return ItemFactory({
      {ItemId::KNIFE, 5 },
      {ItemId::SWORD, 2 },
      {ItemId::BATTLE_AXE, 1 },
      {ItemId::WAR_HAMMER, 2 },
      {ItemId::LEATHER_ARMOR, 2 },
      {ItemId::CHAIN_ARMOR, 1 },
      {ItemId::LEATHER_HELM, 2 },
      {ItemId::IRON_HELM, 1 },
      {ItemId::TELEPATHY_HELM, 0.1 },
      {ItemId::LEATHER_BOOTS, 2 },
      {ItemId::IRON_BOOTS, 1 },
      {ItemId::SPEED_BOOTS, 0.3 },
      {ItemId::LEVITATION_BOOTS, 0.3 },
      {ItemId::LEATHER_GLOVES, 2 },
      {ItemId::STRENGTH_GLOVES, 0.5 },
      {ItemId::DEXTERITY_GLOVES, 0.5 },
      {{ItemId::MUSHROOM, EffectType::PANIC}, 1 },
      {{ItemId::MUSHROOM, EffectType::RAGE}, 1 },
      {{ItemId::MUSHROOM, EffectType::STR_BONUS}, 1 },
      {{ItemId::MUSHROOM, EffectType::DEX_BONUS}, 1} });
}

ItemFactory ItemFactory::dragonCave() {
  return ItemFactory({
      {ItemId::GOLD_PIECE, 10, 30, 100 },
      {ItemId::SPECIAL_SWORD, 1 },
      {ItemId::SPECIAL_BATTLE_AXE, 1 },
      {ItemId::SPECIAL_WAR_HAMMER, 1 }});
}

ItemFactory ItemFactory::workshop(const vector<Technology*>& techs) {
  ItemFactory factory({{ItemId::FIRST_AID_KIT, 2}});
  if (contains(techs, Technology::get(TechId::TRAPS))) {
    factory.addItem({ItemId::BOULDER_TRAP_ITEM, 0.5 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::POISON_GAS, EffectType::EMIT_POISON_GAS})}, 0.5 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::ALARM, EffectType::ALARM})}, 1 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::WEB, EffectType::WEB})}, 1 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::SURPRISE, EffectType::TELE_ENEMIES})}, 1 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::TERROR, EffectType::TERROR})}, 1 });
  }
  if (contains(techs, Technology::get(TechId::ARCHERY))) {
    factory.addItem({ItemId::BOW, 2 });
    factory.addItem({ItemId::ARROW, 2, 10, 30 });
  }
  if (contains(techs, Technology::get(TechId::TWO_H_WEAP))) {
    factory.addItem({ItemId::BATTLE_AXE, 2 });
    factory.addItem({ItemId::WAR_HAMMER, 2 });
    factory.addItem({ItemId::SPECIAL_BATTLE_AXE, 0.05 });
    factory.addItem({ItemId::SPECIAL_WAR_HAMMER, 0.05 });
  }
  if (contains(techs, Technology::get(TechId::IRON_WORKING))) {
    factory.addItem({ItemId::SWORD, 6 });
    factory.addItem({ItemId::SPECIAL_SWORD, 0.05 });
    factory.addItem({ItemId::CHAIN_ARMOR, 4 });
    factory.addItem({ItemId::IRON_HELM, 2 });
    factory.addItem({ItemId::IRON_BOOTS, 2 });
  } else {
    factory.addItem({ItemId::LEATHER_ARMOR, 4 });
    factory.addItem({ItemId::LEATHER_HELM, 2 });
    factory.addItem({ItemId::LEATHER_BOOTS, 2 });
    factory.addItem({ItemId::LEATHER_GLOVES, 2 });
  }
  return factory;
}

ItemFactory ItemFactory::laboratory(const vector<Technology*>& techs) {
  ItemFactory factory({
      {{ItemId::POTION, EffectType::HEAL}, 1 },
      {{ItemId::POTION, EffectType::SLEEP}, 1 },
      {{ItemId::POTION, EffectType::SLOW}, 1 },
      {{ItemId::POTION, EffectType::POISON_RESISTANCE}, 1 }});
  if (contains(techs, Technology::get(TechId::ALCHEMY_ADV))) {
    factory.addItem({{ItemId::POTION, EffectType::BLINDNESS}, 1 });
    factory.addItem({{ItemId::POTION, EffectType::INVISIBLE}, 1 });
    factory.addItem({{ItemId::POTION, EffectType::LEVITATION}, 1 });
    factory.addItem({{ItemId::POTION, EffectType::POISON}, 1 });
    factory.addItem({{ItemId::POTION, EffectType::SPEED}, 1 });
  }
  return factory;
}

ItemFactory ItemFactory::potions() {
  return ItemFactory({
      {{ItemId::POTION, EffectType::HEAL}, 1 },
      {{ItemId::POTION, EffectType::SLEEP}, 1 },
      {{ItemId::POTION, EffectType::SLOW}, 1 },
      {{ItemId::POTION, EffectType::BLINDNESS}, 1 },
      {{ItemId::POTION, EffectType::INVISIBLE}, 1 },
      {{ItemId::POTION, EffectType::POISON}, 1 },
      {{ItemId::POTION, EffectType::POISON_RESISTANCE}, 1 },
      {{ItemId::POTION, EffectType::LEVITATION}, 1 },
      {{ItemId::POTION, EffectType::SPEED}, 1 }});
}

ItemFactory ItemFactory::scrolls() {
  return ItemFactory({
      {{ItemId::SCROLL, EffectType::TELEPORT}, 1 },
      {{ItemId::SCROLL, EffectType::IDENTIFY}, 1 },
      {ItemId::FIRE_SCROLL, 1 },
      {{ItemId::SCROLL, EffectType::FIRE_SPHERE_PET}, 1 },
      {{ItemId::SCROLL, EffectType::WORD_OF_POWER}, 1 },
      {{ItemId::SCROLL, EffectType::DECEPTION}, 1 },
      {{ItemId::SCROLL, EffectType::SUMMON_INSECTS}, 1 },
      {{ItemId::SCROLL, EffectType::PORTAL}, 1 }});
}

ItemFactory ItemFactory::mushrooms(bool onlyGood) {
  return ItemFactory({
      {{ItemId::MUSHROOM, EffectType::STR_BONUS}, 1 },
      {{ItemId::MUSHROOM, EffectType::DEX_BONUS}, 1 },
      {{ItemId::MUSHROOM, EffectType::PANIC}, 1 },
      {{ItemId::MUSHROOM, EffectType::HALLU}, onlyGood ? 0.1 : 8. },
      {{ItemId::MUSHROOM, EffectType::RAGE}, 1 }});
}

ItemFactory ItemFactory::amulets() {
  return ItemFactory({
    {ItemId::WARNING_AMULET, 1},
    {ItemId::HEALING_AMULET, 1},
    {ItemId::DEFENSE_AMULET, 1},
    {ItemId::FRIENDLY_ANIMALS_AMULET, 1},}
  );
}

ItemFactory ItemFactory::dungeon() {
  return ItemFactory({
      {ItemId::KNIFE, 50 },
      {ItemId::SWORD, 50 },
      {ItemId::SPECIAL_SWORD, 1 },
      {ItemId::BATTLE_AXE, 10 },
      {ItemId::SPECIAL_BATTLE_AXE, 1 },
      {ItemId::WAR_HAMMER, 20 },
      {ItemId::SPECIAL_WAR_HAMMER, 1 },
      {ItemId::LEATHER_ARMOR, 20 },
      {ItemId::CHAIN_ARMOR, 1 },
      {ItemId::LEATHER_HELM, 20 },
      {ItemId::IRON_HELM, 5 },
      {ItemId::TELEPATHY_HELM, 1 },
      {ItemId::LEATHER_BOOTS, 20 },
      {ItemId::IRON_BOOTS, 7 },
      {ItemId::SPEED_BOOTS, 3 },
      {ItemId::LEVITATION_BOOTS, 3 },
      {ItemId::LEATHER_GLOVES, 30 },
      {ItemId::DEXTERITY_GLOVES, 3 },
      {ItemId::STRENGTH_GLOVES, 3 },
      {{ItemId::SCROLL, EffectType::TELEPORT}, 30 },
      {{ItemId::SCROLL, EffectType::PORTAL}, 10 },
      {{ItemId::SCROLL, EffectType::IDENTIFY}, 30 },
      {ItemId::FIRE_SCROLL, 30 },
      {{ItemId::SCROLL, EffectType::FIRE_SPHERE_PET}, 30 },
      {{ItemId::SCROLL, EffectType::WORD_OF_POWER}, 5 },
      {{ItemId::SCROLL, EffectType::DECEPTION}, 10 },
      {{ItemId::SCROLL, EffectType::SUMMON_INSECTS}, 30 },
      {{ItemId::POTION, EffectType::HEAL}, 50 },
      {{ItemId::POTION, EffectType::SLEEP}, 50 },
      {{ItemId::POTION, EffectType::SLOW}, 50 },
      {{ItemId::POTION, EffectType::SPEED}, 50 },
      {{ItemId::POTION, EffectType::BLINDNESS}, 30 },
      {{ItemId::POTION, EffectType::INVISIBLE}, 10 },
      {{ItemId::POTION, EffectType::POISON}, 20 },
      {{ItemId::POTION, EffectType::POISON_RESISTANCE}, 20 },
      {{ItemId::POTION, EffectType::LEVITATION}, 20 },
      {ItemId::WARNING_AMULET, 3 },
      {ItemId::HEALING_AMULET, 3 },
      {ItemId::DEFENSE_AMULET, 3 },
      {ItemId::FRIENDLY_ANIMALS_AMULET, 3},
      {{ItemId::RING, LastingEffect::POISON_RESISTANT}, 3},
      {{ItemId::RING, LastingEffect::FIRE_RESISTANT}, 3},
      {ItemId::FIRST_AID_KIT, 30 }});
}

ItemFactory ItemFactory::chest() {
  return dungeon().addItem({ItemId::GOLD_PIECE, 300, 20, 41});
}

ItemFactory ItemFactory::singleType(ItemType id) {
  return ItemFactory({{id, 1}});
}

void ItemFactory::init() {
  for (int i : Range(100))
    scroll_looks.push_back(toUpper(NameGenerator::get(NameGeneratorId::SCROLL)->getNext()));
  random_shuffle(potion_looks.begin(), potion_looks.end(),[](int a) { return Random.getRandom(a);});
  random_shuffle(amulet_looks.begin(), amulet_looks.end(),[](int a) { return Random.getRandom(a);});
}

string getEffectName(EffectType type) {
  switch (type) {
    case EffectType::HEAL: return "healing";
    case EffectType::SLEEP: return "sleep";
    case EffectType::SLOW: return "slowness";
    case EffectType::SPEED: return "speed";
    case EffectType::BLINDNESS: return "blindness";
    case EffectType::INVISIBLE: return "invisibility";
    case EffectType::POISON: return "poison";
    case EffectType::POISON_RESISTANCE: return "poison resistance";
    case EffectType::LEVITATION: return "levitation";
    case EffectType::PANIC: return "panic";
    case EffectType::RAGE: return "rage";
    case EffectType::HALLU: return "magic";
    case EffectType::STR_BONUS: return "strength";
    case EffectType::DEX_BONUS: return "dexterity";
    default: FAIL << "Effect item not implemented " << int(type);
  }
  return "";
}

string getScrollName(EffectType type) {
  switch (type) {
    case EffectType::TELEPORT: return "effugium";
    case EffectType::PORTAL: return "ianuae magicae";
    case EffectType::IDENTIFY: return "rium propositum";
    case EffectType::ROLLING_BOULDER: return "rolling boulder";
    case EffectType::EMIT_POISON_GAS: return "poison gas";
    case EffectType::DESTROY_EQUIPMENT: return "destruction";
    case EffectType::ENHANCE_WEAPON: return "melius telum";
    case EffectType::ENHANCE_ARMOR: return "melius armatus";
    case EffectType::FIRE_SPHERE_PET: return "sphaera ignis";
    case EffectType::WORD_OF_POWER: return "verbum potentiae";
    case EffectType::DECEPTION: return "deceptio";
    case EffectType::SUMMON_INSECTS: return "vocet insecta";
    case EffectType::LEAVE_BODY: return "incorporalis";
    default: FAIL << "Scroll effect not implemented " << int(type);
  }
  return "";
}

int getEffectPrice(EffectType type) {
  switch (type) {
    case EffectType::HALLU:
    case EffectType::IDENTIFY: return 15;
    case EffectType::HEAL: return 40;
    case EffectType::TELEPORT:
    case EffectType::PORTAL: return 50;
    case EffectType::ROLLING_BOULDER:
    case EffectType::DESTROY_EQUIPMENT:
    case EffectType::ENHANCE_WEAPON:
    case EffectType::ENHANCE_ARMOR:
    case EffectType::FIRE_SPHERE_PET:
    case EffectType::SPEED:
    case EffectType::PANIC:
    case EffectType::RAGE:
    case EffectType::SUMMON_INSECTS: return 60;
    case EffectType::BLINDNESS: return 80;
    case EffectType::STR_BONUS:
    case EffectType::DEX_BONUS:
    case EffectType::EMIT_POISON_GAS:  return 100;
    case EffectType::DECEPTION: 
    case EffectType::LEAVE_BODY: 
    case EffectType::WORD_OF_POWER: return 150;
    case EffectType::SLEEP:
    case EffectType::SLOW:
    case EffectType::POISON_RESISTANCE:
    case EffectType::POISON: return 100;
    case EffectType::INVISIBLE: return 120;
    case EffectType::LEVITATION: return 130;
    default: FAIL << "effect item not implemented " << int(type);
  }
  return -1;
}

const static vector<EffectType> potionEffects {
   EffectType::SLEEP,
   EffectType::SLOW,
   EffectType::HEAL,
   EffectType::SPEED,
   EffectType::BLINDNESS,
   EffectType::POISON_RESISTANCE,
   EffectType::POISON,
   EffectType::INVISIBLE,
   EffectType::LEVITATION,
};

static int getPotionNum(EffectType e) {
  return (*findElement(potionEffects, e)) % potion_looks.size();
}

ViewId getTrapViewId(TrapType t) {
  switch (t) {
    case TrapType::BOULDER: FAIL << "Not handled trap type " << int(t);
    case TrapType::POISON_GAS: return ViewId::GAS_TRAP;
    case TrapType::ALARM: return ViewId::ALARM_TRAP;
    case TrapType::WEB: return ViewId::WEB_TRAP;
    case TrapType::SURPRISE: return ViewId::SURPRISE_TRAP;
    case TrapType::TERROR: return ViewId::TERROR_TRAP;
  }
}

PItem getTrap(const ItemAttributes& attr, TrapType trapType, EffectType effectType) {
  return PItem(new TrapItem(
        ViewObject(getTrapViewId(trapType), ViewLayer::LARGE_ITEM, getEffectName(effectType) + " trap"),
        attr,
        effectType));
}

string getLastingEffectName(LastingEffect e) {
  switch (e) {
    case LastingEffect::FIRE_RESISTANT: return "fire resistance";
    case LastingEffect::POISON_RESISTANT: return "poison resistance";
    default: FAIL << "Unhandled lasting effect " << int(e);
  }
  return "";
}

ViewId getRingViewId(LastingEffect e) {
  switch (e) {
    case LastingEffect::FIRE_RESISTANT: return ViewId::FIRE_RESIST_RING;
    case LastingEffect::POISON_RESISTANT: return ViewId::POISON_RESIST_RING;
    default: FAIL << "Unhandled lasting effect " << int(e);
  }
  return ViewId(0);
}

static int maybePlusMinusOne(int prob) {
  if (Random.roll(prob))
    return Random.getRandom(2) * 2 - 1;
  return 0;
}

vector<pair<string, vector<string>>> badArtifactNames {
  {"sword", { "bang", "crush", "fist"}},
  {"battle axe", {"crush", "tooth", "razor", "fist", "bite", "bolt", "sword"}},
  {"war hammer", {"blade", "tooth", "bite", "bolt", "sword", "steel"}}};

void makeArtifact(ItemAttributes& i) {
  bool good;
  do {
    good = true;
    i.artifactName = NameGenerator::get(NameGeneratorId::WEAPON)->getNext();
    for (auto elem : badArtifactNames)
      for (auto pattern : elem.second)
        if (contains(toLower(*i.artifactName), pattern) && contains(*i.name, elem.first)) {
          Debug() << "Rejected artifact " << *i.name << " " << *i.artifactName;
          good = false;
        }
  } while (!good);
  Debug() << "Making artifact " << *i.name << " " << *i.artifactName;
  i.modifiers[AttrType::DAMAGE] += Random.getRandom(1, 4);
  i.modifiers[AttrType::TO_HIT] += Random.getRandom(1, 4);
  i.price *= 15;
}

enum class WeaponPrefix { SILVER, FLAMING, POISONOUS, GREAT, LEAD_FILLED };

void addPrefix(ItemAttributes& i, WeaponPrefix prefix) {
  i.price *= 7;
  switch (prefix) {
    case WeaponPrefix::SILVER:
      i.name = "silver " + *i.name;
      if (i.plural)
        i.plural = "silver " + *i.plural;
      i.attackEffect = EffectType::SILVER_DAMAGE;
      break;
    case WeaponPrefix::FLAMING:
      i.name = "flaming " + *i.name;
      if (i.plural)
        i.plural = "flaming " + *i.plural;
      i.attackEffect = EffectType::FIRE;
      break;
    case WeaponPrefix::POISONOUS:
      i.name = "poisonous " + *i.name;
      if (i.plural)
        i.plural = "poisonous " + *i.plural;
      i.attackEffect = EffectType::POISON;
      break;
    case WeaponPrefix::GREAT:
      i.name = "great " + *i.name;
      if (i.plural)
        i.plural = "great " + *i.plural;
      i.attackEffect = EffectType::STUN;
      break;
    case WeaponPrefix::LEAD_FILLED:
      i.name = "lead-filled " + *i.name;
      if (i.plural)
        i.plural = "lead-filled " + *i.plural;
      i.attackEffect = EffectType::STUN;
      break;
  }
}

int prefixChance = 30;

#define INHERIT(ID, X) ItemAttributes([&](ItemAttributes& i) { i = getAttributes(ItemId::ID); X })

PItem ItemFactory::fromId(ItemType item) {
  switch (item.getId()) {
    case ItemId::WARNING_AMULET: return PItem(new AmuletOfWarning(getAttributes(item), 5));
    case ItemId::TELEPATHY_HELM: return PItem(new ItemOfCreatureVision(getAttributes(item), new Telepathy()));
    case ItemId::LEVITATION_BOOTS: return PItem(new LastingEffectItem(getAttributes(item), LastingEffect::FLYING));
    case ItemId::HEALING_AMULET: return PItem(new AmuletOfHealing(getAttributes(item)));
    case ItemId::FRIENDLY_ANIMALS_AMULET:
        return PItem(new AmuletOfEnemyCheck(getAttributes(item), EnemyCheck::friendlyAnimals(0.5)));
    case ItemId::FIRE_SCROLL: return PItem(new FireScroll(getAttributes(item)));
    case ItemId::RANDOM_TECH_BOOK: return PItem(new TechBook(getAttributes(item)));
    case ItemId::TECH_BOOK: return PItem(new TechBook(getAttributes(item), Technology::get(item.get<TechId>())));
    case ItemId::POTION: return PItem(new Potion(getAttributes(item)));
    case ItemId::TRAP_ITEM:
        return getTrap(getAttributes(item), item.get<TrapInfo>().trapType(), item.get<TrapInfo>().effectType());
    case ItemId::RING: return PItem(new LastingEffectItem(getAttributes(item), item.get<LastingEffect>()));
    default: return PItem(new Item(getAttributes(item)));
  }
  return PItem();
}

ItemAttributes ItemFactory::getAttributes(ItemType item) {
  switch (item.getId()) {
    case ItemId::KNIFE: return ITATTR(
            i.viewId = ViewId::KNIFE;
            i.name = "knife";
            i.plural = "knives";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 0.3;
            i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = maybePlusMinusOne(4);
            i.attackTime = 0.7;
            i.modifiers[AttrType::THROWN_DAMAGE] = 3 + maybePlusMinusOne(4);
            i.modifiers[AttrType::THROWN_TO_HIT] = 3 + maybePlusMinusOne(4);
            i.price = 5;
            i.attackType = AttackType::STAB;);
    case ItemId::SPECIAL_KNIFE: return INHERIT(KNIFE,
            addPrefix(i, WeaponPrefix::POISONOUS);
            makeArtifact(i);
            );
    case ItemId::SPEAR: return ITATTR(
            i.viewId = ViewId::SPEAR;
            i.name = "spear";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1.5;
            i.modifiers[AttrType::DAMAGE] = 10 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 2 + maybePlusMinusOne(4);
            i.price = 20;
            i.attackType = AttackType::STAB;);
    case ItemId::SPECIAL_SWORD: return INHERIT(SWORD,
            addPrefix(i, WeaponPrefix::FLAMING);
            i.viewId = ViewId::SPECIAL_SWORD;
            makeArtifact(i);
            );
    case ItemId::SWORD: return ITATTR(
            i.viewId = ViewId::SWORD;
            i.name = "sword";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1.5;
            i.modifiers[AttrType::DAMAGE] = 8 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 3 + maybePlusMinusOne(4);
            i.price = 20;
            i.attackType = AttackType::CUT;);
    case ItemId::SPECIAL_ELVEN_SWORD: return INHERIT(ELVEN_SWORD,
            addPrefix(i, WeaponPrefix::SILVER);
            i.viewId = ViewId::SPECIAL_SWORD;
            makeArtifact(i);
            );
    case ItemId::ELVEN_SWORD: return ITATTR(
            i.viewId = ViewId::ELVEN_SWORD;
            i.name = "elven sword";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1;
            i.modifiers[AttrType::DAMAGE] = 9 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 5 + maybePlusMinusOne(4);
            i.price = 40;
            i.attackType = AttackType::CUT;);
    case ItemId::SPECIAL_BATTLE_AXE: return INHERIT(BATTLE_AXE,
            addPrefix(i, WeaponPrefix::GREAT);
            i.viewId = ViewId::SPECIAL_BATTLE_AXE;
            makeArtifact(i);
            );
    case ItemId::BATTLE_AXE: return ITATTR(
            i.viewId = ViewId::BATTLE_AXE;
            i.name = "battle axe";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 8;
            i.modifiers[AttrType::DAMAGE] = 14 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 2 + maybePlusMinusOne(4);
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 140;
            i.attackType = AttackType::CUT;);
    case ItemId::SPECIAL_WAR_HAMMER: return INHERIT(WAR_HAMMER,
            addPrefix(i, WeaponPrefix::LEAD_FILLED);
            i.viewId = ViewId::SPECIAL_WAR_HAMMER;
            makeArtifact(i);
            );
    case ItemId::WAR_HAMMER: return ITATTR(
            i.viewId = ViewId::WAR_HAMMER;
            i.name = "war hammer";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 8;
            i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 2 + maybePlusMinusOne(4);
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 100;
            i.attackType = AttackType::CRUSH;);
    case ItemId::SCYTHE: return ITATTR(
            i.viewId = ViewId::SWORD;
            i.name = "scythe";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 5;
            i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 0 + maybePlusMinusOne(4);
            i.twoHanded = true;
            i.price = 100;
            i.attackType = AttackType::CUT;);
    case ItemId::BOW: return ITATTR(
            i.viewId = ViewId::BOW;
            i.name = "short bow";
            i.itemClass = ItemClass::RANGED_WEAPON;
            i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
            i.weight = 1;
            i.rangedWeaponAccuracy = 10 + maybePlusMinusOne(4);
            i.price = 60;);
    case ItemId::ARROW: return ITATTR(
            i.viewId = ViewId::ARROW;
            i.name = "arrow";
            i.itemClass = ItemClass::AMMO;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = 5;
            i.modifiers[AttrType::THROWN_TO_HIT] = -5;
            i.price = 2;);
    case ItemId::ROBE: return ITATTR(
            i.viewId = ViewId::ROBE;
            i.name = "robe";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 2;
            i.price = 50;
            i.modifiers[AttrType::WILLPOWER] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_GLOVES: return ITATTR(
            i.viewId = ViewId::LEATHER_GLOVES;
            i.name = "leather gloves";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 10;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::DEXTERITY_GLOVES: return ITATTR(
            i.viewId = ViewId::DEXTERITY_GLOVES;
            i.name = "gloves of dexterity";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 120;
            i.modifiers[AttrType::DEXTERITY] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::STRENGTH_GLOVES: return ITATTR(
            i.viewId = ViewId::STRENGTH_GLOVES;
            i.name = "gauntlets of strength";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 120;
            i.modifiers[AttrType::STRENGTH] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_ARMOR: return ITATTR(
            i.viewId = ViewId::LEATHER_ARMOR;
            i.name = "leather armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 7;
            i.price = 20;
            i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4););
    case ItemId::LEATHER_HELM: return ITATTR(
            i.viewId = ViewId::LEATHER_HELM;
            i.name = "leather helm";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 5;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::CHAIN_ARMOR: return ITATTR(
            i.viewId = ViewId::CHAIN_ARMOR;
            i.name = "chain armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 15;
            i.price = 130;
            i.modifiers[AttrType::DEFENSE] = 5 + maybePlusMinusOne(4););
    case ItemId::IRON_HELM: return ITATTR(
            i.viewId = ViewId::IRON_HELM;
            i.name = "iron helm";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 4;
            i.price = 40;
            i.modifiers[AttrType::DEFENSE]= 2 + maybePlusMinusOne(4););
    case ItemId::TELEPATHY_HELM: return ITATTR(
            i.viewId = ViewId::TELEPATHY_HELM;
            i.name = "helm of telepathy";
            i.plural = "helms of telepathy";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 340;
            i.modifiers[AttrType::DEFENSE]= 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_BOOTS: return ITATTR(
            i.viewId = ViewId::LEATHER_BOOTS;
            i.name = "leather boots";
            i.plural = "pairs of leather boots";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 10;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::IRON_BOOTS: return ITATTR(
            i.viewId = ViewId::IRON_BOOTS;
            i.name = "iron boots";
            i.plural = "pairs of iron boots";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 4;
            i.price = 40;
            i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4););
    case ItemId::SPEED_BOOTS: return ITATTR(
            i.viewId = ViewId::SPEED_BOOTS;
            i.name = "boots of speed";
            i.plural = "pairs of boots of speed";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 360;
            i.modifiers[AttrType::SPEED] = 30;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEVITATION_BOOTS: return ITATTR(
            i.viewId = ViewId::LEVITATION_BOOTS;
            i.name = "boots of levitation";
            i.plural = "pairs of boots of levitation";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 360;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::RING: return ITATTR(
            i.viewId = getRingViewId(item.get<LastingEffect>());
            i.name = "ring of " + getLastingEffectName(item.get<LastingEffect>());
            i.plural = "rings of " + getLastingEffectName(item.get<LastingEffect>());
            i.weight = 0.05;
            i.equipmentSlot = EquipmentSlot::RINGS;
            i.itemClass = ItemClass::RING;
            i.price = 200;);
    case ItemId::WARNING_AMULET: return ITATTR(
            i.viewId = amulet_ids[0];
            i.name = amulet_looks[0] + " amulet";
            i.realName = "amulet of warning";
            i.realPlural = "amulets of warning";
            i.description = "Warns about dangerous beasts and enemies.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 220;
            i.identifiable = true;
            i.weight = 0.3;);
    case ItemId::HEALING_AMULET: return ITATTR(
            i.viewId = amulet_ids[1];
            i.name = amulet_looks[1] + " amulet";
            i.realName = "amulet of healing";
            i.realPlural = "amulets of healing";
            i.description = "Slowly heals all wounds.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 300;
            i.identifiable = true;
            i.weight = 0.3;);
    case ItemId::DEFENSE_AMULET: return ITATTR(
            i.viewId = amulet_ids[2];
            i.name = amulet_looks[2] + " amulet";
            i.realName = "amulet of defense";
            i.realPlural = "amulets of defense";
            i.description = "Increases the toughness of your skin and flesh, making you harder to wound.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 300;
            i.identifiable = true;
            i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4); 
            i.weight = 0.3;);
    case ItemId::FRIENDLY_ANIMALS_AMULET: return ITATTR(
            i.viewId = amulet_ids[3];
            i.name = amulet_looks[3] + " amulet";
            i.realName = "amulet of nature affinity";
            i.realPlural = "amulets of nature affinity";
            i.description = "Makes all animals peaceful.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 120;
            i.identifiable = true;
            i.weight = 0.3;);
    case ItemId::FIRST_AID_KIT: return ITATTR(
            i.viewId = ViewId::FIRST_AID;
            i.name = "first aid kit";
            i.weight = 0.5;
            i.itemClass = ItemClass::TOOL;
            i.applyTime = 3;
            i.uses = Random.getRandom(3, 6);
            i.usedUpMsg = true;
            i.displayUses = true;
            i.price = 10;
            i.effect = EffectType::HEAL;);
    case ItemId::BOULDER_TRAP_ITEM: return ITATTR(
            i.viewId = ViewId::TRAP_ITEM;
            i.name = "boulder trap";
            i.weight = 0.5;
            i.itemClass = ItemClass::TOOL;
            i.applyTime = 3;
            i.uses = 1;
            i.usedUpMsg = true;
            i.effect = EffectType::GUARDING_BOULDER;
            i.trapType = TrapType::BOULDER;
            i.price = 10;);
    case ItemId::TRAP_ITEM: return ITATTR(
            i.viewId = ViewId::TRAP_ITEM;
            i.name = "Unarmed " + getEffectName(item.get<TrapInfo>().effectType()) + " trap";
            i.weight = 0.5;
            i.itemClass = ItemClass::TOOL;
            i.applyTime = 3;
            i.uses = 1;
            i.usedUpMsg = true;
            i.trapType = item.get<TrapInfo>().trapType();
            i.price = 10;);
    case ItemId::POTION: return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = potion_ids[getPotionNum(effect)];
            i.name = potion_looks[getPotionNum(effect)] + " potion";
            i.realName = "potion of " + getEffectName(effect);
            i.realPlural = "potions of " + getEffectName(effect);
            i.blindName = "potion";
            i.itemClass = ItemClass::POTION;
            i.fragile = true;
            i.modifiers[AttrType::THROWN_TO_HIT] = 6;
            i.weight = 0.3;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.flamability = 0.3;
            i.identifiable = true;
            i.uses = 1;); 
    case ItemId::MUSHROOM: return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = ViewId::MUSHROOM;
            i.name = "mushroom";
            i.realName = getEffectName(effect) + " mushroom";
            i.itemClass= ItemClass::FOOD;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = -15;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.identifyOnApply = false;
            i.uses = 1;);
    case ItemId::SCROLL: 
        if (item.get<EffectType>() == EffectType::IDENTIFY && Item::isEverythingIdentified())
          return getAttributes({ItemId::SCROLL, chooseRandom(
                {EffectType::ENHANCE_WEAPON, EffectType::ENHANCE_ARMOR})});
        return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = ViewId::SCROLL;
            i.name = "scroll labelled " + getScrollName(effect);
            i.plural= "scrolls labelled " + getScrollName(effect);
            i.blindName = "scroll";
            i.itemClass = ItemClass::SCROLL;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = -10;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.flamability = 1;
            i.uses = 1;);
    case ItemId::FIRE_SCROLL: return ITATTR(
            i.viewId = ViewId::SCROLL;
            i.name = "scroll labelled combustio";
            i.plural= "scrolls labelled combustio";
            i.description = "Sets itself on fire.";
            i.blindName = "scroll";
            i.itemClass= ItemClass::SCROLL;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = -10;
            i.price = 15;
            i.flamability = 1;
            i.uses = 1;);
    case ItemId::TECH_BOOK: return ITATTR(
            i.viewId = ViewId::BOOK;
            i.name = "book of " + Technology::get(item.get<TechId>())->getName();
            i.plural = "books of " + Technology::get(item.get<TechId>())->getName();
            i.weight = 1;
            i.itemClass = ItemClass::BOOK;
            i.applyTime = 3;
            i.price = 1000;);
    case ItemId::RANDOM_TECH_BOOK: return ITATTR(
            i.viewId = ViewId::BOOK;
            i.name = "book of knowledge";
            i.plural = "books of knowledge";
            i.weight = 0.5;
            i.itemClass = ItemClass::BOOK;
            i.applyTime = 3;
            i.price = 300;);
    case ItemId::ROCK: return ITATTR(
            i.viewId = ViewId::ROCK;
            i.name = "rock";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.weight = 0.3;);
    case ItemId::IRON_ORE: return ITATTR(
            i.viewId = ViewId::IRON_ROCK;
            i.name = "iron ore";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.weight = 0.5;);
    case ItemId::WOOD_PLANK: return ITATTR(
            i.viewId = ViewId::WOOD_PLANK;
            i.name = "wood plank";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.weight = 5;);
    case ItemId::GOLD_PIECE: return ITATTR(
            i.viewId = ViewId::GOLD;
            i.name = "gold piece";
            i.itemClass = ItemClass::GOLD;
            i.price = 1;
            i.weight = 0.01;);
  }
  FAIL << "unhandled item " << (int) item.getId();
  return {};
}
  
vector<PItem> ItemFactory::fromId(ItemType id, int num) {
  vector<PItem> ret;
  for (int i : Range(num))
    ret.push_back(fromId(id));
  return ret;
}
