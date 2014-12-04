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
            int diff = c->getModifier(ModifierType::DAMAGE) - owner->getModifier(ModifierType::DAMAGE);
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
        Effect::applyToPosition(level, position, EffectId::EMIT_POISON_GAS, EffectStrength::WEAK);
      if (getWeight() > 10 && !corpseInfo.isSkeleton && 
        !level->getCoverInfo(position).covered() && Random.roll(35)) {
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
  return corpse(c->getName().bare() + " corpse", c->getName().bare() + " skeleton", c->getWeight(), type, corpseInfo);
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
    index = gen.get(weights);
  } else
    index = Random.get(weights);
  return fromId(items[index], Random.get(minCount[index], maxCount[index]));
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
      {{ItemId::SCROLL, EffectId::TELEPORT}, 5 },
      {{ItemId::SCROLL, EffectId::PORTAL}, 5 },
      {{ItemId::SCROLL, EffectId::IDENTIFY}, 5 },
      {ItemId::FIRE_SCROLL, 5 },
      {{ItemId::SCROLL, EffectId::FIRE_SPHERE_PET}, 5 },
      {{ItemId::SCROLL, EffectId::WORD_OF_POWER}, 1 },
      {{ItemId::SCROLL, EffectId::DECEPTION}, 2 },
      {{ItemId::SCROLL, EffectId::SUMMON_INSECTS}, 5 },
      {{ItemId::POTION, EffectId::HEAL}, 7 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)}, 5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)}, 5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)},5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)}, 5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)}, 2 },
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
  return armory().addUniqueItem({ItemId::SCROLL, EffectId::PORTAL});
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

ItemFactory ItemFactory::orcShop() {
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
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::PANIC)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::RAGE)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::STR_BONUS)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS)}, 1} });
}

ItemFactory ItemFactory::dragonCave() {
  return ItemFactory({
      {ItemId::GOLD_PIECE, 10, 30, 100 },
      {ItemId::SPECIAL_SWORD, 1 },
      {ItemId::SPECIAL_BATTLE_AXE, 1 },
      {ItemId::SPECIAL_WAR_HAMMER, 1 }});
}

ItemFactory ItemFactory::minerals() {
  return ItemFactory({
      {ItemId::IRON_ORE, 1 },
      {ItemId::ROCK, 1 }});
}

ItemFactory ItemFactory::workshop(const vector<Technology*>& techs) {
  ItemFactory factory({
    {ItemId::FIRST_AID_KIT, 4},
    {ItemId::LEATHER_ARMOR, 4 },
    {ItemId::LEATHER_HELM, 2 },
    {ItemId::LEATHER_BOOTS, 2 },
    {ItemId::LEATHER_GLOVES, 2 },
    {ItemId::CLUB, 3 },
    {ItemId::HEAVY_CLUB, 1 },
  });
  if (contains(techs, Technology::get(TechId::TRAPS))) {
    factory.addItem({ItemId::BOULDER_TRAP_ITEM, 0.5 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::POISON_GAS, EffectId::EMIT_POISON_GAS})}, 0.5 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::ALARM, EffectId::ALARM})}, 1 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::WEB,
          EffectType(EffectId::LASTING, LastingEffect::ENTANGLED)})}, 1 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::SURPRISE, EffectId::TELE_ENEMIES})}, 1 });
    factory.addItem({{ItemId::TRAP_ITEM, TrapInfo({TrapType::TERROR,
          EffectType(EffectId::LASTING, LastingEffect::PANIC)})}, 1 });
  }
  if (contains(techs, Technology::get(TechId::ARCHERY))) {
    factory.addItem({ItemId::BOW, 2 });
    factory.addItem({ItemId::ARROW, 2, 10, 30 });
  }
  return factory;
}

ItemFactory ItemFactory::forge(const vector<Technology*>& techs) {
//  CHECK(contains(techs, Technology::get(TechId::IRON_WORKING)));
  ItemFactory factory({
    {ItemId::SWORD, 6 },
    {ItemId::SPECIAL_SWORD, 0.05 },
    {ItemId::CHAIN_ARMOR, 4 },
    {ItemId::IRON_HELM, 2 },
    {ItemId::IRON_BOOTS, 2 },
  });
  if (contains(techs, Technology::get(TechId::TWO_H_WEAP))) {
    factory.addItem({ItemId::BATTLE_AXE, 5 });
    factory.addItem({ItemId::WAR_HAMMER, 5 });
    factory.addItem({ItemId::SPECIAL_BATTLE_AXE, 0.05 });
    factory.addItem({ItemId::SPECIAL_WAR_HAMMER, 0.05 });
  }
  return factory;
}

ItemFactory ItemFactory::jeweler(const vector<Technology*>& techs) {
  return ItemFactory({
    {ItemId::WARNING_AMULET, 3 },
    {ItemId::HEALING_AMULET, 3 },
    {ItemId::DEFENSE_AMULET, 3 },
    {{ItemId::RING, LastingEffect::POISON_RESISTANT}, 3},
    {{ItemId::RING, LastingEffect::FIRE_RESISTANT}, 3},
  });
}

ItemFactory ItemFactory::laboratory(const vector<Technology*>& techs) {
  ItemFactory factory({
      {{ItemId::POTION, EffectId::HEAL}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT)}, 1 }});
  if (contains(techs, Technology::get(TechId::ALCHEMY_ADV))) {
    factory.addItem({{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)}, 1 });
    factory.addItem({{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)}, 1 });
    factory.addItem({{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::FLYING)}, 1 });
    factory.addItem({{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON)}, 1 });
    factory.addItem({{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)}, 1 });
  }
  return factory;
}

ItemFactory ItemFactory::potions() {
  return ItemFactory({
      {{ItemId::POTION, EffectId::HEAL}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::FLYING)}, 1 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)}, 1 }});
}

ItemFactory ItemFactory::scrolls() {
  return ItemFactory({
      {{ItemId::SCROLL, EffectId::TELEPORT}, 1 },
      {{ItemId::SCROLL, EffectId::IDENTIFY}, 1 },
      {ItemId::FIRE_SCROLL, 1 },
      {{ItemId::SCROLL, EffectId::FIRE_SPHERE_PET}, 1 },
      {{ItemId::SCROLL, EffectId::WORD_OF_POWER}, 1 },
      {{ItemId::SCROLL, EffectId::DECEPTION}, 1 },
      {{ItemId::SCROLL, EffectId::SUMMON_INSECTS}, 1 },
      {{ItemId::SCROLL, EffectId::PORTAL}, 1 }});
}

ItemFactory ItemFactory::mushrooms(bool onlyGood) {
  return ItemFactory({
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::STR_BONUS)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::PANIC)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::HALLU)}, onlyGood ? 0.1 : 8. },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::RAGE)}, 1 }});
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
      {{ItemId::SCROLL, EffectId::TELEPORT}, 30 },
      {{ItemId::SCROLL, EffectId::PORTAL}, 10 },
      {{ItemId::SCROLL, EffectId::IDENTIFY}, 30 },
      {ItemId::FIRE_SCROLL, 30 },
      {{ItemId::SCROLL, EffectId::FIRE_SPHERE_PET}, 30 },
      {{ItemId::SCROLL, EffectId::WORD_OF_POWER}, 5 },
      {{ItemId::SCROLL, EffectId::DECEPTION}, 10 },
      {{ItemId::SCROLL, EffectId::SUMMON_INSECTS}, 30 },
      {{ItemId::POTION, EffectId::HEAL}, 50 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)}, 50 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)}, 50 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)}, 30 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)}, 10 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON)}, 20 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT)}, 20 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::FLYING)}, 20 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)}, 50 },
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
  random_shuffle(potion_looks.begin(), potion_looks.end(),[](int a) { return Random.get(a);});
  random_shuffle(amulet_looks.begin(), amulet_looks.end(),[](int a) { return Random.get(a);});
}

int getEffectPrice(EffectType type) {
  switch (type.getId()) {
    case EffectId::LASTING:
        switch (type.get<LastingEffect>()) {
          case LastingEffect::INSANITY:
          case LastingEffect::HALLU: return 15;
          case LastingEffect::SPEED:
          case LastingEffect::PANIC:
          case LastingEffect::SLEEP:
          case LastingEffect::ENTANGLED:
          case LastingEffect::STUNNED:
          case LastingEffect::MAGIC_SHIELD:
          case LastingEffect::RAGE: return 60;
          case LastingEffect::BLIND: return 80;
          case LastingEffect::STR_BONUS:
          case LastingEffect::DEX_BONUS: return 100;
          case LastingEffect::SLOWED:
          case LastingEffect::POISON_RESISTANT:
          case LastingEffect::FIRE_RESISTANT:
          case LastingEffect::POISON: return 100;
          case LastingEffect::INVISIBLE: return 120;
          case LastingEffect::FLYING: return 130;
        }
    case EffectId::IDENTIFY: return 15;
    case EffectId::ACID:
    case EffectId::HEAL: return 40;
    case EffectId::TELEPORT:
    case EffectId::FIRE:
    case EffectId::ALARM:
    case EffectId::SILVER_DAMAGE:
    case EffectId::PORTAL: return 50;
    case EffectId::ROLLING_BOULDER:
    case EffectId::DESTROY_EQUIPMENT:
    case EffectId::ENHANCE_WEAPON:
    case EffectId::ENHANCE_ARMOR:
    case EffectId::FIRE_SPHERE_PET:
    case EffectId::TELE_ENEMIES:
    case EffectId::CURE_POISON:
    case EffectId::SUMMON_INSECTS: return 60;
    case EffectId::GUARDING_BOULDER:
    case EffectId::SUMMON_SPIRIT:
    case EffectId::EMIT_POISON_GAS:  return 100;
    case EffectId::DECEPTION: 
    case EffectId::LEAVE_BODY: 
    case EffectId::METEOR_SHOWER: 
    case EffectId::WORD_OF_POWER: return 150;
  }
  return -1;
}

const static vector<EffectType> potionEffects {
   {EffectId::LASTING, LastingEffect::SLEEP},
   {EffectId::LASTING, LastingEffect::SLOWED},
   EffectId::HEAL,
   {EffectId::LASTING, LastingEffect::SPEED},
   {EffectId::LASTING, LastingEffect::BLIND},
   {EffectId::LASTING, LastingEffect::POISON_RESISTANT},
   {EffectId::LASTING, LastingEffect::POISON},
   {EffectId::LASTING, LastingEffect::INVISIBLE},
   {EffectId::LASTING, LastingEffect::FLYING},
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
  return ViewId(0);
}

PItem getTrap(const ItemAttributes& attr, TrapType trapType, EffectType effectType) {
  return PItem(new TrapItem(
        ViewObject(getTrapViewId(trapType), ViewLayer::LARGE_ITEM, Effect::getName(effectType) + " trap"),
        attr,
        effectType));
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
    return Random.get(2) * 2 - 1;
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
  i.modifiers[ModifierType::DAMAGE] += Random.get(1, 4);
  i.modifiers[ModifierType::ACCURACY] += Random.get(1, 4);
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
      i.attackEffect = EffectId::SILVER_DAMAGE;
      break;
    case WeaponPrefix::FLAMING:
      i.name = "flaming " + *i.name;
      if (i.plural)
        i.plural = "flaming " + *i.plural;
      i.attackEffect = EffectId::FIRE;
      break;
    case WeaponPrefix::POISONOUS:
      i.name = "poisonous " + *i.name;
      if (i.plural)
        i.plural = "poisonous " + *i.plural;
      i.attackEffect = {EffectId::LASTING, LastingEffect::POISON};
      break;
    case WeaponPrefix::GREAT:
      i.name = "great " + *i.name;
      if (i.plural)
        i.plural = "great " + *i.plural;
      i.attackEffect = {EffectId::LASTING, LastingEffect::STUNNED};
      break;
    case WeaponPrefix::LEAD_FILLED:
      i.name = "lead-filled " + *i.name;
      if (i.plural)
        i.plural = "lead-filled " + *i.plural;
      i.attackEffect = {EffectId::LASTING, LastingEffect::STUNNED};
      break;
  }
}

int prefixChance = 30;

#define INHERIT(ID, X) ItemAttributes([&](ItemAttributes& i) { i = getAttributes(ItemId::ID); X })

PItem ItemFactory::fromId(ItemType item) {
  switch (item.getId()) {
    case ItemId::WARNING_AMULET: return PItem(new AmuletOfWarning(getAttributes(item), 5));
    case ItemId::BOW: return PItem(new RangedWeapon(getAttributes(item)));
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
            i.modifiers[ModifierType::DAMAGE] = 5 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = maybePlusMinusOne(4);
            i.attackTime = 0.7;
            i.modifiers[ModifierType::THROWN_DAMAGE] = 3 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::THROWN_ACCURACY] = 3 + maybePlusMinusOne(4);
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
            i.modifiers[ModifierType::DAMAGE] = 10 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 2 + maybePlusMinusOne(4);
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
            i.modifiers[ModifierType::DAMAGE] = 8 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 3 + maybePlusMinusOne(4);
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
            i.modifiers[ModifierType::DAMAGE] = 9 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 5 + maybePlusMinusOne(4);
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
            i.modifiers[ModifierType::DAMAGE] = 14 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 2 + maybePlusMinusOne(4);
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
            i.modifiers[ModifierType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 2 + maybePlusMinusOne(4);
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 100;
            i.attackType = AttackType::CRUSH;);
    case ItemId::CLUB: return ITATTR(
            i.viewId = ViewId::CLUB;
            i.name = "club";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 2;
            i.modifiers[ModifierType::DAMAGE] = 4 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 4 + maybePlusMinusOne(4);
            i.price = 10;
            i.attackType = AttackType::CRUSH;);
    case ItemId::HEAVY_CLUB: return ITATTR(
            i.viewId = ViewId::HEAVY_CLUB;
            i.name = "heavy club";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 8;
            i.modifiers[ModifierType::DAMAGE] = 10 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 2 + maybePlusMinusOne(4);
            i.twoHanded = true;
            i.price = 20;
            i.attackType = AttackType::CRUSH;);
    case ItemId::SCYTHE: return ITATTR(
            i.viewId = ViewId::SWORD;
            i.name = "scythe";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 5;
            i.modifiers[ModifierType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.modifiers[ModifierType::ACCURACY] = 0 + maybePlusMinusOne(4);
            i.twoHanded = true;
            i.price = 100;
            i.attackType = AttackType::CUT;);
    case ItemId::BOW: return ITATTR(
            i.viewId = ViewId::BOW;
            i.name = "short bow";
            i.itemClass = ItemClass::RANGED_WEAPON;
            i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
            i.weight = 1;
            i.modifiers[ModifierType::FIRED_ACCURACY] = 10 + maybePlusMinusOne(4);
            i.price = 60;);
    case ItemId::ARROW: return ITATTR(
            i.viewId = ViewId::ARROW;
            i.name = "arrow";
            i.itemClass = ItemClass::AMMO;
            i.weight = 0.1;
            i.modifiers[ModifierType::FIRED_DAMAGE] = 5;
            i.modifiers[ModifierType::FIRED_ACCURACY] = -5;
            i.price = 2;);
    case ItemId::ROBE: return ITATTR(
            i.viewId = ViewId::ROBE;
            i.name = "robe";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 2;
            i.price = 50;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_GLOVES: return ITATTR(
            i.viewId = ViewId::LEATHER_GLOVES;
            i.name = "pair of leather gloves";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 10;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::DEXTERITY_GLOVES: return ITATTR(
            i.viewId = ViewId::DEXTERITY_GLOVES;
            i.name = "pair of gloves of dexterity";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 120;
            i.attrs[AttrType::DEXTERITY] = 2 + maybePlusMinusOne(4);;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::STRENGTH_GLOVES: return ITATTR(
            i.viewId = ViewId::STRENGTH_GLOVES;
            i.name = "gauntlets of strength";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 120;
            i.attrs[AttrType::STRENGTH] = 2 + maybePlusMinusOne(4);;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_ARMOR: return ITATTR(
            i.viewId = ViewId::LEATHER_ARMOR;
            i.name = "leather armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 7;
            i.price = 20;
            i.modifiers[ModifierType::DEFENSE] = 3 + maybePlusMinusOne(4););
    case ItemId::LEATHER_HELM: return ITATTR(
            i.viewId = ViewId::LEATHER_HELM;
            i.name = "leather helm";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 5;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::CHAIN_ARMOR: return ITATTR(
            i.viewId = ViewId::CHAIN_ARMOR;
            i.name = "chain armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 15;
            i.price = 130;
            i.modifiers[ModifierType::DEFENSE] = 5 + maybePlusMinusOne(4););
    case ItemId::IRON_HELM: return ITATTR(
            i.viewId = ViewId::IRON_HELM;
            i.name = "iron helm";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 4;
            i.price = 40;
            i.modifiers[ModifierType::DEFENSE]= 2 + maybePlusMinusOne(4););
    case ItemId::TELEPATHY_HELM: return ITATTR(
            i.viewId = ViewId::TELEPATHY_HELM;
            i.name = "helm of telepathy";
            i.plural = "helms of telepathy";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 340;
            i.modifiers[ModifierType::DEFENSE]= 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_BOOTS: return ITATTR(
            i.viewId = ViewId::LEATHER_BOOTS;
            i.name = "pair of leather boots";
            i.plural = "pairs of leather boots";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 10;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::IRON_BOOTS: return ITATTR(
            i.viewId = ViewId::IRON_BOOTS;
            i.name = "pair of iron boots";
            i.plural = "pairs of iron boots";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 4;
            i.price = 40;
            i.modifiers[ModifierType::DEFENSE] = 2 + maybePlusMinusOne(4););
    case ItemId::SPEED_BOOTS: return ITATTR(
            i.viewId = ViewId::SPEED_BOOTS;
            i.name = "pair of boots of speed";
            i.plural = "pairs of boots of speed";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 360;
            i.attrs[AttrType::SPEED] = 30;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEVITATION_BOOTS: return ITATTR(
            i.viewId = ViewId::LEVITATION_BOOTS;
            i.name = "pair of boots of levitation";
            i.plural = "pairs of boots of levitation";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 360;
            i.modifiers[ModifierType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::RING: return ITATTR(
            i.viewId = getRingViewId(item.get<LastingEffect>());
            i.name = "ring of " + Effect::getName(item.get<LastingEffect>());
            i.plural = "rings of " + Effect::getName(item.get<LastingEffect>());
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
            i.modifiers[ModifierType::DEFENSE] = 3 + maybePlusMinusOne(4); 
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
            i.uses = Random.get(3, 6);
            i.usedUpMsg = true;
            i.displayUses = true;
            i.price = 10;
            i.effect = EffectId::HEAL;);
    case ItemId::BOULDER_TRAP_ITEM: return ITATTR(
            i.viewId = ViewId::TRAP_ITEM;
            i.name = "boulder trap";
            i.weight = 0.5;
            i.itemClass = ItemClass::TOOL;
            i.applyTime = 3;
            i.uses = 1;
            i.usedUpMsg = true;
            i.effect = EffectId::GUARDING_BOULDER;
            i.trapType = TrapType::BOULDER;
            i.price = 10;);
    case ItemId::TRAP_ITEM: return ITATTR(
            i.viewId = ViewId::TRAP_ITEM;
            i.name = "unarmed " + Effect::getName(item.get<TrapInfo>().effectType()) + " trap";
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
            i.realName = "potion of " + Effect::getName(effect);
            i.realPlural = "potions of " + Effect::getName(effect);
            i.blindName = "potion";
            i.itemClass = ItemClass::POTION;
            i.fragile = true;
            i.modifiers[ModifierType::THROWN_ACCURACY] = 6;
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
            i.realName = Effect::getName(effect) + " mushroom";
            i.itemClass= ItemClass::FOOD;
            i.weight = 0.1;
            i.modifiers[ModifierType::THROWN_DAMAGE] = -15;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.identifyOnApply = false;
            i.uses = 1;);
    case ItemId::SCROLL: 
        if (item.get<EffectType>() == EffectId::IDENTIFY && Item::isEverythingIdentified())
          return getAttributes({ItemId::SCROLL, chooseRandom(
                {EffectId::ENHANCE_WEAPON, EffectId::ENHANCE_ARMOR})});
        return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = ViewId::SCROLL;
            i.name = "scroll of " + Effect::getName(effect);
            i.plural= "scrolls of " + Effect::getName(effect);
            i.blindName = "scroll";
            i.itemClass = ItemClass::SCROLL;
            i.weight = 0.1;
            i.modifiers[ModifierType::THROWN_DAMAGE] = -10;
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
            i.modifiers[ModifierType::THROWN_DAMAGE] = -10;
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
            i.resourceId = CollectiveResourceId::STONE;
            i.weight = 0.3;);
    case ItemId::IRON_ORE: return ITATTR(
            i.viewId = ViewId::IRON_ROCK;
            i.name = "iron ore";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.resourceId = CollectiveResourceId::IRON;
            i.weight = 0.5;);
    case ItemId::WOOD_PLANK: return ITATTR(
            i.viewId = ViewId::WOOD_PLANK;
            i.name = "wood plank";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.resourceId = CollectiveResourceId::WOOD;
            i.weight = 5;);
    case ItemId::GOLD_PIECE: return ITATTR(
            i.viewId = ViewId::GOLD;
            i.name = "gold piece";
            i.itemClass = ItemClass::GOLD;
            i.price = 1;
            i.resourceId = CollectiveResourceId::GOLD;
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
