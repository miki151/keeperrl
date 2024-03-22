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
#include "lasting_effect.h"
#include "name_generator.h"
#include "furniture_type.h"
#include "resource_id.h"
#include "game_event.h"
#include "content_factory.h"
#include "effect_type.h"
#include "item_types.h"
#include "item_prefix.h"
#include "statistics.h"
#include "lasting_or_buff.h"
#include "assembled_minion.h"

STRUCT_IMPL(ItemType)

ItemType ItemType::legs(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("leg_attack"), "legs", damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::KICK}});
}

ItemType ItemType::claws(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("claws_attack"), "claws", damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::CLAW}});
}

ItemType ItemType::beak(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("beak_attack"), "beak", damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::BITE}});
}

ItemType ItemType::fists(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("fist_attack"), "fists", damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::SWING}});
}

static ItemType fangsBase(int damage, vector<ItemPrefixes::VictimEffect> effect) {
  return ItemType(ItemTypes::Intrinsic{ViewId("bite_attack"), "fangs", damage,
      WeaponInfo{false, AttackType::BITE, AttrType("DAMAGE"), std::move(effect), {}, AttackMsg::BITE}});
}

ItemType ItemType::fangs(int damage) {
  return fangsBase(damage, {});
}

ItemType ItemType::fangs(int damage, ItemPrefixes::VictimEffect effect) {
  return fangsBase(damage, {std::move(effect)});
}

ItemType ItemType::spellHit(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("fist_attack"), "spell", damage,
      WeaponInfo{false, AttackType::HIT, AttrType("SPELL_DAMAGE"), {}, {}, AttackMsg::SPELL}});
}

ItemType::ItemType(const ItemTypeVariant& v) : type(v) {
}

ItemType::ItemType() {}

ItemType ItemType::setPrefixChance(double chance)&& {
  return ItemType(ItemTypes::PrefixChance{chance, *this});
}

class FireScrollItem : public Item {
  public:
  FireScrollItem(const ItemAttributes& attr, const ContentFactory* f) : Item(attr, f) {}

  virtual void applySpecial(Creature* c) override {
    fireDamage(c->getPosition());
    set = true;
  }

  virtual bool canEverTick(bool carried) const override {
    return true;
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

  virtual bool canEverTick(bool carried) const override {
    return true;
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

static PItem corpse(const ItemAttributes& attr, const string& rottenName, const ContentFactory* f,
    bool instantlyRotten, CorpseInfo corpseInfo) {
  const auto rotTime = 300_visible;
  return makeOwner<Corpse>(
        ViewObject(ViewId("bone"), ViewLayer::ITEM, rottenName),
        attr,
        rottenName,
        rotTime,
        corpseInfo,
        instantlyRotten,
        f);
}

static string getBodyPartBone(BodyPart part) {
  switch (part) {
    case BodyPart::HEAD: return "skull";
    default: return "bone";
  }
}

static string getBodyPartName(const string& creatureName, BodyPart part) {
  return creatureName + " " + getName(part);
}

PItem ItemType::severedLimb(const string& creatureName, BodyPart part, double weight, ItemClass itemClass,
    const ContentFactory* factory) {
  return corpse(getBodyPartName(creatureName, part), creatureName + " " + getBodyPartBone(part),
        weight / 8, factory, false, itemClass);

}

static ItemAttributes getCorpseAttr(const string& name, ItemClass itemClass, double weight, bool isResource,
    optional<string> ingredient) {
  return ITATTR(
    i.viewId = ViewId("body_part");
    i.name = name;
    if (isResource)
      i.resourceId = CollectiveResourceId("CORPSE");
    i.shortName = name;
    i.itemClass = itemClass;
    i.weight = weight;
    i.storageIds = {StorageId("corpses")};
    i.ingredientType = ingredient;
  );
}

PItem ItemType::corpse(const string& name, const string& rottenName, double weight, const ContentFactory* f,
    bool instantlyRotten, ItemClass itemClass, CorpseInfo corpseInfo, optional<string> ingredient) {
  return ::corpse(getCorpseAttr(name, itemClass, weight, corpseInfo.canBeRevived, ingredient),
      rottenName, f, instantlyRotten, corpseInfo);
}

class PotionItem : public Item {
  public:
  PotionItem(const ItemAttributes& attr, const ContentFactory* f) : Item(attr, f) {}

  virtual void fireDamage(Position position) override {
    heat += 0.3;
//    INFO << getName() << " heat " << heat;
    if (heat >= 1.0) {
      position.globalMessage(getAName() + " boils and explodes!");
      discarded = true;
    }
  }

  virtual void iceDamage(Position position) override {
    position.globalMessage(getAName() + " freezes and explodes!");
    discarded = true;
  }

  virtual bool canEverTick(bool carried) const override {
    return true;
  }

  virtual void specialTick(Position position) override {
    heat = max(0., heat - 0.005);
  }

  SERIALIZE_ALL(SUBCLASS(Item), heat)
  SERIALIZATION_CONSTRUCTOR(PotionItem)

  private:
  double SERIAL(heat) = 0;
};

REGISTER_TYPE(PotionItem)
REGISTER_TYPE(FireScrollItem)
REGISTER_TYPE(Corpse)


ItemAttributes ItemType::getAttributes(const ContentFactory* factory) const {
  return type->visit<ItemAttributes>([&](const auto& t) { return t.getAttributes(factory); });
}

PItem ItemType::get(const ContentFactory* factory) const {
  auto attributes = getAttributes(factory);
  for (auto& elem : attributes.modifiers) {
    auto var = factory->attrInfo.at(elem.first).modifierVariation;
    auto& mod = elem.second;
    if (Random.chance(attributes.variationChance) && var > 0)
      mod = max(1, mod + Random.get(-var, var + 1));
  }
  if (attributes.ingredientType)
    attributes.description = "Special crafting ingredient";
  return type->visit<PItem>(
      [&](const ItemTypes::FireScroll&) {
        return makeOwner<FireScrollItem>(std::move(attributes), factory);
      },
      [&](const ItemTypes::Potion&) {
        return makeOwner<PotionItem>(std::move(attributes), factory);
      },
      [&](const ItemTypes::Corpse&) {
        return ::corpse(attributes, "skeleton", factory, false, CorpseInfo{UniqueEntity<Creature>::Id(), false, false, false});
      },
      [&](const auto&) {
        return makeOwner<Item>(std::move(attributes), factory);
      }
  );
}

ViewId getAmuletViewId(LastingOrBuff e) {
  return e.visit(
    [](LastingEffect e) {
      switch (e) {
        case LastingEffect::WARNING: return ViewId("amulet2");
        default: return ViewId("amulet3");
      }
    },
    [](BuffId buff) {
      if (buff == BuffId("REGENERATION"))
        return ViewId("amulet1");
      return ViewId("amulet3");
    }
  );
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

static string getRandomPoemType() {
  return Random.choose(makeVec<string>("poem", "haiku", "sonnet", "limerick"));
}

static string getRandomPoem() {
  return Random.choose(makeVec<string>("bad", "obscene", "vulgar")) + " " + getRandomPoemType();
}

ItemAttributes ItemTypes::Corpse::getAttributes(const ContentFactory*) const {
  return getCorpseAttr("corpse", ItemClass::CORPSE, 100, true, none);
}

ItemAttributes ItemTypes::Poem::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.name = getRandomPoem();
      i.blindName = "scroll"_s;
      i.weight = 0.1;
      i.applyVerb = make_pair("read", "reads");
      i.effect = Effect(Effects::Area{10,
          Effect(Effects::Filter(CreaturePredicates::Enemy{}, Effect(Effects::Lasting{30_visible, BuffId("DEF_DEBUFF")})))});
      i.price = i.effect->getPrice(f);
      i.burnTime = 5;
      i.uses = 1;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = LIST(StorageId("scrolls"), StorageId("equipment"));
  );
}

ItemAttributes ItemTypes::EventPoem::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.shortName = getRandomPoemType();
      i.name = *i.shortName + " about " + eventName;
      i.blindName = "scroll"_s;
      i.weight = 0.1;
      i.applyVerb = make_pair("read", "reads");
      i.effect = Effect(Effects::Area{10,
          Effect(Effects::Filter(CreaturePredicates::Enemy{}, Effect(Effects::Lasting{30_visible, BuffId("DEF_DEBUFF")})))});
      i.price = i.effect->getPrice(f);
      i.burnTime = 5;
      i.uses = 1;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = LIST(StorageId("scrolls"), StorageId("equipment"));
      i.equipmentGroup = "consumables"_s;
  );
}

ItemAttributes ItemTypes::Assembled::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      auto allIds = factory->getCreatures().getViewId(creature);
      i.viewId = allIds.front();
      i.partIds = allIds.getSubsequence(1);
      i.partIds.append(partIds);
      i.assembledMinion = AssembledMinion LIST(creature, traits);
      i.name = itemName;
      i.weight = 1;
      i.uses = 1;
      i.maxUpgrades = maxUpgrades;
      i.upgradeType = upgradeType;
      i.storageIds = LIST(StorageId("automaton_parts"), StorageId("equipment"));
  );
}

ItemAttributes ItemTypes::Intrinsic::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.viewId = viewId;
      i.name = name;
      for (auto& effect : weaponInfo.attackerEffect)
        i.suffixes.push_back("of " + effect.getName(factory));
      for (auto& effect : weaponInfo.victimEffect)
        i.suffixes.push_back("of " + effect.effect.getName(factory));
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[weaponInfo.meleeAttackAttr] = damage;
      i.price = 1;
      i.weaponInfo = weaponInfo;
      i.storageIds = {StorageId("equipment")};
 );
}

ItemAttributes ItemTypes::Ring::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("ring", getColor(lastingEffect, f));
      i.shortName = getName(lastingEffect, f);
      i.equipedEffect.push_back(lastingEffect);
      i.name = "ring of " + *i.shortName;
      i.plural = "rings of " + *i.shortName;
      i.weight = 0.05;
      i.equipmentSlot = EquipmentSlot::RINGS;
      i.price = 40;
      i.storageIds = LIST(StorageId("jewellery"), StorageId("equipment"));
      i.equipmentGroup = "jewellery"_s;
  );
}

ItemAttributes ItemTypes::Amulet::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = getAmuletViewId(lastingEffect);
      i.shortName = getName(lastingEffect, f);
      i.equipedEffect.push_back(lastingEffect);
      i.name = "amulet of " + *i.shortName;
      i.plural = "amulets of " + *i.shortName;
      i.equipmentSlot = EquipmentSlot::AMULET;
      i.price = 5 * getPrice(lastingEffect, f);
      i.weight = 0.3;
      i.storageIds = LIST(StorageId("jewellery"), StorageId("equipment"));
      i.equipmentGroup = "jewellery"_s;
  );
}

ItemAttributes CustomItemId::getAttributes(const ContentFactory* factory) const {
  if (auto ret = getReferenceMaybe(factory->items, *this))
    return *ret;
  else {
    USER_INFO << "Item not found: " << data() << ". Returning a rock.";
    return CustomItemId("Rock").getAttributes(factory);
  }
}

static ItemAttributes getPotionAttr(const ContentFactory* factory, Effect effect, double scale, const char* prefix,
    const char* viewId) {
  effect.scale(scale, factory);
  return ITATTR(
      i.viewId = ViewId(viewId, effect.getColor(factory));
      i.shortName = effect.getName(factory);
      i.name = prefix + "potion of "_s + *i.shortName;
      i.plural = prefix + "potions of "_s + *i.shortName;
      i.blindName = "potion"_s;
      i.applyVerb = make_pair("drink", "drinks");
      i.fragile = true;
      i.weight = 0.3;
      i.effect = effect;
      i.price = i.effect->getPrice(factory) * scale * scale;
      i.burnTime = 1;
      i.effectAppliedWhenThrown = true;
      i.uses = 1;
      i.producedStat = StatId::POTION_PRODUCED;
      i.storageIds = LIST(StorageId("potions"), StorageId("equipment"));
      i.equipmentGroup = "consumables"_s;
  );
}

ItemAttributes ItemTypes::Potion::getAttributes(const ContentFactory* factory) const {
  return getPotionAttr(factory, effect, 1, "", "potion1");
}

ItemAttributes ItemTypes::Potion2::getAttributes(const ContentFactory* factory) const {
  return getPotionAttr(factory, effect, 2, "concentrated ", "potion3");
}

ItemAttributes ItemTypes::PrefixChance::getAttributes(const ContentFactory* factory) const {
  auto attributes = type->getAttributes(factory);
  if (!attributes.genPrefixes.empty() && Random.chance(chance))
    applyPrefix(factory, Random.choose(attributes.genPrefixes), attributes);
  return attributes;
}

static ViewId getMushroomViewId(Effect e) {
  return e.effect->visit<ViewId>(
      [&](const Effects::Lasting& e) {
        return e.lastingEffect.visit(
          [](LastingEffect e) {
            switch (e) {
              case LastingEffect::PANIC: return ViewId("mushroom3");
              case LastingEffect::HALLU: return ViewId("mushroom4");
              case LastingEffect::RAGE: return ViewId("mushroom5");
              case LastingEffect::NIGHT_VISION: return ViewId("mushroom7");
              default: return ViewId("mushroom6");
            }
          },
          [&](BuffId buff) {
            if (buff == BuffId("DAM_BONUS"))
              return ViewId("mushroom1");
            if (buff == BuffId("DEF_BONUS"))
              return ViewId("mushroom2");
            if (buff == BuffId("REGENERATION"))
              return ViewId("mushroom6");
            return ViewId("mushroom6");
          }
        );
      },
      [&](const auto&) {
        return ViewId("mushroom6");
      }
  );
}

ItemAttributes ItemTypes::Mushroom::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.viewId = getMushroomViewId(effect);
      i.shortName = effect.getName(factory);
      i.name = *i.shortName + " mushroom";
      i.blindName = "mushroom"_s;
      i.itemClass= ItemClass::FOOD;
      i.weight = 0.1;
      i.effect = effect;
      i.price = i.effect->getPrice(factory);
      i.uses = 1;
      i.storageIds = {StorageId("equipment")};
      i.equipmentGroup = "consumables"_s;
  );
}

static ViewId getRuneViewId(const string& name) {
  int h = int(combineHash(name));
  const static vector<Color> colors = {Color::RED, Color::YELLOW, Color::ORANGE, Color::BLUE, Color::LIGHT_BLUE,
      Color::PURPLE, Color::PINK, Color::RED, Color::ORANGE, Color::GREEN, Color::LIGHT_GREEN};
  return ViewId("glyph", colors[(h % colors.size() + colors.size()) % colors.size()]);
}

ItemAttributes ItemTypes::Glyph::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.shortName = getGlyphName(factory, *rune.prefix);
      i.viewId = getRuneViewId(*i.shortName);
      i.upgradeInfo = rune;
      i.name = "glyph " + *i.shortName;
      i.plural= "glyphs "  + *i.shortName;
      i.blindName = "glyph"_s;
      i.itemClass = ItemClass::OTHER;
      i.weight = 0.1;
      i.price = 100;
      i.uses = 1;
      i.storageIds = LIST(StorageId("upgrades"), StorageId("equipment"));
  );
}

static ViewId getBalsamViewId(const string& name) {
  int h = int(combineHash(name));
  const static vector<Color> colors = {Color::RED, Color::YELLOW, Color::ORANGE, Color::BLUE, Color::LIGHT_BLUE,
      Color::PURPLE, Color::PINK, Color::RED, Color::ORANGE, Color::GREEN, Color::LIGHT_GREEN};
  return ViewId("potion2", colors[(h % colors.size() + colors.size()) % colors.size()]);
}

ItemAttributes ItemTypes::Balsam::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.shortName = effect.getName(factory);
      i.viewId = getBalsamViewId(*i.shortName);
      i.upgradeInfo = ItemUpgradeInfo LIST(ItemUpgradeType::BALSAM, effect);
      i.name = "balsam of " + *i.shortName;
      i.plural= "balsams of "  + *i.shortName;
      i.blindName = "balsam"_s;
      i.weight = 0.5;
      i.price = 100;
      i.uses = 1;
      i.storageIds = LIST(StorageId("potions"), StorageId("equipment"));
  );
}

ItemAttributes ItemTypes::Scroll::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.shortName = effect.getName(factory);
      i.name = "scroll of " + *i.shortName;
      i.plural= "scrolls of "  + *i.shortName;
      i.blindName = "scroll"_s;
      i.applyVerb = make_pair("read", "reads");
      i.weight = 0.1;
      i.effect = effect;
      i.price = i.effect->getPrice(factory);
      i.burnTime = 5;
      i.uses = 1;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = LIST(StorageId("scrolls"), StorageId("equipment"));
      i.equipmentGroup = "consumables"_s;
  );
}

ItemAttributes ItemTypes::FireScroll::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.name = "scroll of fire";
      i.plural= "scrolls of fire"_s;
      i.shortName = "fire"_s;
      i.description = "Sets itself on fire.";
      i.blindName = "scroll"_s;
      i.applyVerb = make_pair("read", "reads");
      i.weight = 0.1;
      i.price = 15;
      i.burnTime = 10;
      i.uses = 1;
      i.effect = Effect(EffectType::EChain{});
      i.effectDescription = false;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = {StorageId("equipment")};
      i.equipmentGroup = "consumables"_s;
  );
}

ItemAttributes ItemTypes::TechBook::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("book");
      i.shortName = string(techId.data());
      i.name = "book of " + *i.shortName;
      i.plural = "books of " + *i.shortName;
      i.weight = 1;
      i.applyTime = 3_visible;
      i.effect = Effect(techId);
      i.price = 1000;
      i.storageIds = {StorageId("equipment")};
  );
}

SERIALIZE_DEF(ItemType, NAMED(type))


#define VARIANT_TYPES_LIST ITEM_TYPES_LIST
#define VARIANT_NAME ItemTypeVariant

namespace ItemTypes {


#include "gen_variant_serialize.h"
#ifdef MEM_USAGE_TEST
template void serialize(MemUsageArchive&, ItemTypeVariant&);
#endif
}

#include "pretty_archive.h"
template void ItemType::serialize(PrettyInputArchive&, unsigned);

namespace ItemTypes {
#define DEFAULT_ELEM "Simple"

template<>
#include "gen_variant_serialize_pretty.h"
#undef DEFAULT_ELEM
}
