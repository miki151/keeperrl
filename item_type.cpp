#include "stdafx.h"
#include "item_type.h"
#include "item.h"
#include "creature.h"
#include "equipment.h"
#include "furniture.h"
#include "attr_type.h"
#include "player_message.h"
#include "vision.h"
#include "body.h"
#include "view_object.h"
#include "attack_type.h"
#include "game.h"
#include "creature_factory.h"
#include "monster_ai.h"
#include "view_id.h"
#include "item_attributes.h"
#include "skill.h"
#include "technology.h"
#include "lasting_effect.h"
#include "name_generator.h"
#include "furniture_type.h"
#include "resource_id.h"
#include "game_event.h"
#include "content_factory.h"
#include "tech_id.h"

ItemType::ItemType(const ItemType&) = default;
ItemType::ItemType(ItemType&) = default;
ItemType::ItemType(ItemType&&) = default;

ItemType ItemType::legs(int damage) {
  return ItemType::Intrinsic{ViewId("leg_attack"), "legs", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, {}, {}, AttackMsg::KICK}};
}

ItemType ItemType::claws(int damage) {
  return ItemType::Intrinsic{ViewId("claws_attack"), "claws", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, {}, {}, AttackMsg::CLAW}};
}

ItemType ItemType::beak(int damage) {
  return ItemType::Intrinsic{ViewId("beak_attack"), "beak", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, {}, {}, AttackMsg::BITE}};
}

ItemType ItemType::fists(int damage) {
  return ItemType::Intrinsic{ViewId("fist_attack"), "fists", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, {}, {}, AttackMsg::SWING}};
}

static ItemType fangsBase(int damage, vector<VictimEffect> effect) {
  return ItemType::Intrinsic{ViewId("bite_attack"), "fangs", damage,
      WeaponInfo{false, AttackType::BITE, AttrType::DAMAGE, std::move(effect), {}, AttackMsg::BITE}};
}

ItemType ItemType::fangs(int damage) {
  return fangsBase(damage, {});
}

ItemType ItemType::fangs(int damage, VictimEffect effect) {
  return fangsBase(damage, {std::move(effect)});
}

ItemType ItemType::spellHit(int damage) {
  return ItemType::Intrinsic{ViewId("fist_attack"), "spell", damage,
      WeaponInfo{false, AttackType::HIT, AttrType::SPELL_DAMAGE, {}, {}, AttackMsg::SPELL}};
}

ItemType::ItemType() {}

bool ItemType::operator == (const ItemType& t) const {
  return t.type == type;
}

bool ItemType::operator != (const ItemType& t) const {
  return !(*this == t);
}

ItemType& ItemType::setPrefixChance(double chance) {
  prefixChance = chance;
  return *this;
}

ItemType& ItemType::operator = (const ItemType&) = default;
ItemType& ItemType::operator = (ItemType&&) = default;


class FireScrollItem : public Item {
  public:
  FireScrollItem(const ItemAttributes& attr, const ContentFactory* f) : Item(attr, f) {}

  virtual void applySpecial(Creature* c) override {
    fireDamage(c->getPosition());
    set = true;
  }

  virtual void specialTick(Position position) override {
    if (set) {
      fireDamage(position);
      set = false;
    }
  }

  SERIALIZE_ALL(SUBCLASS(Item), set)
  SERIALIZATION_CONSTRUCTOR(FireScrollItem)

  private:
  bool SERIAL(set) = false;
};

class Corpse : public Item {
  public:
  Corpse(const ViewObject& obj2, const ItemAttributes& attr, const string& rottenN,
      TimeInterval rottingT, CorpseInfo info, bool instantlyRotten, const ContentFactory* f) :
      Item(attr, f),
      object2(obj2),
      rottingTime(rottingT),
      rottenName(rottenN),
      corpseInfo(info) {
    if (instantlyRotten)
      makeRotten();
  }

  void makeRotten() {
    setName(rottenName);
    setViewObject(object2);
    corpseInfo.isSkeleton = true;
    rotten = true;
  }

  virtual void specialTick(Position position) override {
  PROFILE;
    auto time = position.getGame()->getGlobalTime();
    if (!rottenTime)
      rottenTime = time + rottingTime;
    if (time >= *rottenTime && !rotten)
      makeRotten();
    else if (getWeight() > 10 && !corpseInfo.isSkeleton && !position.isCovered() && Random.roll(350)) {
      for (Position v : position.neighbors8(Random)) {
        PCreature vulture = position.getGame()->getContentFactory()->getCreatures().fromId(CreatureId("VULTURE"), TribeId::getPest(),
            MonsterAIFactory::scavengerBird());
        if (v.canEnter(vulture.get())) {
          v.addCreature(std::move(vulture));
          v.globalMessage("A vulture lands near " + getTheName());
          *rottenTime -= 40_visible;
          break;
        }
      }
    }
  }

  virtual optional<CorpseInfo> getCorpseInfo() const override {
    return corpseInfo;
  }

  SERIALIZE_ALL(SUBCLASS(Item), object2, rotten, rottenTime, rottingTime, rottenName, corpseInfo)
  SERIALIZATION_CONSTRUCTOR(Corpse)

  private:
  ViewObject SERIAL(object2);
  bool SERIAL(rotten) = false;
  optional<GlobalTime> SERIAL(rottenTime);
  TimeInterval SERIAL(rottingTime);
  string SERIAL(rottenName);
  CorpseInfo SERIAL(corpseInfo);
};

PItem ItemFactory::corpse(const string& name, const string& rottenName, double weight, const ContentFactory* f,
    bool instantlyRotten, ItemClass itemClass, CorpseInfo corpseInfo) {
  const auto rotTime = 300_visible;
  return makeOwner<Corpse>(
        ViewObject(ViewId("bone"), ViewLayer::ITEM, rottenName),
        ITATTR(
          i.viewId = ViewId("body_part");
          i.name = name;
          i.shortName = name;
          i.itemClass = itemClass;
          i.weight = weight;),
        rottenName,
        rotTime,
        corpseInfo,
        instantlyRotten,
        f);
}

class PotionItem : public Item {
  public:
  PotionItem(const ItemAttributes& attr, const ContentFactory* f) : Item(attr, f) {}

  virtual void fireDamage(Position position) override {
    heat += 0.3;
    INFO << getName() << " heat " << heat;
    if (heat >= 1.0) {
      position.globalMessage(getAName() + " boils and explodes!");
      discarded = true;
    }
  }

  virtual void iceDamage(Position position) override {
    position.globalMessage(getAName() + " freezes and explodes!");
    discarded = true;
  }

  virtual void specialTick(Position position) override {
    heat = max(0., heat - 0.005);
  }

  SERIALIZE_ALL(SUBCLASS(Item), heat)
  SERIALIZATION_CONSTRUCTOR(PotionItem)

  private:
  double SERIAL(heat) = 0;
};

class TechBookItem : public Item {
  public:
  TechBookItem(const ItemAttributes& attr, TechId t, const ContentFactory* f) : Item(attr, f), tech(t) {}

  virtual void applySpecial(Creature* c) override {
    if (!read) {
      c->getGame()->addEvent(EventInfo::TechbookRead{tech});
      read = true;
    }
  }

  SERIALIZE_ALL(SUBCLASS(Item), tech, read)
  SERIALIZATION_CONSTRUCTOR(TechBookItem)

  private:
  TechId SERIAL(tech);
  bool SERIAL(read) = false;
};

REGISTER_TYPE(TechBookItem)
REGISTER_TYPE(PotionItem)
REGISTER_TYPE(FireScrollItem)
REGISTER_TYPE(Corpse)


ItemAttributes ItemType::getAttributes(const ContentFactory* factory) const {
  return type.visit([&](const auto& t) { return t.getAttributes(factory); });
}

PItem ItemType::get(const ContentFactory* factory) const {
  auto attributes = getAttributes(factory);
  for (auto attr : ENUM_ALL(AttrType)) {
    auto& mod = attributes.modifiers[attr];
    auto& var = attributes.modifierVariation[attr];
    if (Random.chance(attributes.variationChance) && mod > 0)
      mod = max(1, mod + Random.get(-var, var + 1));
  }
  if (attributes.ingredientFor)
    attributes.description = "Ingredient for " + attributes.ingredientFor->get(factory)->getName();
  if (!attributes.genPrefixes.empty() && Random.chance(prefixChance))
    applyPrefix(Random.choose(attributes.genPrefixes), attributes);
  return type.visit(
      [&](const FireScroll&) {
        return makeOwner<FireScrollItem>(std::move(attributes), factory);
      },
      [&](const TechBook& t) {
        return makeOwner<TechBookItem>(std::move(attributes), t.techId, factory);
      },
      [&](const Potion& t) {
        return makeOwner<PotionItem>(std::move(attributes), factory);
      },
      [&](const auto&) {
        return makeOwner<Item>(std::move(attributes), factory);
      }
  );
}


static int getEffectPrice(Effect type) {
  return type.visit(
      [&](const Effect::Lasting& e) {
        return LastingEffects::getPrice(e.lastingEffect);
      },
      [&](const Effect::Permanent& e) {
        //Permanent effects will probably be from consumable artifacts.
        return LastingEffects::getPrice(e.lastingEffect) * 100;
      },
      [&](const Effect::Acid&) {
        return 8;
      },
      [&](const Effect::Suicide&) {
        return 8;
      },
      [&](const Effect::Heal&) {
        return 8;
      },
      [&](const Effect::Escape&) {
        return 12;
      },
      [&](const Effect::Fire&) {
        return 12;
      },
      [&](const Effect::Alarm&) {
        return 12;
      },
      [&](const Effect::SilverDamage&) {
        return 12;
      },
      [&](const Effect::DestroyEquipment&) {
        return 12;
      },
      [&](const Effect::DestroyWalls&) {
        return 30;
      },
      [&](const Effect::Enhance&) {
        return 12;
      },
      [&](const Effect::TeleEnemies&) {
        return 12;
      },
      [&](const Effect::Summon&) {
        return 12;
      },
      [&](const Effect::EmitPoisonGas&) {
        return 20;
      },
      [&](const auto&) {
        return 30;
      }
  );
}

ViewId getRingViewId(LastingEffect e) {
  switch (e) {
    case LastingEffect::FIRE_RESISTANT: return ViewId("ring_red");
    case LastingEffect::POISON_RESISTANT: return ViewId("ring_green");
    default: return ViewId("ring_red");
  }
}

ViewId getAmuletViewId(LastingEffect e) {
  switch (e) {
    case LastingEffect::REGENERATION: return ViewId("amulet1");
    case LastingEffect::WARNING: return ViewId("amulet2");
    default: return ViewId("amulet3");
  }
}

static const vector<pair<string, vector<string>>> badArtifactNames {
  {"sword", { "bang", "crush", "fist"}},
  {"battle axe", {"crush", "tooth", "razor", "fist", "bite", "bolt", "sword"}},
  {"war hammer", {"blade", "tooth", "bite", "bolt", "sword", "steel"}}};

vector<PItem> ItemType::get(int num, const ContentFactory* factory) const {
  vector<PItem> ret;
  for (int i : Range(num))
    ret.push_back(get(factory));
  return ret;
}

static string getRandomPoem() {
  return Random.choose(makeVec<string>("bad", "obscene", "vulgar")) + " " +
      Random.choose(makeVec<string>("poem", "haiku", "sonnet", "limerick"));
}

ItemAttributes ItemType::Poem::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.name = getRandomPoem();
      i.blindName = "scroll"_s;
      i.itemClass = ItemClass::SCROLL;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -10;
      i.effect = Effect(Effect::Area{10, Effect::Filter{FilterType::ENEMY, Effect::IncreaseMorale{-0.1}}});
      i.price = getEffectPrice(*i.effect);
      i.burnTime = 5;
      i.uses = 1;
  );
}

ItemAttributes ItemType::Intrinsic::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = viewId;
      i.name = name;
      for (auto& effect : weaponInfo.victimEffect)
        i.prefixes.push_back(effect.effect.getName());
      for (auto& effect : weaponInfo.attackerEffect)
        i.prefixes.push_back(effect.getName());
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[weaponInfo.meleeAttackAttr] = damage;
      i.price = 1;
      i.weaponInfo = weaponInfo;
  );
}

ItemAttributes ItemType::Ring::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = getRingViewId(lastingEffect);
      i.shortName = LastingEffects::getName(lastingEffect);
      i.equipedEffect.push_back(lastingEffect);
      i.name = "ring of " + *i.shortName;
      i.plural = "rings of " + *i.shortName;
      i.weight = 0.05;
      i.equipmentSlot = EquipmentSlot::RINGS;
      i.itemClass = ItemClass::RING;
      i.price = 40;
  );
}

ItemAttributes ItemType::Amulet::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = getAmuletViewId(lastingEffect);
      i.shortName = LastingEffects::getName(lastingEffect);
      i.equipedEffect.push_back(lastingEffect);
      i.name = "amulet of " + *i.shortName;
      i.plural = "amulets of " + *i.shortName;
      i.itemClass = ItemClass::AMULET;
      i.equipmentSlot = EquipmentSlot::AMULET;
      i.price = 5 * LastingEffects::getPrice(lastingEffect);
      i.weight = 0.3;
  );
}

ItemAttributes ItemType::TrapItem::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("trap_item");
      i.name = "unarmed " + trapName + " trap";
      i.shortName = trapName;
      i.weight = 0.5;
      i.itemClass = ItemClass::TOOL;
      i.applyTime = 3_visible;
      i.applySound = SoundId::TRAP_ARMING;
      i.uses = 1;
      i.usedUpMsg = true;
      i.effect = Effect(Effect::PlaceFurniture{trapType});
      i.price = 2;
  );
}

ItemAttributes CustomItemId::getAttributes(const ContentFactory* factory) const {
  return factory->items.at(*this);
}

ItemAttributes ItemType::Potion::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("potion1");
      i.shortName = effect.getName();
      i.name = "potion of " + *i.shortName;
      i.plural = "potions of " + *i.shortName;
      i.blindName = "potion"_s;
      i.itemClass = ItemClass::POTION;
      i.fragile = true;
      i.weight = 0.3;
      i.effect = effect;
      i.price = getEffectPrice(effect);
      i.burnTime = 1;
      i.uses = 1;
  );
}

static ViewId getMushroomViewId(Effect e) {
  return e.visit(
      [&](const Effect::Lasting& e) {
        switch (e.lastingEffect) {
          case LastingEffect::DAM_BONUS: return ViewId("mushroom1");
          case LastingEffect::DEF_BONUS: return ViewId("mushroom2");
          case LastingEffect::PANIC: return ViewId("mushroom3");
          case LastingEffect::HALLU: return ViewId("mushroom4");
          case LastingEffect::RAGE: return ViewId("mushroom5");
          case LastingEffect::REGENERATION: return ViewId("mushroom6");
          case LastingEffect::NIGHT_VISION: return ViewId("mushroom7");
          default: return ViewId("mushroom6");
        }
      },
      [&](const auto&) {
        return ViewId("mushroom6");
      }
  );
}

ItemAttributes ItemType::Mushroom::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = getMushroomViewId(effect);
      i.shortName = effect.getName();
      i.name = *i.shortName + " mushroom";
      i.blindName = "mushroom"_s;
      i.itemClass= ItemClass::FOOD;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -15;
      i.effect = effect;
      i.price = getEffectPrice(effect);
      i.uses = 1;
  );
}

static ViewId getRuneViewId(const string& name) {
  int h = int(combineHash(name));
  const static vector<ViewId> ids = {ViewId("rune1"), ViewId("rune2"), ViewId("rune3"), ViewId("rune4")};
  return ids[(h % ids.size() + ids.size()) % ids.size()];
}

ItemAttributes ItemType::Glyph::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.shortName = getGlyphName(rune.prefix);
      i.viewId = getRuneViewId(*i.shortName);
      i.upgradeInfo = rune;
      i.name = "glyph of " + *i.shortName;
      i.plural= "glyph of "  + *i.shortName;
      i.blindName = "glyph"_s;
      i.itemClass = ItemClass::SCROLL;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -10;
      i.price = 100;
      i.uses = 1;
  );
}

ItemAttributes ItemType::Scroll::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.shortName = effect.getName();
      i.name = "scroll of " + *i.shortName;
      i.plural= "scrolls of "  + *i.shortName;
      i.blindName = "scroll"_s;
      i.itemClass = ItemClass::SCROLL;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -10;
      i.effect = effect;
      i.price = getEffectPrice(effect);
      i.burnTime = 5;
      i.uses = 1;
  );
}

ItemAttributes ItemType::FireScroll::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.name = "scroll of fire";
      i.plural= "scrolls of fire"_s;
      i.shortName = "fire"_s;
      i.description = "Sets itself on fire.";
      i.blindName = "scroll"_s;
      i.itemClass= ItemClass::SCROLL;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -10;
      i.price = 15;
      i.burnTime = 10;
      i.uses = 1;
  );
}

ItemAttributes ItemType::TechBook::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("book");
      i.shortName = string(techId.data());
      i.name = "book of " + *i.shortName;
      i.plural = "books of " + *i.shortName;
      i.weight = 1;
      i.itemClass = ItemClass::BOOK;
      i.applyTime = 3_visible;
      i.price = 1000;
  );
}

SERIALIZE_DEF(ItemType, NAMED(type), OPTION(prefixChance))

#include "pretty_archive.h"
template void ItemType::serialize(PrettyInputArchive&, unsigned);
