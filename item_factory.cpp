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
  FireScroll(const ViewObject& obj, const ItemAttributes& attr) : Item(obj, attr) {}

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
  AmuletOfWarning(const ViewObject& obj, const ItemAttributes& attr, int r) : Item(obj, attr), radius(r) {}

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
  AmuletOfHealing(const ViewObject& obj, const ItemAttributes& attr) : Item(obj, attr) {}

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
  AmuletOfEnemyCheck(const ViewObject& obj, const ItemAttributes& attr, EnemyCheck* c) : Item(obj, attr), check(c) {}

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
  ItemOfCreatureVision(const ViewObject& obj, const ItemAttributes& attr, CreatureVision* v)
      : Item(obj, attr), vision(v) {}

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
  LastingEffectItem(const ViewObject& obj, const ItemAttributes& attr, LastingEffect e)
      : Item(obj, attr), effect(e) {}

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
  Corpse(const ViewObject& obj, const ViewObject& obj2, const ItemAttributes& attr, const string& rottenN, 
        double rottingT, Item::CorpseInfo info) : 
      Item(obj, attr), 
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

PItem ItemFactory::corpse(CreatureId id, ItemType type, Item::CorpseInfo corpseInfo) {
  PCreature c = CreatureFactory::fromId(id, Tribe::get(TribeId::MONSTER));
  return corpse(c->getName() + " corpse", c->getName() + " skeleton", c->getWeight(), type, corpseInfo);
}

PItem ItemFactory::corpse(const string& name, const string& rottenName, double weight, ItemType type,
    Item::CorpseInfo corpseInfo) {
  const double rotTime = 300;
  return PItem(new Corpse(
        ViewObject(ViewId::BODY_PART, ViewLayer::ITEM, name),
        ViewObject(ViewId::BONE, ViewLayer::ITEM, rottenName),
        ITATTR(
          i.name = name;
          i.type = type;
          i.weight = weight;),
        rottenName,
        rotTime,
        corpseInfo));
}

class Potion : public Item {
  public:
  Potion(ViewObject object, const ItemAttributes& attr) : Item(object, attr) {}

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
  SkillBook(ViewObject object, const ItemAttributes& attr, Skill* s) : Item(object, attr), skill(s) {}

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
  TechBook(ViewObject object, const ItemAttributes& attr, Technology* t = nullptr)
      : Item(object, attr), tech(t) {}

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
  TrapItem(ViewObject object, ViewObject _trapObject, const ItemAttributes& attr, EffectType _effect)
      : Item(object, attr), effect(_effect), trapObject(_trapObject) {
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
      const vector<ItemId>& _unique) : unique(_unique) {
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

ItemFactory& ItemFactory::addUniqueItem(ItemId id) {
  unique.push_back(id);
  return *this;
}

vector<PItem> ItemFactory::random(Optional<int> seed) {
  if (unique.size() > 0) {
    ItemId id = unique.back();
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
  for (ItemId id : unique)
    ret.push_back(fromId(id));
  for (ItemId id : items)
    ret.push_back(fromId(id));
  return ret;
}

ItemFactory ItemFactory::villageShop() {
  return ItemFactory({
      {ItemId::TELE_SCROLL, 5 },
      {ItemId::PORTAL_SCROLL, 5 },
      {ItemId::IDENTIFY_SCROLL, 5 },
      {ItemId::FIRE_SCROLL, 5 },
      {ItemId::FIRE_SPHERE_SCROLL, 5 },
      {ItemId::WORD_OF_POWER_SCROLL, 1 },
      {ItemId::DECEPTION_SCROLL, 2 },
      {ItemId::SUMMON_INSECTS_SCROLL, 5 },
      {ItemId::HEALING_POTION, 7 },
      {ItemId::SLEEP_POTION, 5 },
      {ItemId::SLOW_POTION, 5 },
      {ItemId::SPEED_POTION,5 },
      {ItemId::BLINDNESS_POTION, 5 },
      {ItemId::INVISIBLE_POTION, 2 },
      {ItemId::WARNING_AMULET, 0.5 },
      {ItemId::HEALING_AMULET, 0.5 },
      {ItemId::DEFENSE_AMULET, 0.5 },
      {ItemId::FRIENDLY_ANIMALS_AMULET, 0.5},
      {ItemId::POISON_RESIST_RING, 0.5},
      {ItemId::FIRE_RESIST_RING, 0.5},
      {ItemId::FIRST_AID_KIT, 5},
      {ItemId::SPEED_BOOTS, 2},
      {ItemId::LEVITATION_BOOTS, 2},
      {ItemId::TELEPATHY_HELM, 2}});
}

ItemFactory ItemFactory::dwarfShop() {
  return armory().addUniqueItem(ItemId::PORTAL_SCROLL);
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
      {ItemId::PANIC_MUSHROOM, 1 },
      {ItemId::RAGE_MUSHROOM, 1 },
      {ItemId::STRENGTH_MUSHROOM, 1 },
      {ItemId::DEXTERITY_MUSHROOM, 1} });
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
    factory.addItem({ItemId::GAS_TRAP_ITEM, 0.5 });
    factory.addItem({ItemId::ALARM_TRAP_ITEM, 1 });
    factory.addItem({ItemId::WEB_TRAP_ITEM, 1 });
    factory.addItem({ItemId::SURPRISE_TRAP_ITEM, 1 });
    factory.addItem({ItemId::TERROR_TRAP_ITEM, 1 });
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
      {ItemId::HEALING_POTION, 1 },
      {ItemId::SLEEP_POTION, 1 },
      {ItemId::SLOW_POTION, 1 },
      {ItemId::POISON_RESIST_POTION, 1 }});
  if (contains(techs, Technology::get(TechId::ALCHEMY_ADV))) {
    factory.addItem({ItemId::BLINDNESS_POTION, 1 });
    factory.addItem({ItemId::INVISIBLE_POTION, 1 });
    factory.addItem({ItemId::LEVITATION_POTION, 1 });
    factory.addItem({ItemId::POISON_POTION, 1 });
    factory.addItem({ItemId::SPEED_POTION, 1 });
  }
  return factory;
}

ItemFactory ItemFactory::potions() {
  return ItemFactory({
      {ItemId::HEALING_POTION, 1 },
      {ItemId::SLEEP_POTION, 1 },
      {ItemId::SLOW_POTION, 1 },
      {ItemId::BLINDNESS_POTION, 1 },
      {ItemId::INVISIBLE_POTION, 1 },
      {ItemId::POISON_POTION, 1 },
      {ItemId::POISON_RESIST_POTION, 1 },
      {ItemId::LEVITATION_POTION, 1 },
      {ItemId::SPEED_POTION, 1 }});
}

ItemFactory ItemFactory::scrolls() {
  return ItemFactory({
      {ItemId::TELE_SCROLL, 1 },
      {ItemId::IDENTIFY_SCROLL, 1 },
      {ItemId::FIRE_SCROLL, 1 },
      {ItemId::FIRE_SPHERE_SCROLL, 1 },
      {ItemId::WORD_OF_POWER_SCROLL, 1 },
      {ItemId::DECEPTION_SCROLL, 1 },
      {ItemId::SUMMON_INSECTS_SCROLL, 1 },
      {ItemId::PORTAL_SCROLL, 1 }});
}

ItemFactory ItemFactory::mushrooms(bool onlyGood) {
  return ItemFactory({
      {ItemId::STRENGTH_MUSHROOM, 1 },
      {ItemId::DEXTERITY_MUSHROOM, 1 },
      {ItemId::PANIC_MUSHROOM, 1 },
      {ItemId::HALLU_MUSHROOM, onlyGood ? 0.1 : 8. },
      {ItemId::RAGE_MUSHROOM, 1 }});
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
      {ItemId::TELE_SCROLL, 30 },
      {ItemId::PORTAL_SCROLL, 10 },
      {ItemId::IDENTIFY_SCROLL, 30 },
      {ItemId::FIRE_SCROLL, 30 },
      {ItemId::FIRE_SPHERE_SCROLL, 30 },
      {ItemId::WORD_OF_POWER_SCROLL, 5 },
      {ItemId::DECEPTION_SCROLL, 10 },
      {ItemId::SUMMON_INSECTS_SCROLL, 30 },
      {ItemId::HEALING_POTION, 50 },
      {ItemId::SLEEP_POTION, 50 },
      {ItemId::SLOW_POTION, 50 },
      {ItemId::SPEED_POTION, 50 },
      {ItemId::BLINDNESS_POTION, 30 },
      {ItemId::INVISIBLE_POTION, 10 },
      {ItemId::POISON_POTION, 20 },
      {ItemId::POISON_RESIST_POTION, 20 },
      {ItemId::LEVITATION_POTION, 20 },
      {ItemId::WARNING_AMULET, 3 },
      {ItemId::HEALING_AMULET, 3 },
      {ItemId::DEFENSE_AMULET, 3 },
      {ItemId::FRIENDLY_ANIMALS_AMULET, 3},
      {ItemId::POISON_RESIST_RING, 3},
      {ItemId::FIRE_RESIST_RING, 3},
      {ItemId::FIRST_AID_KIT, 30 }});
}

ItemFactory ItemFactory::chest() {
  return dungeon().addItem({ItemId::GOLD_PIECE, 300, 20, 41});
}

ItemFactory ItemFactory::singleType(ItemId id) {
  return ItemFactory({{id, 1}});
}

void ItemFactory::init() {
  for (int i : Range(100))
    scroll_looks.push_back(toUpper(NameGenerator::get(NameGeneratorId::SCROLL)->getNext()));
  random_shuffle(potion_looks.begin(), potion_looks.end(),[](int a) { return Random.getRandom(a);});
  random_shuffle(amulet_looks.begin(), amulet_looks.end(),[](int a) { return Random.getRandom(a);});
}

PItem getPotion(int numLooks, string name, EffectType effect, int price, string description) {
  return PItem(new Potion(
        ViewObject(potion_ids[numLooks], ViewLayer::ITEM, "Potion"), ITATTR(
            i.name = potion_looks[numLooks] + " potion";
            i.realName = "potion of " + name;
            i.realPlural = "potions of " + name;
            i.description = description;
            i.blindName = "potion";
            i.type = ItemType::POTION;
            i.fragile = true;
            i.modifiers[AttrType::THROWN_TO_HIT] = 6;
            i.weight = 0.3;
            i.effect = effect;
            i.price = price;
            i.flamability = 0.3;
            i.identifiable = true;
            i.uses = 1;)));
}

PItem getScroll(string name, EffectType effect, int price, string description) {
  return PItem(new Item(
         ViewObject(ViewId::SCROLL, ViewLayer::ITEM, "Scroll"), ITATTR(
            i.name = "scroll labelled " + name;
            i.plural= "scrolls labelled " + name;
            i.description = description;
            i.blindName = "scroll";
            i.type= ItemType::SCROLL;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = -10;
            i.effect = effect;
            i.price = price;
            i.flamability = 1;
//            i.identifiable = true;
            i.uses = 1;)));
}

PItem getMushroom(string name, EffectType effect, string description) {
  return PItem(new Item(
         ViewObject(ViewId::MUSHROOM, ViewLayer::ITEM, "Mushroom"), ITATTR(
            i.name = "mushroom";
            i.realName = name + " mushroom";
            i.description = description;
            i.type= ItemType::FOOD;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = -15;
            i.effect = effect;
            i.price = 80;
            i.identifyOnApply = false;
            i.uses = 1;)));
}

PItem getTrap(string name, ViewId viewId, TrapType trapType, EffectType effectType) {
  return PItem(new TrapItem(ViewObject(ViewId::TRAP_ITEM, ViewLayer::ITEM, "Unarmed " + name + " trap"),
        ViewObject(viewId, ViewLayer::LARGE_ITEM, capitalFirst(name) + " trap"),ITATTR(
            i.name = name + " trap";
            i.weight = 0.5;
            i.type = ItemType::TOOL;
            i.applyTime = 3;
            i.uses = 1;
            i.usedUpMsg = true;
            i.trapType = trapType;
            i.price = 10;), effectType));
}

PItem getTechBook(TechId tech) {
  return PItem(new TechBook(
      ViewObject(ViewId::BOOK, ViewLayer::ITEM, "Book"), ITATTR(
            i.name = "book of " + Technology::get(tech)->getName();
            i.plural = "books of " + Technology::get(tech)->getName();
            i.weight = 1;
            i.type = ItemType::BOOK;
            i.applyTime = 3;
            i.price = 1000;), Technology::get(tech)));
}

PItem getRing(LastingEffect effect, string name, ViewId viewId) {
  return PItem(new LastingEffectItem(
      ViewObject(viewId, ViewLayer::ITEM, "Ring"), ITATTR(
            i.name = "ring of " + name;
            i.plural = "rings of " + name;
            i.weight = 0.05;
            i.equipmentSlot = EquipmentSlot::RINGS;
            i.type = ItemType::RING;
            i.price = 200;), effect));
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
  i.name = "antique " + *i.name;
  i.price *= 15;
}

enum class WeaponPrefix { SILVER, FLAMING, POISONOUS };

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
  }
}

int prefixChance = 30;

PItem ItemFactory::fromId(ItemId id) {
  bool artifact = false;
  bool flaming = false;
  switch (id) {
    case ItemId::KNIFE: return PItem(new Item(
        ViewObject(ViewId::KNIFE, ViewLayer::ITEM, "Knife"), ITATTR(
            i.name = "knife";
            i.plural = "knives";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 0.3;
            i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = maybePlusMinusOne(4);
            i.attackTime = 0.7;
            i.modifiers[AttrType::THROWN_DAMAGE] = 3 + maybePlusMinusOne(4);
            i.modifiers[AttrType::THROWN_TO_HIT] = 3 + maybePlusMinusOne(4);
            i.price = 5;
            i.attackType = AttackType::STAB;
            if (Random.roll(prefixChance))
              addPrefix(i, WeaponPrefix::POISONOUS);
            )));
    case ItemId::SPEAR: return PItem(new Item(
        ViewObject(ViewId::SPEAR, ViewLayer::ITEM, "Spear"), ITATTR(
            i.name = "spear";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1.5;
            i.modifiers[AttrType::DAMAGE] = 10 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 2 + maybePlusMinusOne(4);
            i.price = 20;
            i.attackType = AttackType::STAB;)));
    case ItemId::SPECIAL_SWORD: artifact = true;
    case ItemId::FLAMING_SWORD: flaming = true;
    case ItemId::SWORD: return PItem(new Item(
        ViewObject(artifact ? ViewId::SPECIAL_SWORD : ViewId::SWORD, ViewLayer::ITEM, "Sword"), ITATTR(
            i.name = "sword";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1.5;
            i.modifiers[AttrType::DAMAGE] = 8 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 3 + maybePlusMinusOne(4);
            i.price = 20;
            if (artifact) {
              makeArtifact(i);
            } else
            if (flaming || Random.roll(prefixChance))
              addPrefix(i, WeaponPrefix::FLAMING);
            i.attackType = AttackType::CUT;)));
    case ItemId::SPECIAL_ELVEN_SWORD: artifact = true;
    case ItemId::ELVEN_SWORD: return PItem(new Item(
        ViewObject(ViewId::ELVEN_SWORD, ViewLayer::ITEM, "Elven sword"), ITATTR(
            i.name = "elven sword";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1;
            i.modifiers[AttrType::DAMAGE] = 9 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 5 + maybePlusMinusOne(4);
            i.price = 40;
            if (artifact) {
              makeArtifact(i);
            } else
            if (Random.roll(prefixChance))
              addPrefix(i, WeaponPrefix::SILVER);
            i.attackType = AttackType::CUT;)));
    case ItemId::SPECIAL_BATTLE_AXE: artifact = true;
    case ItemId::BATTLE_AXE: return PItem(new Item(
        ViewObject(artifact ? ViewId::SPECIAL_BATTLE_AXE : ViewId::BATTLE_AXE, ViewLayer::ITEM, "Battle axe"), ITATTR(
            i.name = "battle axe";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 8;
            i.modifiers[AttrType::DAMAGE] = 14 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 2 + maybePlusMinusOne(4);
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 140;
            if (artifact) {
              makeArtifact(i);
            }
            i.attackType = AttackType::CUT;)));
    case ItemId::SPECIAL_WAR_HAMMER: artifact = true;
    case ItemId::WAR_HAMMER: return PItem(new Item(
        ViewObject(artifact ? ViewId::SPECIAL_WAR_HAMMER : ViewId::WAR_HAMMER, ViewLayer::ITEM, "War hammer"), ITATTR(
            i.name = "war hammer";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 8;
            i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 2 + maybePlusMinusOne(4);
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 100;
            if (artifact) {
              makeArtifact(i);
            }
            i.attackType = AttackType::CRUSH;)));
    case ItemId::SCYTHE: return PItem(new Item(
        ViewObject(ViewId::SWORD, ViewLayer::ITEM, "Scythe"), ITATTR(
            i.name = "scythe";
            i.type = ItemType::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 5;
            i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.modifiers[AttrType::TO_HIT] = 0 + maybePlusMinusOne(4);
            i.twoHanded = true;
            i.price = 100;
            i.attackType = AttackType::CUT;)));
    case ItemId::BOW: return PItem(new RangedWeapon(
        ViewObject(ViewId::BOW, ViewLayer::ITEM, "Short bow"), ITATTR(
            i.name = "short bow";
            i.type = ItemType::RANGED_WEAPON;
            i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
            i.weight = 1;
            i.rangedWeaponAccuracy = 10 + maybePlusMinusOne(4);
            i.price = 60;)));
    case ItemId::ARROW: return PItem(new Item(
        ViewObject(ViewId::ARROW, ViewLayer::ITEM, "Arrow"), ITATTR(
            i.name = "arrow";
            i.type = ItemType::AMMO;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = 5;
            i.modifiers[AttrType::THROWN_TO_HIT] = -5;
            i.price = 2;)));
    case ItemId::ROBE: return PItem(new Item(
        ViewObject(ViewId::ROBE, ViewLayer::ITEM, "Robe"), ITATTR(
            i.name = "robe";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 2;
            i.price = 50;
            i.modifiers[AttrType::WILLPOWER] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::LEATHER_GLOVES: return PItem(new Item(
        ViewObject(ViewId::LEATHER_GLOVES, ViewLayer::ITEM, "Gloves"), ITATTR(
            i.name = "leather gloves";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 10;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::DEXTERITY_GLOVES: return PItem(new Item(
        ViewObject(ViewId::DEXTERITY_GLOVES, ViewLayer::ITEM, "Gloves"), ITATTR(
            i.name = "gloves of dexterity";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 120;
            i.modifiers[AttrType::DEXTERITY] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::STRENGTH_GLOVES: return PItem(new Item(
        ViewObject(ViewId::STRENGTH_GLOVES, ViewLayer::ITEM, "Gloves"), ITATTR(
            i.name = "gauntlets of strength";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 120;
            i.modifiers[AttrType::STRENGTH] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::LEATHER_ARMOR: return PItem(new Item(
        ViewObject(ViewId::LEATHER_ARMOR, ViewLayer::ITEM, "Armor"), ITATTR(
            i.name = "leather armor";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 7;
            i.price = 20;
            i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);)));
    case ItemId::LEATHER_HELM: return PItem(new Item(
        ViewObject(ViewId::LEATHER_HELM, ViewLayer::ITEM, "Helmet"), ITATTR(
            i.name = "leather helm";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 5;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::CHAIN_ARMOR: return PItem(new Item(
        ViewObject(ViewId::CHAIN_ARMOR, ViewLayer::ITEM, "Armor"), ITATTR(
            i.name = "chain armor";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 15;
            i.price = 130;
            i.modifiers[AttrType::DEFENSE] = 5 + maybePlusMinusOne(4);)));
    case ItemId::IRON_HELM: return PItem(new Item(
        ViewObject(ViewId::IRON_HELM, ViewLayer::ITEM, "Helmet"), ITATTR(
            i.name = "iron helm";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 4;
            i.price = 40;
            i.modifiers[AttrType::DEFENSE]= 2 + maybePlusMinusOne(4);)));
    case ItemId::TELEPATHY_HELM: return PItem(new ItemOfCreatureVision(
        ViewObject(ViewId::TELEPATHY_HELM, ViewLayer::ITEM, "Helmet"), ITATTR(
            i.name = "helm of telepathy";
            i.plural = "helms of telepathy";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 340;
            i.modifiers[AttrType::DEFENSE]= 1 + maybePlusMinusOne(4);),
                new Telepathy() ));
    case ItemId::LEATHER_BOOTS: return PItem(new Item(
        ViewObject(ViewId::LEATHER_BOOTS, ViewLayer::ITEM, "Boots"), ITATTR(
            i.name = "leather boots";
            i.plural = "pairs of leather boots";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 10;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::IRON_BOOTS: return PItem(new Item(
        ViewObject(ViewId::IRON_BOOTS, ViewLayer::ITEM, "Boots"), ITATTR(
            i.name = "iron boots";
            i.plural = "pairs of iron boots";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 4;
            i.price = 40;
            i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4);)));
    case ItemId::SPEED_BOOTS: return PItem(new Item(
        ViewObject(ViewId::SPEED_BOOTS, ViewLayer::ITEM, "Boots"), ITATTR(
            i.name = "boots of speed";
            i.plural = "pairs of boots of speed";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 360;
            i.modifiers[AttrType::SPEED] = 30;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);)));
    case ItemId::LEVITATION_BOOTS: return PItem(new LastingEffectItem(
        ViewObject(ViewId::LEVITATION_BOOTS, ViewLayer::ITEM, "Boots"), ITATTR(
            i.name = "boots of levitation";
            i.plural = "pairs of boots of levitation";
            i.type = ItemType::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 360;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);), LastingEffect::FLYING));
    case ItemId::FIRE_RESIST_RING:
      return getRing(LastingEffect::FIRE_RESISTANT, "fire resistance", ViewId::FIRE_RESIST_RING);
    case ItemId::POISON_RESIST_RING:
      return getRing(LastingEffect::POISON_RESISTANT, "poison resistance", ViewId::POISON_RESIST_RING);
    case ItemId::WARNING_AMULET: return PItem(
        new AmuletOfWarning(ViewObject(amulet_ids[0], ViewLayer::ITEM, "Amulet"), 
          ITATTR(
            i.name = amulet_looks[0] + " amulet";
            i.realName = "amulet of warning";
            i.realPlural = "amulets of warning";
            i.description = "Warns about dangerous beasts and enemies.";
            i.type = ItemType::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 220;
            i.identifiable = true;
            i.weight = 0.3;), 5));
    case ItemId::HEALING_AMULET: return PItem(
        new AmuletOfHealing(ViewObject(amulet_ids[1], ViewLayer::ITEM, "Amulet"), 
          ITATTR(
            i.name = amulet_looks[1] + " amulet";
            i.realName = "amulet of healing";
            i.realPlural = "amulets of healing";
            i.description = "Slowly heals all wounds.";
            i.type = ItemType::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 300;
            i.identifiable = true;
            i.weight = 0.3;)));
    case ItemId::DEFENSE_AMULET: return PItem(
        new Item(ViewObject(amulet_ids[2], ViewLayer::ITEM, "Amulet"), 
          ITATTR(
            i.name = amulet_looks[2] + " amulet";
            i.realName = "amulet of defense";
            i.realPlural = "amulets of defense";
            i.description = "Increases the toughness of your skin and flesh, making you harder to wound.";
            i.type = ItemType::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 300;
            i.identifiable = true;
            i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4); 
            i.weight = 0.3;)));
    case ItemId::FRIENDLY_ANIMALS_AMULET: return PItem(
        new AmuletOfEnemyCheck(ViewObject(amulet_ids[3], ViewLayer::ITEM, "Amulet"), 
          ITATTR(
            i.name = amulet_looks[3] + " amulet";
            i.realName = "amulet of nature affinity";
            i.realPlural = "amulets of nature affinity";
            i.description = "Makes all animals peaceful.";
            i.type = ItemType::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 120;
            i.identifiable = true;
            i.weight = 0.3;), EnemyCheck::friendlyAnimals(0.5)));
    case ItemId::FIRST_AID_KIT: return PItem(new Item(
        ViewObject(ViewId::FIRST_AID, ViewLayer::ITEM, "First aid kit"), ITATTR(
            i.name = "first aid kit";
            i.weight = 0.5;
            i.type = ItemType::TOOL;
            i.applyTime = 3;
            i.uses = Random.getRandom(3, 6);
            i.usedUpMsg = true;
            i.displayUses = true;
            i.price = 10;
            i.effect = EffectType::HEAL;)));
    case ItemId::BOULDER_TRAP_ITEM: return PItem(new Item(
        ViewObject(ViewId::TRAP_ITEM, ViewLayer::ITEM, "Unarmed boulder trap"), ITATTR(
            i.name = "boulder trap";
            i.weight = 0.5;
            i.type = ItemType::TOOL;
            i.applyTime = 3;
            i.uses = 1;
            i.usedUpMsg = true;
            i.effect = EffectType::GUARDING_BOULDER;
            i.trapType = TrapType::BOULDER;
            i.price = 10;)));
    case ItemId::WEB_TRAP_ITEM:
        return getTrap("web", ViewId::WEB_TRAP, TrapType::WEB, EffectType::WEB);
    case ItemId::GAS_TRAP_ITEM:
        return getTrap("gas", ViewId::GAS_TRAP, TrapType::POISON_GAS, EffectType::EMIT_POISON_GAS);
    case ItemId::ALARM_TRAP_ITEM:
        return getTrap("alarm", ViewId::ALARM_TRAP, TrapType::ALARM, EffectType::ALARM);
    case ItemId::SURPRISE_TRAP_ITEM:
        return getTrap("surprise", ViewId::SURPRISE_TRAP, TrapType::SURPRISE, EffectType::TELE_ENEMIES);
    case ItemId::TERROR_TRAP_ITEM:
        return getTrap("terror", ViewId::TERROR_TRAP, TrapType::TERROR, EffectType::TERROR);
    case ItemId::HEALING_POTION: return getPotion(0, "healing", EffectType::HEAL, 40, "Heals all your wounds.");
    case ItemId::SLEEP_POTION: return getPotion(1, "sleep", EffectType::SLEEP, 40,
                                   "Puts anyone to sleep immediately.");
    case ItemId::SLOW_POTION: return getPotion(2, "slowness", EffectType::SLOW, 40,
                                  "Makes all your moves very sluggish");
    case ItemId::SPEED_POTION: return getPotion(3, "speed", EffectType::SPEED, 60,
                                  "Makes all your moves very swift");
    case ItemId::BLINDNESS_POTION: return getPotion(4, "blindness", EffectType::BLINDNESS, 80,
                                       "Makes you blind for some time.");
    case ItemId::INVISIBLE_POTION: return getPotion(5, "invisibility", EffectType::INVISIBLE, 120,
                                       "Makes you and your belongings invisible for a short time.");
    case ItemId::POISON_POTION: return getPotion(6, "poison", EffectType::POISON, 100, "Poisons.");
    case ItemId::POISON_RESIST_POTION: return getPotion(7, "poison resistance", EffectType::POISON_RESISTANCE, 100,
                                           "Makes one resistant to all poisons.");
    case ItemId::LEVITATION_POTION: return getPotion(0, "levitation", EffectType::LEVITATION, 130,
                                        "Makes one levitate for some time.");
    case ItemId::PANIC_MUSHROOM: return getMushroom("panic", EffectType::PANIC,
                                     "Makes the one who ate it favor defensive actions over offensive.");
    case ItemId::RAGE_MUSHROOM: return getMushroom("rage", EffectType::RAGE,
                                     "Makes the one who ate it favor offensive actions over defensive.");
    case ItemId::HALLU_MUSHROOM: return getMushroom("magic", EffectType::HALLU,
                                     "Has a strong hallucinogenic effect.");
    case ItemId::STRENGTH_MUSHROOM: return getMushroom("strength", EffectType::STR_BONUS,
                                     "Gives you extra strength for a while.");
    case ItemId::DEXTERITY_MUSHROOM: return getMushroom("dexterity", EffectType::DEX_BONUS,
                                     "Gives you extra dexterity for a while.");
    case ItemId::TELE_SCROLL: return getScroll("effugium", EffectType::TELEPORT, 50,
                                  "Teleports the reader away by a short distance");
    case ItemId::PORTAL_SCROLL: return getScroll("ianuae magicae", EffectType::PORTAL, 50, "Creates a magical portal. Two of such portals allow instant travel between them.");
    case ItemId::IDENTIFY_SCROLL: if (!Item::isEverythingIdentified()) 
                                    return getScroll("rium propositum", EffectType::IDENTIFY, 15,
                                        "Identifies a hidden or magical use of a chosen item.");
                                  else
                                    return fromId(chooseRandom({ItemId::ENHANCE_W_SCROLL, ItemId::ENHANCE_A_SCROLL}));
    case ItemId::BOULDER_SCROLL: return getScroll("rolling boulder", EffectType::ROLLING_BOULDER, 100, "");
    case ItemId::POISON_GAS_SCROLL: return getScroll("poison gas", EffectType::EMIT_POISON_GAS, 100, "");
    case ItemId::DESTROY_EQ_SCROLL: return getScroll("destruction", EffectType::DESTROY_EQUIPMENT, 100,
                                      "Destroys a piece of equipment of the reader.");
    case ItemId::ENHANCE_W_SCROLL: return getScroll("melius telum", EffectType::ENHANCE_WEAPON, 100,
                                      "Increases the damage or accuracy of a weapon.");
    case ItemId::ENHANCE_A_SCROLL: return getScroll("melius armatus", EffectType::ENHANCE_ARMOR, 100,
                                      "Increases the defense value of a piece of armor.");
    case ItemId::FIRE_SPHERE_SCROLL: return getScroll("sphaera ignis", EffectType::FIRE_SPHERE_PET, 100,
                                      "Creates a sphere of fire.");
    case ItemId::WORD_OF_POWER_SCROLL: return getScroll("verbum potentiae", EffectType::WORD_OF_POWER, 150,
                                      "Causes an explosion of energy that throws back all sorrounding creatures.");
    case ItemId::DECEPTION_SCROLL: return getScroll("deceptio", EffectType::DECEPTION, 150,
                                      "Creates optical illusions of the spellcaster to decept enemies.");
    case ItemId::SUMMON_INSECTS_SCROLL: return getScroll("vocet insecta", EffectType::SUMMON_INSECTS, 60,
                                      "Summon insects to confuse enemies.");
    case ItemId::LEAVE_BODY_SCROLL: return getScroll("incorporalis", EffectType::LEAVE_BODY, 60,
                                      "Frees the reader's spirit from his body.");
    case ItemId::FIRE_SCROLL: return PItem(new FireScroll(
         ViewObject(ViewId::SCROLL, ViewLayer::ITEM, "Scroll"), ITATTR(
            i.name = "scroll labelled combustio";
            i.plural= "scrolls labelled combustio";
            i.description = "Sets itself on fire.";
            i.blindName = "scroll";
            i.type= ItemType::SCROLL;
            i.weight = 0.1;
            i.modifiers[AttrType::THROWN_DAMAGE] = -10;
            i.price = 15;
            i.flamability = 1;
            i.uses = 1;)));
    case ItemId::SPELLS_MAS_BOOK: return getTechBook(TechId::SPELLS_MAS);
    case ItemId::ALCHEMY_ADV_BOOK: return getTechBook(TechId::ALCHEMY_ADV);
    case ItemId::IRON_WORKING_BOOK: return getTechBook(TechId::IRON_WORKING);
    case ItemId::HUMANOID_MUT_BOOK: return getTechBook(TechId::HUMANOID_MUT);
    case ItemId::BEAST_MUT_BOOK: return getTechBook(TechId::BEAST_MUT);
    case ItemId::TECH_BOOK: return PItem(new TechBook(
        ViewObject(ViewId::BOOK, ViewLayer::ITEM, "Book"), ITATTR(
            i.name = "book of knowledge";
            i.plural = "bookss of knowledge";
            i.weight = 0.5;
            i.type = ItemType::BOOK;
            i.applyTime = 3;
            i.price = 300;)));
    case ItemId::ROCK: return PItem(new Item(
         ViewObject(ViewId::ROCK, ViewLayer::ITEM, "Rock"), ITATTR(
            i.name = "rock";
            i.type = ItemType::OTHER;
            i.price = 0;
            i.weight = 0.3;)));
    case ItemId::IRON_ORE: return PItem(new Item(
         ViewObject(ViewId::IRON_ROCK, ViewLayer::ITEM, "Iron ore"), ITATTR(
            i.name = "iron ore";
            i.type = ItemType::OTHER;
            i.price = 0;
            i.weight = 0.5;)));
    case ItemId::WOOD_PLANK: return PItem(new Item(
         ViewObject(ViewId::WOOD_PLANK, ViewLayer::ITEM, "Wood plank"), ITATTR(
            i.name = "wood plank";
            i.type = ItemType::OTHER;
            i.price = 0;
            i.weight = 5;)));
    case ItemId::GOLD_PIECE: return PItem(new Item(
         ViewObject(ViewId::GOLD, ViewLayer::ITEM, "Gold"), ITATTR(
            i.name = "gold piece";
            i.type = ItemType::GOLD;
            i.price = 1;
            i.weight = 0.01;)));
  }
  FAIL << "unhandled item " << (int) id;
  return PItem(nullptr);
}
  
vector<PItem> ItemFactory::fromId(ItemId id, int num) {
  vector<PItem> ret;
  for (int i : Range(num))
    ret.push_back(fromId(id));
  return ret;
}
