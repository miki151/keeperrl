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

ItemType ItemType::touch(Effect victimEffect, vector<Effect> attackerEffect) {
  return ItemType::Intrinsic{ViewId("touch_attack"), "touch", 0,
      WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, {victimEffect}, attackerEffect, AttackMsg::TOUCH}};
}

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

static ItemType fistsBase(int damage, vector<Effect> effect) {
  return ItemType::Intrinsic{ViewId("fist_attack"), "fists", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, effect, {}, AttackMsg::SWING}};
}

ItemType ItemType::fists(int damage) {
  return fistsBase(damage, {});
}

ItemType ItemType::fists(int damage, Effect effect) {
  return fistsBase(damage, {effect});
}

static ItemType fangsBase(int damage, vector<Effect> effect) {
  return ItemType::Intrinsic{ViewId("bite_attack"), "fangs", damage,
      WeaponInfo{false, AttackType::BITE, AttrType::DAMAGE, effect, {}, AttackMsg::BITE}};
}

ItemType ItemType::fangs(int damage) {
  return fangsBase(damage, {});
}

ItemType ItemType::fangs(int damage, Effect effect) {
  return fangsBase(damage, {effect});
}

ItemType ItemType::spellHit(int damage) {
  return ItemType::Intrinsic{ViewId("fist_attack"), "spell", damage,
      WeaponInfo{false, AttackType::HIT, AttrType::SPELL_DAMAGE, {}, {}, AttackMsg::SPELL}};
}

ItemType::ItemType() {}

ItemType& ItemType::setPrefixChance(double chance) {
  prefixChance = chance;
  return *this;
}

ItemType& ItemType::operator = (const ItemType&) = default;
ItemType& ItemType::operator = (ItemType&&) = default;


class FireScrollItem : public Item {
  public:
  FireScrollItem(const ItemAttributes& attr) : Item(attr) {}

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
      TimeInterval rottingT, CorpseInfo info, bool instantlyRotten) :
      Item(attr),
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
        PCreature vulture = position.getGame()->getContentFactory()->creatures.fromId(CreatureId("VULTURE"), TribeId::getPest(),
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

PItem ItemFactory::corpse(const string& name, const string& rottenName, double weight, bool instantlyRotten,
    ItemClass itemClass, CorpseInfo corpseInfo) {
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
        instantlyRotten);
}

class PotionItem : public Item {
  public:
  PotionItem(const ItemAttributes& attr) : Item(attr) {}

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
  TechBookItem(const ItemAttributes& attr, TechId t) : Item(attr), tech(t) {}

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
  if (!attributes.genPrefixes.empty() && Random.chance(prefixChance))
    applyPrefix(Random.choose(attributes.genPrefixes), attributes);
  return type.visit(
      [&](const FireScroll&) {
        return makeOwner<FireScrollItem>(std::move(attributes));
      },
      [&](const TechBook& t) {
        return makeOwner<TechBookItem>(std::move(attributes), t.techId);
      },
      [&](const Potion& t) {
        return makeOwner<PotionItem>(std::move(attributes));
      },
      [&](const auto&) {
        return makeOwner<Item>(std::move(attributes));
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
      [&](const Effect::EnhanceWeapon&) {
        return 12;
      },
      [&](const Effect::EnhanceArmor&) {
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

static int maybePlusMinusOne(int prob) {
  if (Random.roll(prob))
    return Random.get(2) * 2 - 1;
  return 0;
}

static const vector<pair<string, vector<string>>> badArtifactNames {
  {"sword", { "bang", "crush", "fist"}},
  {"battle axe", {"crush", "tooth", "razor", "fist", "bite", "bolt", "sword"}},
  {"war hammer", {"blade", "tooth", "bite", "bolt", "sword", "steel"}}};

/*static void makeArtifact(ItemAttributes& i) {
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
}*/

vector<PItem> ItemType::get(int num, const ContentFactory* factory) const {
  vector<PItem> ret;
  for (int i : Range(num))
    ret.push_back(get(factory));
  return ret;
}

ItemAttributes ItemType::AutomatonItem::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("trap_item");
      i.name = "automaton";
      i.applyMsgFirstPerson = "assemble the automaton"_s;
      i.applyMsgThirdPerson = "assembles an automaton"_s;
      i.applySound = SoundId::TRAP_ARMING;
      i.weight = 30;
      i.itemClass = ItemClass::TOOL;
      i.applyTime = 3_visible;
      i.uses = 1;
      i.price = 60;
      i.effect = Effect(Effect::Summon{CreatureId("AUTOMATON"), Range(1, 2)});
  );
}

ItemAttributes ItemType::Knife::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("knife");
      i.name = "knife";
      i.plural = "knives"_s;
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
      i.price = 1;
      i.weaponInfo.attackType = AttackType::STAB;
      i.weaponInfo.attackMsg = AttackMsg::THRUST;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::POISON}}});
  );
}

ItemAttributes ItemType::Intrinsic::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = viewId;
      i.name = name;
      for (auto& effect : weaponInfo.victimEffect)
        i.prefixes.push_back(effect.getName());
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

ItemAttributes ItemType::UnicornHorn::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("knife");
      i.name = "horn";
      i.plural = "horn"_s;
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
      i.weaponInfo.victimEffect.push_back(Effect::Lasting{LastingEffect::POISON});
      i.price = 1;
      i.weaponInfo.attackType = AttackType::STAB;
      i.weaponInfo.attackMsg = AttackMsg::THRUST;
  );
}

ItemAttributes ItemType::Spear::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("spear");
      i.name = "spear";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::DAMAGE] = 6 + maybePlusMinusOne(4);
      i.price = 4;
      i.weaponInfo.attackType = AttackType::STAB;
      i.weaponInfo.attackMsg = AttackMsg::THRUST;
  );
}

ItemAttributes ItemType::Sword::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("sword");
      i.name = "sword";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::DAMAGE] = 8 + maybePlusMinusOne(4);
      i.price = 4;
      i.weaponInfo.attackType = AttackType::CUT;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Fire{}}});
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::POISON}}});
      i.genPrefixes.push_back({1, LastingEffect::RAGE});
      i.genPrefixes.push_back({1, JoinPrefixes{{
          ItemPrefix{ItemAttrBonus{AttrType::DAMAGE, 3}},
          ItemPrefix{LastingEffect::HALLU},
      }}});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::AdaSword::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_sword");
      i.name = "adamantine sword";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.2;
      i.modifiers[AttrType::DAMAGE] = 11 + maybePlusMinusOne(4);
      i.price = 20;
      i.weaponInfo.attackType = AttackType::CUT;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Acid{}}});
      i.genPrefixes.push_back({1, VictimEffect{Effect::Fire{}}});
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::POISON}}});
      i.genPrefixes.push_back({1, LastingEffect::RAGE});
  );
}

ItemAttributes ItemType::ElvenSword::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("elven_sword");
      i.name = "elven sword";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1;
      i.modifiers[AttrType::DAMAGE] = 9 + maybePlusMinusOne(4);
      i.price = 8;
      i.weaponInfo.attackType = AttackType::CUT;
      i.genPrefixes.push_back({1, VictimEffect{Effect::SilverDamage{}}});
  );
}

ItemAttributes ItemType::BattleAxe::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("battle_axe");
      i.name = "battle axe";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 8;
      i.modifiers[AttrType::DAMAGE] = 14 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 30;
      i.weaponInfo.attackType = AttackType::CUT;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::BLEEDING}}});
      i.genPrefixes.push_back({1, LastingEffect::RAGE});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::AdaBattleAxe::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_battle_axe");
      i.shortName = "adamantine"_s;
      i.name = "adamantine battle axe";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 7;
      i.modifiers[AttrType::DAMAGE] = 18 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 150;
      i.weaponInfo.attackType = AttackType::CUT;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::BLEEDING}}});
      i.maxUpgrades = 2;
  );
}

ItemAttributes ItemType::WarHammer::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("war_hammer");
      i.name = "war hammer";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 8;
      i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 20;
      i.weaponInfo.attackType = AttackType::CRUSH;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::COLLAPSED}}});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::AdaWarHammer::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_war_hammer");
      i.name = "adamantine war hammer";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 6;
      i.modifiers[AttrType::DAMAGE] = 15 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 100;
      i.weaponInfo.attackType = AttackType::CRUSH;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::COLLAPSED}}});
      i.maxUpgrades = 2;
  );
}

ItemAttributes ItemType::Club::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("club");
      i.name = "club";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 2;
      i.modifiers[AttrType::DAMAGE] = 4 + maybePlusMinusOne(4);
      i.price = 2;
      i.weaponInfo.attackType = AttackType::CRUSH;
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::HeavyClub::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("heavy_club");
      i.name = "heavy club";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 8;
      i.modifiers[AttrType::DAMAGE] = 10 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 4;
      i.weaponInfo.attackType = AttackType::CRUSH;
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::WoodenStaff::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("wooden_staff");
      i.name = "wooden staff";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::SPELL_DAMAGE] = 4 + maybePlusMinusOne(4);
      i.weaponInfo.meleeAttackAttr = AttrType::SPELL_DAMAGE;
      i.price = 30;
      i.weaponInfo.attackType = AttackType::SPELL;
      i.weaponInfo.attackMsg = AttackMsg::WAVE;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Escape{}}});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::IronStaff::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("iron_staff");
      i.name = "iron staff";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::SPELL_DAMAGE] = 8 + maybePlusMinusOne(4);
      i.weaponInfo.meleeAttackAttr = AttrType::SPELL_DAMAGE;
      i.price = 60;
      i.weaponInfo.attackType = AttackType::SPELL;
      i.weaponInfo.attackMsg = AttackMsg::WAVE;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Escape{}}});
      i.genPrefixes.push_back({1, VictimEffect{Effect::DestroyEquipment{}}});
      i.genPrefixes.push_back({1, JoinPrefixes{{
          ItemPrefix{ItemAttrBonus{AttrType::SPELL_DAMAGE, 20}},
          ItemPrefix{AttackerEffect{Effect::Suicide{}}}
      }}});
      i.maxUpgrades = 2;
  );
}

ItemAttributes ItemType::GoldenStaff::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("golden_staff");
      i.name = "golden staff";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 2.5;
      i.modifiers[AttrType::SPELL_DAMAGE] = 11 + maybePlusMinusOne(4);
      i.weaponInfo.meleeAttackAttr = AttrType::SPELL_DAMAGE;
      i.price = 180;
      i.weaponInfo.attackType = AttackType::SPELL;
      i.weaponInfo.attackMsg = AttackMsg::WAVE;
      i.genPrefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::INSANITY}}});
      i.genPrefixes.push_back({1, VictimEffect{Effect::DestroyEquipment{}}});
      i.genPrefixes.push_back({1, VictimEffect{Effect::Fire{}}});
      i.maxUpgrades = 4;
  );
}

ItemAttributes ItemType::Scythe::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("sword");
      i.name = "scythe";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 5;
      i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 20;
      i.weaponInfo.attackType = AttackType::CUT;
  );
}

ItemAttributes ItemType::ElvenBow::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("elven_bow");
      i.itemClass = ItemClass::RANGED_WEAPON;
      i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
      i.rangedWeapon = RangedWeapon(AttrType::RANGED_DAMAGE, "arrow", ViewId("arrow"), 12);
      i.weaponInfo.twoHanded = true;
      i.weight = 1;
      i.modifiers[AttrType::RANGED_DAMAGE] = 16;
      i.name = "silver elven bow";
      i.price = 100;
  );
}

ItemAttributes ItemType::Bow::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("bow");
      i.name = "short bow";
      i.itemClass = ItemClass::RANGED_WEAPON;
      i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
      i.rangedWeapon = RangedWeapon(AttrType::RANGED_DAMAGE, "arrow", ViewId("arrow"), 10);
      i.weaponInfo.twoHanded = true;
      i.weight = 1;
      i.modifiers[AttrType::RANGED_DAMAGE] = 10 + maybePlusMinusOne(4);
      i.price = 12;
  );
}

ItemAttributes ItemType::Torch::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("hand_torch");
      i.itemClass = ItemClass::TOOL;
      i.weight = 1;
      i.ownedEffect = LastingEffect::LIGHT_SOURCE;
      i.name = "hand torch";
      i.plural = "hand torches"_s;
      i.price = 2;
  );
}

ItemAttributes ItemType::Robe::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("robe");
      i.name = "robe";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 2;
      i.price = 10;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::MAGIC_RESISTANCE});
  );
}

ItemAttributes ItemType::HalloweenCostume::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("halloween_costume");
      i.name = "halloween costume";
      i.shortName = "halloween costume"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 1;
      i.price = 10;
      i.modifiers[AttrType::DEFENSE] = 1;
  );
}

ItemAttributes ItemType::BagOfCandies::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("bag_of_candy");
      i.shortName = "candies"_s;
      i.name = "bag of candies";
      i.blindName = "bag"_s;
      i.description = "Really, just a bag of candies.";
      i.itemClass= ItemClass::FOOD;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -15;
      i.price = 1;
      i.uses = 1;
  );
}

ItemAttributes ItemType::LeatherGloves::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("leather_gloves");
      i.shortName = "gloves"_s;
      i.name = "pair of leather gloves";
      i.plural = "pairs of leather gloves"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::GLOVES;
      i.weight = 0.3;
      i.price = 2;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::IronGloves::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("iron_gloves");
      i.shortName = "gloves"_s;
      i.name = "pair of iron gloves";
      i.plural = "pairs of iron gloves"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::GLOVES;
      i.weight = 1;
      i.price = 10;
      i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::DAMAGE, Random.get(2, 5)}});
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::SPELL_DAMAGE, Random.get(2, 5)}});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::AdaGloves::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_gloves");
      i.shortName = "gloves"_s;
      i.name = "pair of adamantine gloves";
      i.plural = "pairs of adamantine gloves"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::GLOVES;
      i.weight = 0.7;
      i.price = 25;
      i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::DAMAGE, Random.get(2, 5)}});
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::SPELL_DAMAGE, Random.get(2, 5)}});
      i.maxUpgrades = 2;
  );
}

ItemAttributes ItemType::LeatherArmor::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("leather_armor");
      i.shortName = "armor"_s;
      i.name = "leather armor";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 7;
      i.price = 4;
      i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::LeatherHelm::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("leather_helm");
      i.shortName = "helm"_s;
      i.name = "leather helm";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::HELMET;
      i.weight = 1.5;
      i.price = 1;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::TELEPATHY});
      i.genPrefixes.push_back({1, LastingEffect::SLEEP_RESISTANT});
      i.genPrefixes.push_back({2, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::WoodenShield::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("wooden_shield");
      i.shortName = "wooden"_s;
      i.name = "wooden shield";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::SHIELD;
      i.weight = 2;
      i.price = 1;
      i.damageReduction = 0.1;
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::ChainArmor::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("chain_armor");
      i.shortName = "armor"_s;
      i.name = "chain armor";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 15;
      i.price = 25;
      i.modifiers[AttrType::DEFENSE] = 5 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 2;
  );
}

ItemAttributes ItemType::AdaArmor::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_armor");
      i.shortName = "armor"_s;
      i.name = "adamantine armor";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 13;
      i.price = 160;
      i.modifiers[AttrType::DEFENSE] = 8 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 4;
  );
}

ItemAttributes ItemType::IronHelm::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("iron_helm");
      i.shortName = "helm"_s;
      i.name = "iron helm";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::HELMET;
      i.weight = 4;
      i.price = 8;
      i.modifiers[AttrType::DEFENSE]= 2 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::WARNING});
      i.genPrefixes.push_back({1, LastingEffect::SLEEP_RESISTANT});
      i.genPrefixes.push_back({3, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::AdaHelm::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_helm");
      i.shortName = "helm"_s;
      i.name = "adamantine helm";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::HELMET;
      i.weight = 3;
      i.price = 40;
      i.modifiers[AttrType::DEFENSE]= 4 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::TELEPATHY});
      i.genPrefixes.push_back({1, LastingEffect::REGENERATION});
      i.maxUpgrades = 2;
  );
}

ItemAttributes ItemType::LeatherBoots::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("leather_boots");
      i.shortName = "boots"_s;
      i.name = "pair of leather boots";
      i.plural = "pairs of leather boots"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BOOTS;
      i.weight = 2;
      i.price = 2;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::FLYING});
      i.genPrefixes.push_back({1, LastingEffect::SPEED});
      i.genPrefixes.push_back({2, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 0;
  );
}

ItemAttributes ItemType::IronBoots::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("iron_boots");
      i.shortName = "boots"_s;
      i.name = "pair of iron boots";
      i.plural = "pairs of iron boots"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BOOTS;
      i.weight = 4;
      i.price = 8;
      i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::FLYING});
      i.genPrefixes.push_back({1, LastingEffect::FIRE_RESISTANT});
      i.genPrefixes.push_back({3, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
      i.maxUpgrades = 1;
  );
}

ItemAttributes ItemType::AdaBoots::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_boots");
      i.shortName = "boots"_s;
      i.name = "pair of adamantine boots";
      i.plural = "pairs of admantine boots"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BOOTS;
      i.weight = 3;
      i.price = 50;
      i.modifiers[AttrType::DEFENSE] = 4 + maybePlusMinusOne(4);
      i.genPrefixes.push_back({1, LastingEffect::SPEED});
      i.maxUpgrades = 2;
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

ItemAttributes ItemType::DefenseAmulet::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("amulet3");
      i.shortName = "defense"_s;
      i.name = "amulet of " + *i.shortName;
      i.plural = "amulets of " + *i.shortName;
      i.description = "Increases the toughness of your skin and flesh, making you harder to wound.";
      i.itemClass = ItemClass::AMULET;
      i.equipmentSlot = EquipmentSlot::AMULET;
      i.price = 60;
      i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);
      i.weight = 0.3;
  );
}

ItemAttributes ItemType::FirstAidKit::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("first_aid");
      i.name = "first aid kit";
      i.weight = 0.5;
      i.itemClass = ItemClass::TOOL;
      i.description = "Heals your wounds, but requires a few turns to apply.";
      i.applyTime = 3_visible;
      i.uses = Random.get(3, 6);
      i.usedUpMsg = true;
      i.displayUses = true;
      i.price = 2;
      i.effect = {Effect::Heal{}};
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

ItemAttributes ItemType::Rock::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("rock");
      i.name = "rock";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::STONE;
      i.weight = 0.3;
  );
}

ItemAttributes ItemType::IronOre::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("iron_rock");
      i.name = "iron ore";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::IRON;
      i.weight = 0.5;
  );
}

ItemAttributes ItemType::AdaOre::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("ada_ore");
      i.name = "adamantine ore";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::ADA;
      i.weight = 0.5;
  );
}

ItemAttributes ItemType::WoodPlank::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("wood_plank");
      i.name = "wood plank";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::WOOD;
      i.weight = 5;
  );
}

ItemAttributes ItemType::Bone::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("bone");
      i.name = "bone";
      i.itemClass = ItemClass::CORPSE;
      i.price = 0;
      i.weight = 5;
  );
}

ItemAttributes ItemType::GoldPiece::getAttributes(const ContentFactory*) const {
  return ITATTR(
      i.viewId = ViewId("gold");
      i.name = "gold piece";
      i.itemClass = ItemClass::GOLD;
      i.price = 1;
      i.resourceId = CollectiveResourceId::GOLD;
      i.weight = 0.01;
  );
}

SERIALIZE_DEF(ItemType, NAMED(type), OPTION(prefixChance))

#include "pretty_archive.h"
template void ItemType::serialize(PrettyInputArchive&, unsigned);
