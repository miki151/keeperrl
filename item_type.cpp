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
  return ItemType(ItemTypes::Intrinsic{ViewId("leg_attack"), TStringId("LEGS"), damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::KICK}});
}

ItemType ItemType::claws(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("claws_attack"), TStringId("CLAWS_ATTACK"), damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::CLAW}});
}

ItemType ItemType::beak(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("beak_attack"), TStringId("BEAK_ATTACK"), damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::BITE}});
}

ItemType ItemType::fists(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("fist_attack"), TStringId("FISTS_ATTACK"), damage,
        WeaponInfo{false, AttackType::HIT, AttrType("DAMAGE"), {}, {}, AttackMsg::SWING}});
}

static ItemType fangsBase(int damage, vector<ItemPrefixes::VictimEffect> effect) {
  return ItemType(ItemTypes::Intrinsic{ViewId("bite_attack"), TStringId("FANGS_ATTACK"), damage,
      WeaponInfo{false, AttackType::BITE, AttrType("DAMAGE"), std::move(effect), {}, AttackMsg::BITE}});
}

ItemType ItemType::fangs(int damage) {
  return fangsBase(damage, {});
}

ItemType ItemType::fangs(int damage, ItemPrefixes::VictimEffect effect) {
  return fangsBase(damage, {std::move(effect)});
}

ItemType ItemType::spellHit(int damage) {
  return ItemType(ItemTypes::Intrinsic{ViewId("fist_attack"), TStringId("SPELL"), damage,
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
  FireScrollItem(SItemAttributes attr, const ContentFactory* f) : Item(attr, f) {}

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
  Corpse(const ViewObject& obj2, SItemAttributes attr, const TString& rottenN,
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
          v.globalMessage(TSentence("VULTURE_LANDS_NEAR", getTheName()));
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
  TString SERIAL(rottenName);
  CorpseInfo SERIAL(corpseInfo);
};

static PItem corpse(SItemAttributes attr, const TString& rottenName, const ContentFactory* f,
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

static TString getBodyPartBone(BodyPart part) {
  switch (part) {
    case BodyPart::HEAD: return TStringId("SKULL");
    default: return TStringId("BONE");
  }
}

static TString getBodyPartName(const TString& creatureName, BodyPart part) {
  return TSentence("BODY_PART_OF", creatureName, getName(part));
}

PItem ItemType::severedLimb(const TString& creatureName, BodyPart part, double weight, ItemClass itemClass,
    const ContentFactory* factory) {
  return corpse(getBodyPartName(creatureName, part), TSentence("BODY_PART_OF", creatureName, getBodyPartBone(part)),
        weight / 8, factory, false, itemClass);

}

static SItemAttributes getCorpseAttr(const TString& name, ItemClass itemClass, double weight, bool isResource,
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

PItem ItemType::corpse(const TString& name, const TString& rottenName, double weight, const ContentFactory* f,
    bool instantlyRotten, ItemClass itemClass, CorpseInfo corpseInfo, optional<string> ingredient) {
  return ::corpse(getCorpseAttr(name, itemClass, weight, corpseInfo.canBeRevived, ingredient),
      rottenName, f, instantlyRotten, corpseInfo);
}

class PotionItem : public Item {
  public:
  PotionItem(SItemAttributes attr, const ContentFactory* f) : Item(attr, f) {}

  virtual void fireDamage(Position position) override {
    heat += 0.3;
//    INFO << getName() << " heat " << heat;
    if (heat >= 1.0) {
      position.globalMessage(TSentence("BOILS_AND_EXPLODES", getAName()));
      discarded = true;
    }
  }

  virtual void iceDamage(Position position) override {
    position.globalMessage(TSentence("FREEZES_AND_EXPLODES", getAName()));
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


SItemAttributes ItemType::getAttributes(const ContentFactory* factory) const {
  return type->visit<SItemAttributes>([&](const auto& t) { return t.getAttributes(factory); });
}

PItem ItemType::get(const ContentFactory* factory) const {
  auto attributes = getAttributes(factory);
  for (auto& elem : attributes->modifiers) {
    auto var = factory->attrInfo.at(elem.first).modifierVariation;
    auto& mod = elem.second;
    if (Random.chance(attributes->variationChance) && var > 0)
      mod = max(1, mod + Random.get(-var, var + 1));
  }
  if (attributes->ingredientType && attributes->description.empty())
    attributes->description = TStringId("SPECIAL_CRAFTING_INGREDIENT");
  return type->visit<PItem>(
      [&](const ItemTypes::FireScroll&) {
        return makeOwner<FireScrollItem>(std::move(attributes), factory);
      },
      [&](const ItemTypes::Potion&) {
        return makeOwner<PotionItem>(std::move(attributes), factory);
      },
      [&](const ItemTypes::Corpse&) {
        return ::corpse(attributes, TStringId("SKELETON"), factory, false, CorpseInfo{UniqueEntity<Creature>::Id(), false, false, false});
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

static TString getRandomPoemType() {
  return Random.choose(makeVec<TString>(TStringId("POEM"), TStringId("HAIKU"), TStringId("SONNET"),
      TStringId("LIMERICK")));
}

static TString getRandomPoem() {
  return TSentence("POEM_WITH_TYPE",
      Random.choose(makeVec<TStringId>(TStringId("BAD_POEM_TYPE"), TStringId("OBSCENE"), TStringId("VULGAR"))),
      getRandomPoemType());
}

SItemAttributes ItemTypes::Corpse::getAttributes(const ContentFactory*) const {
  return getCorpseAttr(TStringId("CORPSE"), ItemClass::CORPSE, 100, true, none);
}

SItemAttributes ItemTypes::Poem::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.name = getRandomPoem();
      i.blindName = TString(TStringId("SCROLL_ITEM"));
      i.weight = 0.1;
      i.applyVerb = make_pair(TStringId("YOU_READ"), TStringId("HE_READS"));
      i.effect = Effect(Effects::Area{10,
          Effect(Effects::Filter(CreaturePredicates::Enemy{}, Effect(Effects::Lasting{30_visible, BuffId("DEF_DEBUFF")})))});
      i.price = i.effect->getPrice(f);
      i.burnTime = 5;
      i.uses = 1;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = LIST(StorageId("scrolls"), StorageId("equipment"));
  );
}

SItemAttributes ItemTypes::EventPoem::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.shortName = getRandomPoemType();
      i.name = TSentence("POEM_ABOUT", *i.shortName, eventName);
      i.blindName = TString(TStringId("SCROLL_ITEM"));
      i.weight = 0.1;
      i.applyVerb = make_pair(TStringId("YOU_READ"), TStringId("HE_READS"));
      i.effect = Effect(Effects::Area{10,
          Effect(Effects::Filter(CreaturePredicates::Enemy{}, Effect(Effects::Lasting{30_visible, BuffId("DEF_DEBUFF")})))});
      i.price = i.effect->getPrice(f);
      i.burnTime = 5;
      i.uses = 1;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = LIST(StorageId("scrolls"), StorageId("equipment"));
      i.equipmentGroup = TString(TStringId("CONSUMABLES"));
  );
}

SItemAttributes ItemTypes::Assembled::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      auto allIds = factory->getCreatures().getViewId(creature);
      i.viewId = allIds.front();
      i.partIds = allIds.getSubsequence(1);
      i.partIds.append(partIds);
      i.assembledMinion = AssembledMinion LIST(creature, traits);
      i.name = !itemName.empty() ? itemName : factory->getCreatures().getName(creature);
      i.weight = 1;
      i.uses = 1;
      i.maxUpgrades = maxUpgrades;
      i.upgradeType = upgradeType;
      i.storageIds = LIST(StorageId("automaton_parts"), StorageId("equipment"));
  );
}

SItemAttributes ItemTypes::Intrinsic::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.viewId = viewId;
      i.name = name;
      for (auto& effect : weaponInfo.attackerEffect)
        i.suffixes.push_back(TSentence("OF_SUFFIX", effect.getName(factory)));
      for (auto& effect : weaponInfo.victimEffect)
        i.suffixes.push_back(TSentence("OF_SUFFIX", effect.effect.getName(factory)));
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[weaponInfo.meleeAttackAttr] = damage;
      i.price = 1;
      i.weaponInfo = weaponInfo;
      i.storageIds = {StorageId("equipment")};
 );
}

SItemAttributes ItemTypes::Ring::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("ring", getColor(lastingEffect, f));
      i.shortName = getName(lastingEffect, f);
      i.equipedEffect.push_back(lastingEffect);
      i.name = TSentence("RING_OF", *i.shortName);
      i.weight = 0.05;
      i.equipmentSlot = EquipmentSlot::RINGS;
      i.price = 40;
      i.storageIds = LIST(StorageId("jewellery"), StorageId("equipment"));
      i.equipmentGroup = TString(TSentence("JEWELLERY_GROUP"));
  );
}

SItemAttributes ItemTypes::Amulet::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = getAmuletViewId(lastingEffect);
      i.shortName = getName(lastingEffect, f);
      i.equipedEffect.push_back(lastingEffect);
      i.name = TSentence("AMULET_OF", *i.shortName);
      i.equipmentSlot = EquipmentSlot::AMULET;
      i.price = 5 * getPrice(lastingEffect, f);
      i.weight = 0.3;
      i.storageIds = LIST(StorageId("jewellery"), StorageId("equipment"));
      i.equipmentGroup = TString(TSentence("JEWELLERY_GROUP"));
  );
}

SItemAttributes CustomItemId::getAttributes(const ContentFactory* factory) const {
  if (auto ret = getReferenceMaybe(factory->items, *this)) {
    if (!!(*ret)->resourceId)
      return *ret;
    else
      return make_shared<ItemAttributes>(**ret);
  } else {
    USER_INFO << "Item not found: " << data() << ". Returning a rock.";
    return CustomItemId("Rock").getAttributes(factory);
  }
}

static SItemAttributes getPotionAttr(const ContentFactory* factory, Effect effect, double scale, TString prefix,
    const char* viewId) {
  effect.scale(scale, factory);
  return ITATTR(
      i.viewId = ViewId(viewId, effect.getColor(factory));
      i.shortName = effect.getName(factory);
      i.name =  TSentence("POTION_OF", prefix, *i.shortName);
      i.blindName = TString(TStringId("POTION"));
      i.applyVerb = make_pair(TStringId("YOU_DRINK"), TStringId("DRINKS"));
      i.fragile = true;
      i.weight = 0.3;
      i.effect = effect;
      i.price = i.effect->getPrice(factory) * scale * scale;
      i.burnTime = 1;
      i.effectAppliedWhenThrown = true;
      i.uses = 1;
      i.producedStat = StatId::POTION_PRODUCED;
      i.storageIds = LIST(StorageId("potions"), StorageId("equipment"));
      i.equipmentGroup = TString(TSentence("CONSUMABLES_GROUP"));
  );
}

SItemAttributes ItemTypes::Potion::getAttributes(const ContentFactory* factory) const {
  return getPotionAttr(factory, effect, 1, ""_s, "potion1");
}

SItemAttributes ItemTypes::Potion2::getAttributes(const ContentFactory* factory) const {
  return getPotionAttr(factory, effect, 2, TStringId("CONCENTRATED"), "potion3");
}

SItemAttributes ItemTypes::PrefixChance::getAttributes(const ContentFactory* factory) const {
  auto attributes = type->getAttributes(factory);
  if (!attributes->genPrefixes.empty() && Random.chance(chance))
    applyPrefix(factory, Random.choose(attributes->genPrefixes), *attributes);
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

SItemAttributes ItemTypes::Mushroom::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.viewId = getMushroomViewId(effect);
      i.shortName = effect.getName(factory);
      i.name = TSentence("MUSHROOM_OF", *i.shortName);
      i.blindName = TString(TStringId("MUSHROOM"));
      i.itemClass= ItemClass::FOOD;
      i.weight = 0.1;
      i.effect = effect;
      i.price = i.effect->getPrice(factory);
      i.uses = 1;
      i.storageIds = {StorageId("equipment")};
      i.equipmentGroup = TString(TSentence("CONSUMABLES_GROUP"));
  );
}

static ViewId getRuneViewId(const TString& name) {
  int h = int(combineHash(name));
  const static vector<Color> colors = {Color::RED, Color::YELLOW, Color::ORANGE, Color::BLUE, Color::LIGHT_BLUE,
      Color::PURPLE, Color::PINK, Color::RED, Color::ORANGE, Color::GREEN, Color::LIGHT_GREEN};
  return ViewId("glyph", colors[(h % colors.size() + colors.size()) % colors.size()]);
}

SItemAttributes ItemTypes::Glyph::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.shortName = getGlyphName(factory, *rune.prefix);
      i.viewId = getRuneViewId(*i.shortName);
      i.upgradeInfo = rune;
      i.name = TSentence("GLYPH", *i.shortName);
      i.blindName = TString(TStringId("GLYPH_BLIND"));
      i.itemClass = ItemClass::OTHER;
      i.weight = 0.1;
      i.price = 100;
      i.uses = 1;
      i.storageIds = LIST(StorageId("upgrades"), StorageId("equipment"));
  );
}

static ViewId getBalsamViewId(const TString& name) {
  int h = int(combineHash(name));
  const static vector<Color> colors = {Color::RED, Color::YELLOW, Color::ORANGE, Color::BLUE, Color::LIGHT_BLUE,
      Color::PURPLE, Color::PINK, Color::RED, Color::ORANGE, Color::GREEN, Color::LIGHT_GREEN};
  return ViewId("potion2", colors[(h % colors.size() + colors.size()) % colors.size()]);
}

SItemAttributes ItemTypes::Balsam::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.shortName = effect.getName(factory);
      i.viewId = getBalsamViewId(*i.shortName);
      i.upgradeInfo = ItemUpgradeInfo LIST(ItemUpgradeType::BALSAM, effect);
      i.name = TSentence("BALSAM_OF", *i.shortName);
      i.blindName = TString(TStringId("BALSAM"));
      i.weight = 0.5;
      i.price = 100;
      i.uses = 1;
      i.storageIds = LIST(StorageId("potions"), StorageId("equipment"));
  );
}

SItemAttributes ItemTypes::Scroll::getAttributes(const ContentFactory* factory) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.shortName = effect.getName(factory);
      i.name = TSentence("SCROLL_OF", *i.shortName);
      i.blindName = TString(TStringId("SCROLL"));
      i.applyVerb = make_pair(TStringId("YOU_READ"), TStringId("HE_READS"));
      i.weight = 0.1;
      i.effect = effect;
      i.price = i.effect->getPrice(factory);
      i.burnTime = 5;
      i.uses = 1;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = LIST(StorageId("scrolls"), StorageId("equipment"));
      i.equipmentGroup = TString(TSentence("CONSUMABLES_GROUP"));
  );
}

SItemAttributes ItemTypes::FireScroll::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("scroll");
      i.name = TStringId("SCROLL_OF_FIRE");
      i.shortName = TString(TStringId("FIRE"));
      i.description = TString(TStringId("FIRE_SCROLL_DESCRIPTION"));
      i.blindName = TString(TStringId("SCROLL"));
      i.applyVerb = make_pair(TStringId("YOU_READ"), TStringId("HE_READS"));
      i.weight = 0.1;
      i.price = 15;
      i.burnTime = 10;
      i.uses = 1;
      i.effect = Effect(EffectType::Chain{});
      i.effectDescription = false;
      i.applyPredicate = CreaturePredicate(CreaturePredicates::Not{CreaturePredicate(LastingEffect::BLIND)});
      i.storageIds = {StorageId("equipment")};
      i.equipmentGroup = TString(TSentence("CONSUMABLES_GROUP"));
  );
}

SItemAttributes ItemTypes::TechBook::getAttributes(const ContentFactory* f) const {
  return ITATTR(
      i.viewId = ViewId("book");
      i.shortName = f->technology.getName(techId);
      i.name = TSentence("BOOK_OF", *i.shortName);
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
