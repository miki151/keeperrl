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
#include "technology.h"
#include "effect.h"
#include "view_object.h"
#include "view_id.h"
#include "furniture.h"
#include "game.h"
#include "creature.h"
#include "monster_ai.h"
#include "name_generator.h"
#include "player_message.h"
#include "vision.h"
#include "resource_id.h"
#include "equipment.h"
#include "skill.h"
#include "item_attributes.h"
#include "sound.h"
#include "creature_attributes.h"
#include "event_listener.h"
#include "item_type.h"
#include "body.h"
#include "item.h"
#include "furniture_factory.h"
#include "trap_type.h"

template <class Archive> 
void ItemFactory::serialize(Archive& ar, const unsigned int version) {
  ar(items, weights, count, uniqueCounts);
}

SERIALIZABLE(ItemFactory);

SERIALIZATION_CONSTRUCTOR_IMPL(ItemFactory);

struct ItemFactory::ItemInfo {
  ItemInfo(ItemType _id, double _weight) : id(_id), weight(_weight), count(1, 2) {}
  ItemInfo(ItemType _id, double _weight, Range c)
    : id(_id), weight(_weight), count(c) {}

  ItemType id;
  double weight;
  Range count;
};

class FireScroll : public Item {
  public:
  FireScroll(const ItemAttributes& attr) : Item(attr) {}

  virtual void applySpecial(WCreature c) override {
    set = true;
  }

  virtual void specialTick(Position position) override {
    if (set) {
      fireDamage(0.03, position);
      set = false;
    }
  }

  SERIALIZE_ALL(SUBCLASS(Item), set);
  SERIALIZATION_CONSTRUCTOR(FireScroll);

  private:
  bool SERIAL(set) = false;
};

class AmuletOfWarning : public Item {
  public:
  AmuletOfWarning(const ItemAttributes& attr, int r) : Item(attr), radius(r) {}

  virtual void specialTick(Position position) override {
    WCreature owner = position.getCreature();
    if (owner && owner->getEquipment().isEquipped(this)) {
      bool isDanger = false;
      bool isBigDanger = false;
      for (Position v : position.getRectangle(Rectangle(-radius, -radius, radius + 1, radius + 1))) {
        for (auto f : v.getFurniture())
          if (f->emitsWarning(owner)) {
            if (v.dist8(position) <= 1)
              isBigDanger = true;
            else
              isDanger = true;
          }
        if (WCreature c = v.getCreature()) {
          if (!owner->canSee(c) && c->isEnemy(owner)) {
            int diff = c->getAttr(AttrType::DAMAGE) - owner->getAttr(AttrType::DEFENSE);
            if (diff > 5)
              isBigDanger = true;
            else
              if (diff > 0)
                isDanger = true;
          }
        }
      }
      if (isBigDanger)
        owner->privateMessage(getTheName() + " vibrates vigorously");
      else
      if (isDanger)
        owner->privateMessage(getTheName() + " vibrates");
    }
  }

  SERIALIZE_ALL(SUBCLASS(Item), radius);
  SERIALIZATION_CONSTRUCTOR(AmuletOfWarning);

  private:
  int SERIAL(radius);
};

class AmuletOfHealing : public Item {
  public:
  AmuletOfHealing(const ItemAttributes& attr) : Item(attr) {}

  virtual void specialTick(Position position) override {
    WCreature owner = position.getCreature();
    if (owner && owner->getEquipment().isEquipped(this))
        owner->heal(1.0 / 20);
  }

  SERIALIZE_ALL(SUBCLASS(Item));
  SERIALIZATION_CONSTRUCTOR(AmuletOfHealing);
};

class Telepathy : public CreatureVision {
  public:
  virtual bool canSee(WConstCreature c1, WConstCreature c2) override {
    return c1->getPosition().dist8(c2->getPosition()) < 5 && c2->getBody().hasBrain();
  }

  SERIALIZE_ALL(SUBCLASS(CreatureVision));
};

class ItemOfCreatureVision : public Item {
  public:
  ItemOfCreatureVision(const ItemAttributes& attr, PCreatureVision&& v) : Item(attr), vision(std::move(v)) {}

  virtual void onEquipSpecial(WCreature c) {
    c->addCreatureVision(vision.get());
  }

  virtual void onUnequipSpecial(WCreature c) {
    c->removeCreatureVision(vision.get());
  }

  SERIALIZE_ALL(SUBCLASS(Item), vision);
  SERIALIZATION_CONSTRUCTOR(ItemOfCreatureVision);

  private:
  PCreatureVision SERIAL(vision);
};

class Corpse : public Item {
  public:
  Corpse(const ViewObject& obj2, const ItemAttributes& attr, const string& rottenN, 
      double rottingT, CorpseInfo info) : 
      Item(attr), 
      object2(obj2), 
      rottingTime(rottingT), 
      rottenName(rottenN),
      corpseInfo(info) {
  }

  virtual void applySpecial(WCreature c) override {
    WItem it = c->getWeapon();
    if (it && it->getAttackType() == AttackType::CUT) {
      c->you(MsgType::DECAPITATE, getTheName());
      setName("decapitated " + getName());
    } else {
      c->privateMessage("You need something sharp to decapitate the corpse.");
    }
  }

  virtual void specialTick(Position position) override {
    double time = position.getGame()->getGlobalTime();
    if (rottenTime == -1)
      rottenTime = time + rottingTime;
    if (time >= rottenTime && !rotten) {
      setName(rottenName);
      setViewObject(object2);
      corpseInfo.isSkeleton = true;
      rotten = true;
    } else {
      if (!rotten && getWeight() > 10 && Random.roll(20 + (rottenTime - time) / 10))
        Effect::applyToPosition(position, EffectId::EMIT_POISON_GAS, EffectStrength::WEAK);
      if (getWeight() > 10 && !corpseInfo.isSkeleton && 
          !position.isCovered() && Random.roll(350)) {
        for (Position v : position.neighbors8(Random)) {
          PCreature vulture = CreatureFactory::fromId(CreatureId::VULTURE, TribeId::getPest(),
                    MonsterAIFactory::scavengerBird(v));
          if (v.canEnter(vulture.get())) {
            v.addCreature(std::move(vulture));
            v.globalMessage("A vulture lands near " + getTheName());
            rottenTime -= 40;
            break;
          }
        }
      }
    }
  }

  virtual optional<CorpseInfo> getCorpseInfo() const override { 
    return corpseInfo;
  }

  SERIALIZE_ALL(SUBCLASS(Item), object2, rotten, rottenTime, rottingTime, rottenName, corpseInfo);
  SERIALIZATION_CONSTRUCTOR(Corpse);

  private:
  ViewObject SERIAL(object2);
  bool SERIAL(rotten) = false;
  double SERIAL(rottenTime) = -1;
  double SERIAL(rottingTime);
  string SERIAL(rottenName);
  CorpseInfo SERIAL(corpseInfo);
};

PItem ItemFactory::corpse(const string& name, const string& rottenName, double weight, ItemClass itemClass,
    CorpseInfo corpseInfo) {
  const double rotTime = 300;
  return makeOwner<Corpse>(
        ViewObject(ViewId::BONE, ViewLayer::ITEM, rottenName),
        ITATTR(
          i.viewId = ViewId::BODY_PART;
          i.name = name;
          i.shortName = name;
          i.itemClass = itemClass;
          i.weight = weight;),
        rottenName,
        rotTime,
        corpseInfo);
}

class Potion : public Item {
  public:
  Potion(const ItemAttributes& attr) : Item(attr) {}

  virtual void fireDamage(double amount, Position position) override {
    heat += amount;
    INFO << getName() << " heat " << heat;
    if (heat > 0.1) {
      position.globalMessage(getAName() + " boils and explodes!");
      discarded = true;
    }
  }

  virtual void specialTick(Position position) override {
    heat = max(0., heat - 0.005);
  }

  SERIALIZE_ALL(SUBCLASS(Item), heat);
  SERIALIZATION_CONSTRUCTOR(Potion);

  private:
  double SERIAL(heat) = 0;
};

class SkillBook : public Item {
  public:
  SkillBook(const ItemAttributes& attr, Skill* s) : Item(attr), skill(s->getId()) {}

  virtual void applySpecial(WCreature c) override {
    c->addSkill(Skill::get(skill));
  }

  SERIALIZE_ALL(SUBCLASS(Item), skill);
  SERIALIZATION_CONSTRUCTOR(SkillBook);

  private:
  SkillId SERIAL(skill);
};

class TechBook : public Item {
  public:
  TechBook(const ItemAttributes& attr, optional<TechId> t) : Item(attr), tech(t) {}

  virtual void applySpecial(WCreature c) override {
    if (!read || !!tech) {
      c->getGame()->addEvent({EventId::TECHBOOK_READ, tech ? Technology::get(*tech) : nullptr});
      read = true;
    }
  }

  SERIALIZE_ALL(SUBCLASS(Item), tech, read);
  SERIALIZATION_CONSTRUCTOR(TechBook);

  private:
  optional<TechId> SERIAL(tech);
  bool SERIAL(read) = false;
};


static FurnitureType getTrapFurniture(TrapType type) {
  switch (type) {
    case TrapType::ALARM:
      return FurnitureType::ALARM_TRAP;
    case TrapType::BOULDER:
      return FurnitureType::BOULDER_TRAP;
    case TrapType::POISON_GAS:
      return FurnitureType::POISON_GAS_TRAP;
    case TrapType::SURPRISE:
      return FurnitureType::SURPRISE_TRAP;
    case TrapType::TERROR:
      return FurnitureType::TERROR_TRAP;
    case TrapType::WEB:
      return FurnitureType::WEB_TRAP;
  }
}

REGISTER_TYPE(SkillBook);
REGISTER_TYPE(TechBook);
REGISTER_TYPE(Potion);
REGISTER_TYPE(FireScroll);
REGISTER_TYPE(AmuletOfWarning);
REGISTER_TYPE(AmuletOfHealing);
REGISTER_TYPE(Telepathy);
REGISTER_TYPE(ItemOfCreatureVision);
REGISTER_TYPE(Corpse);

ItemFactory::ItemFactory(const vector<ItemInfo>& items) {
  for (auto elem : items)
    addItem(elem);
}

ItemFactory& ItemFactory::addItem(ItemInfo elem) {
  items.push_back(elem.id);
  weights.push_back(elem.weight);
  CHECK(!elem.count.isEmpty());
  count.push_back(elem.count);
  return *this;
}

ItemFactory& ItemFactory::addUniqueItem(ItemType id, Range count) {
  uniqueCounts.push_back({id, count});
  return *this;
}

vector<PItem> ItemFactory::random() {
  if (uniqueCounts.size() > 0) {
    ItemType id = uniqueCounts.back().first;
    int cnt = Random.get(uniqueCounts.back().second);
    uniqueCounts.pop_back();
    return fromId(id, cnt);
  }
  int index = Random.get(weights);
  return fromId(items[index], Random.get(count[index]));
}

ItemFactory ItemFactory::villageShop() {
  return ItemFactory({
      {{ItemId::SCROLL, EffectId::TELEPORT}, 5 },
      {{ItemId::SCROLL, EffectId::ENHANCE_ARMOR}, 5 },
      {{ItemId::SCROLL, EffectId::ENHANCE_WEAPON}, 5 },
      {ItemId::FIRE_SCROLL, 5 },
      {{ItemId::SCROLL, EffectType(EffectId::SUMMON, CreatureId::FIRE_SPHERE)}, 5 },
      {{ItemId::SCROLL, EffectId::CIRCULAR_BLAST}, 1 },
      {{ItemId::SCROLL, EffectId::DECEPTION}, 2 },
      {{ItemId::SCROLL, EffectType(EffectId::SUMMON, CreatureId::FLY)}, 5 },
      {{ItemId::POTION, EffectId::HEAL}, 7 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)}, 5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)}, 5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)},5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)}, 5 },
      {{ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)}, 2 },
      {ItemId::WARNING_AMULET, 0.5 },
      {ItemId::HEALING_AMULET, 0.5 },
      {ItemId::DEFENSE_AMULET, 0.5 },
      {{ItemId::RING, LastingEffect::POISON_RESISTANT}, 0.5},
      {{ItemId::RING, LastingEffect::FIRE_RESISTANT}, 0.5},
      {ItemId::FIRST_AID_KIT, 5},
      {ItemId::SPEED_BOOTS, 2},
      {ItemId::LEVITATION_BOOTS, 2},
      {ItemId::TELEPATHY_HELM, 2}});
}

ItemFactory ItemFactory::dwarfShop() {
  return armory();
}

ItemFactory ItemFactory::armory() {
  return ItemFactory({
      {ItemId::KNIFE, 5 },
      {ItemId::SWORD, 2 },
      {ItemId::BATTLE_AXE, 2 },
      {ItemId::WAR_HAMMER, 2 },
      {ItemId::BOW, 4 },
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
      {ItemId::IRON_BOOTS, 1} })
      .addUniqueItem(ItemId::BOW);
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
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::PANIC)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::RAGE)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DAM_BONUS)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DEF_BONUS)}, 1} });
}

ItemFactory ItemFactory::gnomeShop() {
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
      {ItemId::STRENGTH_GLOVES, 0.5 } })
      .addUniqueItem({ItemId::AUTOMATON_ITEM});
}

ItemFactory ItemFactory::dragonCave() {
  return ItemFactory({
      {ItemId::GOLD_PIECE, 10, Range(30, 100) },
      {ItemId::SPECIAL_SWORD, 1 },
      {ItemId::SPECIAL_BATTLE_AXE, 1 },
      {ItemId::SPECIAL_WAR_HAMMER, 1 }});
}

ItemFactory ItemFactory::minerals() {
  return ItemFactory({
      {ItemId::IRON_ORE, 1 },
      {ItemId::ROCK, 1 }});
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
      {{ItemId::SCROLL, EffectId::ENHANCE_ARMOR}, 1 },
      {{ItemId::SCROLL, EffectId::ENHANCE_WEAPON}, 1 },
      {ItemId::FIRE_SCROLL, 1 },
      {{ItemId::SCROLL, EffectType(EffectId::SUMMON, CreatureId::FIRE_SPHERE)}, 1 },
      {{ItemId::SCROLL, EffectId::CIRCULAR_BLAST}, 1 },
      {{ItemId::SCROLL, EffectId::DECEPTION}, 1 },
      {{ItemId::SCROLL, EffectType(EffectId::SUMMON, CreatureId::FLY)}, 1 }});
}

static ViewId getMushroomViewId(EffectType e) {
  if (e.getId() == EffectId::LASTING)
    switch (e.get<LastingEffect>()) {
      case LastingEffect::DAM_BONUS: return ViewId::MUSHROOM1;
      case LastingEffect::DEF_BONUS: return ViewId::MUSHROOM2;
      case LastingEffect::PANIC: return ViewId::MUSHROOM3;
      case LastingEffect::HALLU: return ViewId::MUSHROOM4;
      case LastingEffect::RAGE: return ViewId::MUSHROOM5;
      default: break;
    }
  return ViewId::MUSHROOM6;
}

ItemFactory ItemFactory::mushrooms(bool onlyGood) {
  return ItemFactory({
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DAM_BONUS)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::DEF_BONUS)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::PANIC)}, 1 },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::HALLU)}, onlyGood ? 0.1 : 8. },
      {{ItemId::MUSHROOM, EffectType(EffectId::LASTING, LastingEffect::RAGE)}, 1 }});
}

ItemFactory ItemFactory::amulets() {
  return ItemFactory({
    {ItemId::WARNING_AMULET, 1},
    {ItemId::HEALING_AMULET, 1},
    {ItemId::DEFENSE_AMULET, 1},}
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
      {ItemId::STRENGTH_GLOVES, 3 },
      {{ItemId::SCROLL, EffectId::TELEPORT}, 30 },
      {{ItemId::SCROLL, EffectId::ENHANCE_ARMOR}, 30 },
      {{ItemId::SCROLL, EffectId::ENHANCE_WEAPON}, 30 },
      {ItemId::FIRE_SCROLL, 30 },
      {{ItemId::SCROLL, EffectType(EffectId::SUMMON, CreatureId::FIRE_SPHERE)}, 30 },
      {{ItemId::SCROLL, EffectId::CIRCULAR_BLAST}, 5 },
      {{ItemId::SCROLL, EffectId::DECEPTION}, 10 },
      {{ItemId::SCROLL, EffectType(EffectId::SUMMON, CreatureId::FLY)}, 30 },
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
      {{ItemId::RING, LastingEffect::POISON_RESISTANT}, 3},
      {{ItemId::RING, LastingEffect::FIRE_RESISTANT}, 3},
      {ItemId::FIRST_AID_KIT, 30 }});
}

ItemFactory ItemFactory::chest() {
  return dungeon().addItem({ItemId::GOLD_PIECE, 300, Range(4, 9)});
}

ItemFactory ItemFactory::singleType(ItemType id, Range count) {
  return ItemFactory({ItemInfo{id, 1, count}});
}

int getEffectPrice(EffectType type) {
  switch (type.getId()) {
    case EffectId::LASTING:
        switch (type.get<LastingEffect>()) {
          case LastingEffect::INSANITY:
          case LastingEffect::HALLU:
          case LastingEffect::BLEEDING:
            return 2;
          case LastingEffect::SPEED:
          case LastingEffect::PANIC:
          case LastingEffect::SLEEP:
          case LastingEffect::ENTANGLED:
          case LastingEffect::TIED_UP:
          case LastingEffect::STUNNED:
          case LastingEffect::MAGIC_SHIELD:
          case LastingEffect::RAGE:
          case LastingEffect::COLLAPSED:
            return 12;
          case LastingEffect::BLIND:
            return 16;
          case LastingEffect::DAM_BONUS:
          case LastingEffect::DEF_BONUS:
            return 20;
          case LastingEffect::SLOWED:
          case LastingEffect::POISON_RESISTANT:
          case LastingEffect::FIRE_RESISTANT:
          case LastingEffect::POISON:
            return 20;
          case LastingEffect::INVISIBLE:
            return 24;
          case LastingEffect::DARKNESS_SOURCE:
          case LastingEffect::PREGNANT:
          case LastingEffect::FLYING:
            return 24;
        }
    case EffectId::ACID:
    case EffectId::HEAL:
      return 8;
    case EffectId::TELEPORT:
    case EffectId::FIRE:
    case EffectId::ALARM:
    case EffectId::SILVER_DAMAGE:
    case EffectId::ROLLING_BOULDER:
    case EffectId::DESTROY_EQUIPMENT:
    case EffectId::ENHANCE_WEAPON:
    case EffectId::ENHANCE_ARMOR:
    case EffectId::TELE_ENEMIES:
    case EffectId::CURE_POISON:
    case EffectId::SUMMON:
      return 12;
    case EffectId::EMIT_POISON_GAS:
      return 20;
    case EffectId::DECEPTION: 
    case EffectId::CIRCULAR_BLAST:
    case EffectId::PLACE_FURNITURE:
    case EffectId::SUMMON_ELEMENT:
    case EffectId::DAMAGE:
    case EffectId::INJURE_BODY_PART:
    case EffectId::LOOSE_BODY_PART:
    case EffectId::REGROW_BODY_PART:
      return 30;
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

ViewId getRingViewId(LastingEffect e) {
  switch (e) {
    case LastingEffect::FIRE_RESISTANT: return ViewId::FIRE_RESIST_RING;
    case LastingEffect::POISON_RESISTANT: return ViewId::POISON_RESIST_RING;
    default: FATAL << "Unhandled lasting effect " << int(e);
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
          INFO << "Rejected artifact " << *i.name << " " << *i.artifactName;
          good = false;
        }
  } while (!good);
  INFO << "Making artifact " << *i.name << " " << *i.artifactName;
  i.modifiers[AttrType::DAMAGE] += Random.get(1, 4);
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
      i.attackEffect = {EffectId::SILVER_DAMAGE};
      break;
    case WeaponPrefix::FLAMING:
      i.name = "flaming " + *i.name;
      if (i.plural)
        i.plural = "flaming " + *i.plural;
      i.attackEffect = {EffectId::FIRE};
      break;
    case WeaponPrefix::POISONOUS:
      i.name = "poisonous " + *i.name;
      if (i.plural)
        i.plural = "poisonous " + *i.plural;
      i.attackEffect = EffectType {EffectId::LASTING, LastingEffect::POISON};
      break;
    case WeaponPrefix::GREAT:
      i.name = "great " + *i.name;
      if (i.plural)
        i.plural = "great " + *i.plural;
      i.attackEffect = EffectType {EffectId::LASTING, LastingEffect::STUNNED};
      break;
    case WeaponPrefix::LEAD_FILLED:
      i.name = "lead-filled " + *i.name;
      if (i.plural)
        i.plural = "lead-filled " + *i.plural;
      i.attackEffect = EffectType {EffectId::LASTING, LastingEffect::STUNNED};
      break;
  }
}

ItemFactory& ItemFactory::operator = (const ItemFactory&) = default;

ItemFactory::ItemFactory(const ItemFactory&) = default;

ItemFactory::~ItemFactory() {}

#define INHERIT(ID, X) ItemAttributes([&](ItemAttributes& i) { i = getAttributes(ItemId::ID); X })

PItem ItemFactory::fromId(ItemType item) {
  switch (item.getId()) {
    case ItemId::WARNING_AMULET: return makeOwner<AmuletOfWarning>(getAttributes(item), 5);
    case ItemId::TELEPATHY_HELM: return makeOwner<ItemOfCreatureVision>(getAttributes(item), makeOwner<Telepathy>());
    case ItemId::HEALING_AMULET: return makeOwner<AmuletOfHealing>(getAttributes(item));
    case ItemId::FIRE_SCROLL: return makeOwner<FireScroll>(getAttributes(item));
    case ItemId::RANDOM_TECH_BOOK: return makeOwner<TechBook>(getAttributes(item), none);
    case ItemId::TECH_BOOK: return makeOwner<TechBook>(getAttributes(item), item.get<TechId>());
    case ItemId::POTION: return makeOwner<Potion>(getAttributes(item));
    default: return makeOwner<Item>(getAttributes(item));
  }
}

ItemAttributes ItemFactory::getAttributes(ItemType item) {
  switch (item.getId()) {
    case ItemId::KNIFE: return ITATTR(
            i.viewId = ViewId::KNIFE;
            i.name = "knife";
            i.plural = "knives"_s;
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 0.3;
            i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
            i.attackTime = 0.7;
            i.price = 1;
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
            i.price = 4;
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
            i.price = 4;
            i.attackType = AttackType::CUT;);
    case ItemId::STEEL_SWORD: return ITATTR(
            i.viewId = ViewId::STEEL_SWORD;
            i.name = "steel sword";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 1.2;
            i.modifiers[AttrType::DAMAGE] = 11 + maybePlusMinusOne(4);
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
            i.price = 8;
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
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 30;
            i.attackType = AttackType::CUT;);
    case ItemId::STEEL_BATTLE_AXE: return ITATTR(
            i.viewId = ViewId::STEEL_BATTLE_AXE;
            i.name = "steel battle axe";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 7;
            i.modifiers[AttrType::DAMAGE] = 18 + maybePlusMinusOne(4);
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 150;
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
            i.attackTime = 1.2;
            i.twoHanded = true;
            i.price = 20;
            i.attackType = AttackType::CRUSH;);
    case ItemId::CLUB: return ITATTR(
            i.viewId = ViewId::CLUB;
            i.name = "club";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 2;
            i.modifiers[AttrType::DAMAGE] = 4 + maybePlusMinusOne(4);
            i.price = 2;
            i.attackType = AttackType::CRUSH;);
    case ItemId::HEAVY_CLUB: return ITATTR(
            i.viewId = ViewId::HEAVY_CLUB;
            i.name = "heavy club";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 8;
            i.modifiers[AttrType::DAMAGE] = 10 + maybePlusMinusOne(4);
            i.twoHanded = true;
            i.price = 4;
            i.attackType = AttackType::CRUSH;);
    case ItemId::SCYTHE: return ITATTR(
            i.viewId = ViewId::SWORD;
            i.name = "scythe";
            i.itemClass = ItemClass::WEAPON;
            i.equipmentSlot = EquipmentSlot::WEAPON;
            i.weight = 5;
            i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
            i.twoHanded = true;
            i.price = 20;
            i.attackType = AttackType::CUT;);
    case ItemId::BOW: return ITATTR(
            i.viewId = ViewId::BOW;
            i.name = "short bow";
            i.itemClass = ItemClass::RANGED_WEAPON;
            i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
            i.rangedWeapon = RangedWeapon(AttrType::RANGED_DAMAGE, "arrow", ViewId::ARROW);
            i.twoHanded = true;
            i.weight = 1;
            i.modifiers[AttrType::RANGED_DAMAGE] = 10 + maybePlusMinusOne(4);
            i.price = 12;);
    case ItemId::ROBE: return ITATTR(
            i.viewId = ViewId::ROBE;
            i.name = "robe";
            i.shortName = "robe"_s;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 2;
            i.price = 10;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_GLOVES: return ITATTR(
            i.viewId = ViewId::LEATHER_GLOVES;
            i.shortName = "leather"_s;
            i.name = "pair of leather gloves";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 2;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::STRENGTH_GLOVES: return ITATTR(
            i.viewId = ViewId::STRENGTH_GLOVES;
            i.shortName = "strength"_s;
            i.name = "gloves of " + *i.shortName;
            i.plural = "pairs of " + *i.name;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::GLOVES;
            i.weight = 0.3;
            i.price = 25;
            i.modifiers[AttrType::DAMAGE] = 2 + maybePlusMinusOne(4);;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_ARMOR: return ITATTR(
            i.viewId = ViewId::LEATHER_ARMOR;
            i.shortName = "leather"_s;
            i.name = "leather armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 7;
            i.price = 4;
            i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4););
    case ItemId::LEATHER_HELM: return ITATTR(
            i.viewId = ViewId::LEATHER_HELM;
            i.shortName = "leather"_s;
            i.name = "leather helm";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 1;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::CHAIN_ARMOR: return ITATTR(
            i.viewId = ViewId::CHAIN_ARMOR;
            i.shortName = "chain"_s;
            i.name = "chain armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 15;
            i.price = 25;
            i.modifiers[AttrType::DEFENSE] = 5 + maybePlusMinusOne(4););
    case ItemId::STEEL_ARMOR: return ITATTR(
            i.viewId = ViewId::STEEL_ARMOR;
            i.shortName = "steel"_s;
            i.name = "steel armor";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
            i.weight = 13;
            i.price = 160;
            i.modifiers[AttrType::DEFENSE] = 8 + maybePlusMinusOne(4););
    case ItemId::IRON_HELM: return ITATTR(
            i.viewId = ViewId::IRON_HELM;
            i.shortName = "iron"_s;
            i.name = "iron helm";
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 4;
            i.price = 8;
            i.modifiers[AttrType::DEFENSE]= 2 + maybePlusMinusOne(4););
    case ItemId::TELEPATHY_HELM: return ITATTR(
            i.viewId = ViewId::TELEPATHY_HELM;
            i.shortName = "telepathy"_s;
            i.name = "helm of " + *i.shortName;
            i.plural = "helms of " + *i.shortName;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::HELMET;
            i.weight = 1.5;
            i.price = 70;
            i.modifiers[AttrType::DEFENSE]= 1 + maybePlusMinusOne(4););
    case ItemId::LEATHER_BOOTS: return ITATTR(
            i.viewId = ViewId::LEATHER_BOOTS;
            i.shortName = "leather"_s;
            i.name = "pair of leather boots";
            i.plural = "pairs of leather boots"_s;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 2;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::IRON_BOOTS: return ITATTR(
            i.viewId = ViewId::IRON_BOOTS;
            i.shortName = "iron"_s;
            i.name = "pair of iron boots";
            i.plural = "pairs of iron boots"_s;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 4;
            i.price = 8;
            i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4););
    case ItemId::SPEED_BOOTS: return ITATTR(
            i.viewId = ViewId::SPEED_BOOTS;
            i.shortName = "speed"_s;
            i.name = "boots of " + *i.shortName;
            i.plural = "pairs of boots of " + *i.shortName;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 70;
            i.modifiers[AttrType::SPEED] = 30;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::LEVITATION_BOOTS: return ITATTR(
            i.viewId = ViewId::LEVITATION_BOOTS;
            i.shortName = "levitation"_s;
            i.equipedEffect = LastingEffect::FLYING;
            i.name = "boots of " + *i.shortName;
            i.plural = "pairs of boots of " + *i.shortName;
            i.itemClass = ItemClass::ARMOR;
            i.equipmentSlot = EquipmentSlot::BOOTS;
            i.weight = 2;
            i.price = 70;
            i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4););
    case ItemId::RING: return ITATTR(
            i.viewId = getRingViewId(item.get<LastingEffect>());
            i.shortName = string(Effect::getName(item.get<LastingEffect>()));
            i.equipedEffect = item.get<LastingEffect>();
            i.name = "ring of " + *i.shortName;
            i.plural = "rings of " + *i.shortName;
            i.weight = 0.05;
            i.equipmentSlot = EquipmentSlot::RINGS;
            i.itemClass = ItemClass::RING;
            i.price = 40;);
    case ItemId::WARNING_AMULET: return ITATTR(
            i.viewId = ViewId::AMULET1;
            i.shortName = "warning"_s;
            i.name = "amulet of " + *i.shortName;
            i.plural = "amulets of " + *i.shortName;
            i.description = "Warns about dangerous beasts and enemies.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 40;
            i.weight = 0.3;);
    case ItemId::HEALING_AMULET: return ITATTR(
            i.viewId = ViewId::AMULET2;
            i.shortName = "healing"_s;
            i.name = "amulet of " + *i.shortName;
            i.plural = "amulets of " + *i.shortName;
            i.description = "Slowly heals all wounds.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 60;
            i.weight = 0.3;);
    case ItemId::DEFENSE_AMULET: return ITATTR(
            i.viewId = ViewId::AMULET3;
            i.shortName = "defense"_s;
            i.name = "amulet of " + *i.shortName;
            i.plural = "amulets of " + *i.shortName;
            i.description = "Increases the toughness of your skin and flesh, making you harder to wound.";
            i.itemClass = ItemClass::AMULET;
            i.equipmentSlot = EquipmentSlot::AMULET;
            i.price = 60;
            i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);
            i.weight = 0.3;);
    case ItemId::FIRST_AID_KIT: return ITATTR(
            i.viewId = ViewId::FIRST_AID;
            i.name = "first aid kit";
            i.weight = 0.5;
            i.itemClass = ItemClass::TOOL;
            i.description = "Heals your wounds, but requires a few turns to apply.";
            i.applyTime = 3;
            i.uses = Random.get(3, 6);
            i.usedUpMsg = true;
            i.displayUses = true;
            i.price = 2;
            i.effect = {EffectId::HEAL};);
    case ItemId::AUTOMATON_ITEM: return ITATTR(
            i.viewId = ViewId::TRAP_ITEM;
            i.shortName = "automaton"_s;
            i.name = "automaton";
            i.applyMsgFirstPerson = "assemble the automaton"_s;
            i.applyMsgThirdPerson = "assembles an automaton"_s;
            i.applySound = SoundId::TRAP_ARMING;
            i.weight = 30;
            i.itemClass = ItemClass::TOOL;
            i.description = "";
            i.applyTime = 3;
            i.uses = 1;
            i.price = 60;
            i.effect = EffectType(EffectId::SUMMON, CreatureId::AUTOMATON););
    case ItemId::TRAP_ITEM: return ITATTR(
            i.viewId = ViewId::TRAP_ITEM;
            auto trapName = getTrapName(item.get<TrapType>());
            i.name = "unarmed " + trapName + " trap";
            i.shortName = trapName;
            i.weight = 0.5;
            i.itemClass = ItemClass::TOOL;
            i.applyTime = 3;
            i.applySound = SoundId::TRAP_ARMING;
            i.uses = 1;
            i.usedUpMsg = true;
            i.trapType = item.get<TrapType>();
            i.effect = EffectType(EffectId::PLACE_FURNITURE, getTrapFurniture(item.get<TrapType>()));
            i.price = 2;);
    case ItemId::POTION: return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = ViewId::POTION1;
            i.shortName = Effect::getName(effect);
            i.name = "potion of " + *i.shortName;
            i.plural = "potions of " + *i.shortName;
            i.description = Effect::getDescription(effect);
            i.blindName = "potion"_s;
            i.itemClass = ItemClass::POTION;
            i.fragile = true;
            i.weight = 0.3;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.flamability = 0.3;
            i.uses = 1;); 
    case ItemId::MUSHROOM: return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = getMushroomViewId(effect);
            i.shortName = Effect::getName(effect);
            i.name = *i.shortName + " mushroom";
            i.blindName = "mushroom"_s;
            i.description = Effect::getDescription(effect);
            i.itemClass= ItemClass::FOOD;
            i.weight = 0.1;
            i.modifiers[AttrType::DAMAGE] = -15;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.uses = 1;);
    case ItemId::SCROLL: 
        return ITATTR(
            EffectType effect = item.get<EffectType>();
            i.viewId = ViewId::SCROLL;
            i.shortName = Effect::getName(effect);
            i.name = "scroll of " + *i.shortName;
            i.plural= "scrolls of "  + *i.shortName;
            i.description = Effect::getDescription(effect);
            i.blindName = "scroll"_s;
            i.itemClass = ItemClass::SCROLL;
            i.weight = 0.1;
            i.modifiers[AttrType::DAMAGE] = -10;
            i.effect = effect;
            i.price = getEffectPrice(effect);
            i.flamability = 1;
            i.uses = 1;);
    case ItemId::FIRE_SCROLL: return ITATTR(
            i.viewId = ViewId::SCROLL;
            i.name = "scroll of fire";
            i.plural= "scrolls of fire"_s;
            i.shortName = "fire"_s;
            i.description = "Sets itself on fire.";
            i.blindName = "scroll"_s;
            i.itemClass= ItemClass::SCROLL;
            i.weight = 0.1;
            i.modifiers[AttrType::DAMAGE] = -10;
            i.price = 15;
            i.flamability = 1;
            i.uses = 1;);
    case ItemId::TECH_BOOK: return ITATTR(
            i.viewId = ViewId::BOOK;
            i.shortName = Technology::get(item.get<TechId>())->getName();
            i.name = "book of " + *i.shortName;
            i.plural = "books of " + *i.shortName;
            i.weight = 1;
            i.itemClass = ItemClass::BOOK;
            i.applyTime = 3;
            i.price = 1000;);
    case ItemId::RANDOM_TECH_BOOK: return ITATTR(
            i.viewId = ViewId::BOOK;
            i.name = "book of knowledge";
            i.plural = "books of knowledge"_s;
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
    case ItemId::STEEL_INGOT: return ITATTR(
            i.viewId = ViewId::STEEL_INGOT;
            i.name = "steel ingot";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.resourceId = CollectiveResourceId::STEEL;
            i.weight = 0.5;);
    case ItemId::WOOD_PLANK: return ITATTR(
            i.viewId = ViewId::WOOD_PLANK;
            i.name = "wood plank";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.resourceId = CollectiveResourceId::WOOD;
            i.weight = 5;);
    case ItemId::BONE: return ITATTR(
            i.viewId = ViewId::BONE;
            i.name = "bone";
            i.itemClass = ItemClass::OTHER;
            i.price = 0;
            i.weight = 5;);
    case ItemId::GOLD_PIECE: return ITATTR(
            i.viewId = ViewId::GOLD;
            i.name = "gold piece";
            i.itemClass = ItemClass::GOLD;
            i.price = 1;
            i.resourceId = CollectiveResourceId::GOLD;
            i.weight = 0.01;);
  }
}
  
vector<PItem> ItemFactory::fromId(ItemType id, int num) {
  vector<PItem> ret;
  for (int i : Range(num))
    ret.push_back(fromId(id));
  return ret;
}
