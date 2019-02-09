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
#include "trap_type.h"
#include "furniture_type.h"
#include "resource_id.h"
#include "game_event.h"

ItemType::ItemType(const ItemType&) = default;
ItemType::ItemType(ItemType&) = default;
ItemType::ItemType(ItemType&&) = default;

ItemType ItemType::touch(Effect victimEffect, optional<Effect> attackerEffect) {
  return ItemType::Intrinsic{ViewId::TOUCH_ATTACK, "touch", 0,
      WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, victimEffect, attackerEffect, AttackMsg::TOUCH}};
}

ItemType ItemType::legs(int damage) {
  return ItemType::Intrinsic{ViewId::LEG_ATTACK, "legs", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, none, none, AttackMsg::KICK}};
}

ItemType ItemType::claws(int damage) {
  return ItemType::Intrinsic{ViewId::CLAWS_ATTACK, "claws", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, none, none, AttackMsg::CLAW}};
}

ItemType ItemType::beak(int damage) {
  return ItemType::Intrinsic{ViewId::BEAK_ATTACK, "beak", damage,
        WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, none, none, AttackMsg::BITE}};
}

static ItemType fistsBase(int damage, optional<Effect> effect) {
  return ItemType::Intrinsic{ViewId::FIST_ATTACK, "fists", damage,
      WeaponInfo{false, AttackType::HIT, AttrType::DAMAGE, effect, none, AttackMsg::SWING}};
}

ItemType ItemType::fists(int damage) {
  return fistsBase(damage, none);
}

ItemType ItemType::fists(int damage, Effect effect) {
  return fistsBase(damage, effect);
}

static ItemType fangsBase(int damage, optional<Effect> effect) {
  return ItemType::Intrinsic{ViewId::BITE_ATTACK, "fangs", damage,
      WeaponInfo{false, AttackType::BITE, AttrType::DAMAGE, effect, none, AttackMsg::BITE}};
}

ItemType ItemType::fangs(int damage) {
  return fangsBase(damage, none);
}

ItemType ItemType::fangs(int damage, Effect effect) {
  return fangsBase(damage, effect);
}

ItemType ItemType::spellHit(int damage) {
  return ItemType::Intrinsic{ViewId::FIST_ATTACK, "spell", damage,
      WeaponInfo{false, AttackType::HIT, AttrType::SPELL_DAMAGE, none, none, AttackMsg::SPELL}};
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
    fireDamage(0.03, c->getPosition());
    set = true;
  }

  virtual void specialTick(Position position) override {
    if (set) {
      fireDamage(0.03, position);
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
    if (time >= rottenTime && !rotten)
      makeRotten();
    else {
      if (!rotten && getWeight() > 10 && Random.roll(20 + (*rottenTime - time).getDouble() / 10)
          && getClass() != ItemClass::FOOD)
        Effect::emitPoisonGas(position, 0.3, false);
      if (getWeight() > 10 && !corpseInfo.isSkeleton &&
          !position.isCovered() && Random.roll(350)) {
        for (Position v : position.neighbors8(Random)) {
          PCreature vulture = position.getGame()->getCreatureFactory()->fromId("VULTURE", TribeId::getPest(),
                    MonsterAIFactory::scavengerBird(v));
          if (v.canEnter(vulture.get())) {
            v.addCreature(std::move(vulture));
            v.globalMessage("A vulture lands near " + getTheName());
            *rottenTime -= 40_visible;
            break;
          }
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
        ViewObject(ViewId::BONE, ViewLayer::ITEM, rottenName),
        ITATTR(
          i.viewId = ViewId::BODY_PART;
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


ItemAttributes ItemType::getAttributes() const {
  return type.visit([&](const auto& t) { return t.getAttributes(); });
}

PItem ItemType::get() const {
  auto attributes = getAttributes();
  if (!attributes.prefixes.empty() && Random.chance(prefixChance))
    applyPrefix(Random.choose(attributes.prefixes), attributes);
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
      [&](const Effect::Teleport&) {
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
      [&](const Effect::CurePoison&) {
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
    case LastingEffect::FIRE_RESISTANT: return ViewId::FIRE_RESIST_RING;
    case LastingEffect::POISON_RESISTANT: return ViewId::POISON_RESIST_RING;
    default: return ViewId::FIRE_RESIST_RING;
  }
}

ViewId getAmuletViewId(LastingEffect e) {
  switch (e) {
    case LastingEffect::REGENERATION: return ViewId::AMULET1;
    case LastingEffect::WARNING: return ViewId::AMULET2;
    default: return ViewId::AMULET3;
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

vector<PItem> ItemType::get(int num) const {
  vector<PItem> ret;
  for (int i : Range(num))
    ret.push_back(get());
  return ret;
}

ItemAttributes ItemType::AutomatonItem::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::TRAP_ITEM;
      i.name = "automaton";
      i.applyMsgFirstPerson = "assemble the automaton"_s;
      i.applyMsgThirdPerson = "assembles an automaton"_s;
      i.applySound = SoundId::TRAP_ARMING;
      i.weight = 30;
      i.itemClass = ItemClass::TOOL;
      i.applyTime = 3_visible;
      i.uses = 1;
      i.price = 60;
      i.effect = Effect(Effect::Summon{"AUTOMATON", Range(1, 2)});
  );
}

ItemAttributes ItemType::Knife::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::KNIFE;
      i.name = "knife";
      i.plural = "knives"_s;
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
      i.price = 1;
      i.weaponInfo.attackType = AttackType::STAB;
      i.weaponInfo.attackMsg = AttackMsg::THRUST;
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::POISON}}});
  );
}

ItemAttributes ItemType::Intrinsic::getAttributes() const {
  return ITATTR(
      i.viewId = viewId;
      i.name = name;
      if (auto& effect = weaponInfo.victimEffect)
        i.prefix = effect->getName();
      else if (auto& effect = weaponInfo.attackerEffect)
        i.prefix = effect->getName();
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[weaponInfo.meleeAttackAttr] = damage;
      i.price = 1;
      i.weaponInfo = weaponInfo;
  );
}

ItemAttributes ItemType::UnicornHorn::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::KNIFE;
      i.name = "horn";
      i.plural = "horn"_s;
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 0.3;
      i.modifiers[AttrType::DAMAGE] = 5 + maybePlusMinusOne(4);
      i.weaponInfo.victimEffect = Effect(Effect::Lasting{LastingEffect::POISON});
      i.price = 1;
      i.weaponInfo.attackType = AttackType::STAB;
      i.weaponInfo.attackMsg = AttackMsg::THRUST;
  );
}

ItemAttributes ItemType::Spear::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::SPEAR;
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

ItemAttributes ItemType::Sword::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::SWORD;
      i.name = "sword";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::DAMAGE] = 8 + maybePlusMinusOne(4);
      i.price = 4;
      i.weaponInfo.attackType = AttackType::CUT;
      i.prefixes.push_back({1, VictimEffect{Effect::Fire{}}});
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::POISON}}});
      i.prefixes.push_back({1, LastingEffect::RAGE});
      i.prefixes.push_back({1, JoinPrefixes{{
          ItemPrefix{ItemAttrBonus{AttrType::DAMAGE, 3}},
          ItemPrefix{LastingEffect::HALLU},
      }}});
  );
}

ItemAttributes ItemType::AdaSword::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_SWORD;
      i.name = "adamantine sword";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.2;
      i.modifiers[AttrType::DAMAGE] = 11 + maybePlusMinusOne(4);
      i.price = 20;
      i.weaponInfo.attackType = AttackType::CUT;
      i.prefixes.push_back({1, VictimEffect{Effect::Acid{}}});
      i.prefixes.push_back({1, VictimEffect{Effect::Fire{}}});
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::POISON}}});
      i.prefixes.push_back({1, LastingEffect::RAGE});
  );
}

ItemAttributes ItemType::ElvenSword::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ELVEN_SWORD;
      i.name = "elven sword";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1;
      i.modifiers[AttrType::DAMAGE] = 9 + maybePlusMinusOne(4);
      i.price = 8;
      i.weaponInfo.attackType = AttackType::CUT;
      i.prefixes.push_back({1, VictimEffect{Effect::SilverDamage{}}});
  );
}

ItemAttributes ItemType::BattleAxe::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::BATTLE_AXE;
      i.name = "battle axe";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 8;
      i.modifiers[AttrType::DAMAGE] = 14 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 30;
      i.weaponInfo.attackType = AttackType::CUT;
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::BLEEDING}}});
      i.prefixes.push_back({1, LastingEffect::RAGE});
  );
}

ItemAttributes ItemType::AdaBattleAxe::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_BATTLE_AXE;
      i.shortName = "adamantine"_s;
      i.name = "adamantine battle axe";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 7;
      i.modifiers[AttrType::DAMAGE] = 18 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 150;
      i.weaponInfo.attackType = AttackType::CUT;
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::BLEEDING}}});
  );
}

ItemAttributes ItemType::WarHammer::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::WAR_HAMMER;
      i.name = "war hammer";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 8;
      i.modifiers[AttrType::DAMAGE] = 12 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 20;
      i.weaponInfo.attackType = AttackType::CRUSH;
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::COLLAPSED}}});
  );
}

ItemAttributes ItemType::AdaWarHammer::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_WAR_HAMMER;
      i.name = "adamantine war hammer";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 6;
      i.modifiers[AttrType::DAMAGE] = 15 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 100;
      i.weaponInfo.attackType = AttackType::CRUSH;
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::COLLAPSED}}});
  );
}

ItemAttributes ItemType::Club::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::CLUB;
      i.name = "club";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 2;
      i.modifiers[AttrType::DAMAGE] = 4 + maybePlusMinusOne(4);
      i.price = 2;
      i.weaponInfo.attackType = AttackType::CRUSH;
  );
}

ItemAttributes ItemType::HeavyClub::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::HEAVY_CLUB;
      i.name = "heavy club";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 8;
      i.modifiers[AttrType::DAMAGE] = 10 + maybePlusMinusOne(4);
      i.weaponInfo.twoHanded = true;
      i.price = 4;
      i.weaponInfo.attackType = AttackType::CRUSH;
  );
}

ItemAttributes ItemType::WoodenStaff::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::WOODEN_STAFF;
      i.name = "wooden staff";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::SPELL_DAMAGE] = 4 + maybePlusMinusOne(4);
      i.weaponInfo.meleeAttackAttr = AttrType::SPELL_DAMAGE;
      i.price = 30;
      i.weaponInfo.attackType = AttackType::SPELL;
      i.weaponInfo.attackMsg = AttackMsg::WAVE;
      i.prefixes.push_back({1, VictimEffect{Effect::Teleport{}}});
  );
}

ItemAttributes ItemType::IronStaff::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::IRON_STAFF;
      i.name = "iron staff";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 1.5;
      i.modifiers[AttrType::SPELL_DAMAGE] = 8 + maybePlusMinusOne(4);
      i.weaponInfo.meleeAttackAttr = AttrType::SPELL_DAMAGE;
      i.price = 60;
      i.weaponInfo.attackType = AttackType::SPELL;
      i.weaponInfo.attackMsg = AttackMsg::WAVE;
      i.prefixes.push_back({1, VictimEffect{Effect::Teleport{}}});
      i.prefixes.push_back({1, VictimEffect{Effect::DestroyEquipment{}}});
      i.prefixes.push_back({1, JoinPrefixes{{
          ItemPrefix{ItemAttrBonus{AttrType::SPELL_DAMAGE, 20}},
          ItemPrefix{AttackerEffect{Effect::Suicide{}}}
      }}});
  );
}

ItemAttributes ItemType::GoldenStaff::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::GOLDEN_STAFF;
      i.name = "golden staff";
      i.itemClass = ItemClass::WEAPON;
      i.equipmentSlot = EquipmentSlot::WEAPON;
      i.weight = 2.5;
      i.modifiers[AttrType::SPELL_DAMAGE] = 11 + maybePlusMinusOne(4);
      i.weaponInfo.meleeAttackAttr = AttrType::SPELL_DAMAGE;
      i.price = 180;
      i.weaponInfo.attackType = AttackType::SPELL;
      i.weaponInfo.attackMsg = AttackMsg::WAVE;
      i.prefixes.push_back({1, VictimEffect{Effect::Lasting{LastingEffect::INSANITY}}});
      i.prefixes.push_back({1, VictimEffect{Effect::DestroyEquipment{}}});
      i.prefixes.push_back({1, VictimEffect{Effect::Fire{}}});
  );
}

ItemAttributes ItemType::Scythe::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::SWORD;
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

ItemAttributes ItemType::ElvenBow::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ELVEN_BOW;
      i.itemClass = ItemClass::RANGED_WEAPON;
      i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
      i.rangedWeapon = RangedWeapon(AttrType::RANGED_DAMAGE, "arrow", ViewId::ARROW, 12);
      i.weaponInfo.twoHanded = true;
      i.weight = 1;
      i.modifiers[AttrType::RANGED_DAMAGE] = 16;
      i.name = "silver elven bow";
      i.price = 100;
  );
}

ItemAttributes ItemType::Bow::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::BOW;
      i.name = "short bow";
      i.itemClass = ItemClass::RANGED_WEAPON;
      i.equipmentSlot = EquipmentSlot::RANGED_WEAPON;
      i.rangedWeapon = RangedWeapon(AttrType::RANGED_DAMAGE, "arrow", ViewId::ARROW, 10);
      i.weaponInfo.twoHanded = true;
      i.weight = 1;
      i.modifiers[AttrType::RANGED_DAMAGE] = 10 + maybePlusMinusOne(4);
      i.price = 12;
  );
}

ItemAttributes ItemType::Torch::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::HAND_TORCH;
      i.itemClass = ItemClass::TOOL;
      i.weight = 1;
      i.ownedEffect = LastingEffect::LIGHT_SOURCE;
      i.name = "hand torch";
      i.plural = "hand torches"_s;
      i.price = 2;
  );
}

ItemAttributes ItemType::Robe::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ROBE;
      i.name = "robe";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 2;
      i.price = 10;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::MAGIC_RESISTANCE});
  );
}

ItemAttributes ItemType::HalloweenCostume::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::HALLOWEEN_COSTUME;
      i.name = "halloween costume";
      i.shortName = "halloween costume"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 1;
      i.price = 10;
      i.modifiers[AttrType::DEFENSE] = 1;
  );
}

ItemAttributes ItemType::BagOfCandies::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::BAG_OF_CANDY;
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

ItemAttributes ItemType::LeatherGloves::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::LEATHER_GLOVES;
      i.shortName = "gloves"_s;
      i.name = "pair of leather gloves";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::GLOVES;
      i.weight = 0.3;
      i.price = 2;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
  );
}

ItemAttributes ItemType::IronGloves::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::IRON_GLOVES;
      i.shortName = "gloves"_s;
      i.name = "pair of iron gloves";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::GLOVES;
      i.weight = 1;
      i.price = 10;
      i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::DAMAGE, Random.get(2, 5)}});
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::SPELL_DAMAGE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::AdaGloves::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_GLOVES;
      i.shortName = "gloves"_s;
      i.name = "pair of adamantine gloves";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::GLOVES;
      i.weight = 0.7;
      i.price = 25;
      i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::DAMAGE, Random.get(2, 5)}});
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::SPELL_DAMAGE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::LeatherArmor::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::LEATHER_ARMOR;
      i.shortName = "armor"_s;
      i.name = "leather armor";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 7;
      i.price = 4;
      i.modifiers[AttrType::DEFENSE] = 3 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::LeatherHelm::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::LEATHER_HELM;
      i.shortName = "helm"_s;
      i.name = "leather helm";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::HELMET;
      i.weight = 1.5;
      i.price = 1;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::TELEPATHY});
      i.prefixes.push_back({1, LastingEffect::SLEEP_RESISTANT});
      i.prefixes.push_back({2, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::ChainArmor::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::CHAIN_ARMOR;
      i.shortName = "armor"_s;
      i.name = "chain armor";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 15;
      i.price = 25;
      i.modifiers[AttrType::DEFENSE] = 5 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::AdaArmor::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_ARMOR;
      i.shortName = "armor"_s;
      i.name = "adamantine armor";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BODY_ARMOR;
      i.weight = 13;
      i.price = 160;
      i.modifiers[AttrType::DEFENSE] = 8 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::IronHelm::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::IRON_HELM;
      i.shortName = "helm"_s;
      i.name = "iron helm";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::HELMET;
      i.weight = 4;
      i.price = 8;
      i.modifiers[AttrType::DEFENSE]= 2 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::WARNING});
      i.prefixes.push_back({1, LastingEffect::SLEEP_RESISTANT});
      i.prefixes.push_back({3, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::AdaHelm::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_HELM;
      i.shortName = "helm"_s;
      i.name = "adamantine helm";
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::HELMET;
      i.weight = 3;
      i.price = 40;
      i.modifiers[AttrType::DEFENSE]= 4 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::TELEPATHY});
      i.prefixes.push_back({1, LastingEffect::REGENERATION});
  );
}

ItemAttributes ItemType::LeatherBoots::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::LEATHER_BOOTS;
      i.shortName = "boots"_s;
      i.name = "pair of leather boots";
      i.plural = "pairs of leather boots"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BOOTS;
      i.weight = 2;
      i.price = 2;
      i.modifiers[AttrType::DEFENSE] = 1 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::FLYING});
      i.prefixes.push_back({1, LastingEffect::SPEED});
      i.prefixes.push_back({2, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::IronBoots::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::IRON_BOOTS;
      i.shortName = "boots"_s;
      i.name = "pair of iron boots";
      i.plural = "pairs of iron boots"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BOOTS;
      i.weight = 4;
      i.price = 8;
      i.modifiers[AttrType::DEFENSE] = 2 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::FLYING});
      i.prefixes.push_back({1, LastingEffect::FIRE_RESISTANT});
      i.prefixes.push_back({3, ItemAttrBonus{AttrType::DEFENSE, Random.get(2, 5)}});
  );
}

ItemAttributes ItemType::AdaBoots::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_BOOTS;
      i.shortName = "boots"_s;
      i.name = "pair of adamantine boots";
      i.plural = "pairs of admantine boots"_s;
      i.itemClass = ItemClass::ARMOR;
      i.equipmentSlot = EquipmentSlot::BOOTS;
      i.weight = 3;
      i.price = 50;
      i.modifiers[AttrType::DEFENSE] = 4 + maybePlusMinusOne(4);
      i.prefixes.push_back({1, LastingEffect::SPEED});
  );
}

ItemAttributes ItemType::Ring::getAttributes() const {
  return ITATTR(
      i.viewId = getRingViewId(lastingEffect);
      i.shortName = LastingEffects::getName(lastingEffect);
      i.equipedEffect = lastingEffect;
      i.name = "ring of " + *i.shortName;
      i.plural = "rings of " + *i.shortName;
      i.weight = 0.05;
      i.equipmentSlot = EquipmentSlot::RINGS;
      i.itemClass = ItemClass::RING;
      i.price = 40;
  );
}

ItemAttributes ItemType::Amulet::getAttributes() const {
  return ITATTR(
      i.viewId = getAmuletViewId(lastingEffect);
      i.shortName = LastingEffects::getName(lastingEffect);
      i.equipedEffect = lastingEffect;
      i.name = "amulet of " + *i.shortName;
      i.plural = "amulets of " + *i.shortName;
      i.itemClass = ItemClass::AMULET;
      i.equipmentSlot = EquipmentSlot::AMULET;
      i.price = 5 * LastingEffects::getPrice(lastingEffect);
      i.weight = 0.3;
  );
}

ItemAttributes ItemType::DefenseAmulet::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::AMULET3;
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

ItemAttributes ItemType::FirstAidKit::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::FIRST_AID;
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

ItemAttributes ItemType::TrapItem::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::TRAP_ITEM;
      auto trapName = getTrapName(trapType);
      i.name = "unarmed " + trapName + " trap";
      i.shortName = trapName;
      i.weight = 0.5;
      i.itemClass = ItemClass::TOOL;
      i.applyTime = 3_visible;
      i.applySound = SoundId::TRAP_ARMING;
      i.uses = 1;
      i.usedUpMsg = true;
      i.trapType = trapType;
      i.effect = Effect(Effect::PlaceFurniture{getTrapFurniture(trapType)});
      i.price = 2;
  );
}

ItemAttributes ItemType::Potion::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::POTION1;
      i.shortName = effect.getName();
      i.name = "potion of " + *i.shortName;
      i.plural = "potions of " + *i.shortName;
      i.blindName = "potion"_s;
      i.itemClass = ItemClass::POTION;
      i.fragile = true;
      i.weight = 0.3;
      i.effect = effect;
      i.price = getEffectPrice(effect);
      i.flamability = 0.3;
      i.uses = 1;
  );
}

static ViewId getMushroomViewId(Effect e) {
  return e.visit(
      [&](const Effect::Lasting& e) {
        switch (e.lastingEffect) {
          case LastingEffect::DAM_BONUS: return ViewId::MUSHROOM1;
          case LastingEffect::DEF_BONUS: return ViewId::MUSHROOM2;
          case LastingEffect::PANIC: return ViewId::MUSHROOM3;
          case LastingEffect::HALLU: return ViewId::MUSHROOM4;
          case LastingEffect::RAGE: return ViewId::MUSHROOM5;
          case LastingEffect::REGENERATION: return ViewId::MUSHROOM6;
          case LastingEffect::NIGHT_VISION: return ViewId::MUSHROOM7;
          default: return ViewId::MUSHROOM6;
        }
      },
      [&](const auto&) {
        return ViewId::MUSHROOM6;
      }
  );
}

ItemAttributes ItemType::Mushroom::getAttributes() const {
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

ItemAttributes ItemType::Scroll::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::SCROLL;
      i.shortName = effect.getName();
      i.name = "scroll of " + *i.shortName;
      i.plural= "scrolls of "  + *i.shortName;
      i.blindName = "scroll"_s;
      i.itemClass = ItemClass::SCROLL;
      i.weight = 0.1;
      i.modifiers[AttrType::DAMAGE] = -10;
      i.effect = effect;
      i.price = getEffectPrice(effect);
      i.flamability = 1;
      i.uses = 1;
  );
}

ItemAttributes ItemType::FireScroll::getAttributes() const {
  return ITATTR(
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
      i.uses = 1;
  );
}

ItemAttributes ItemType::TechBook::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::BOOK;
      i.shortName = techId;
      i.name = "book of " + *i.shortName;
      i.plural = "books of " + *i.shortName;
      i.weight = 1;
      i.itemClass = ItemClass::BOOK;
      i.applyTime = 3_visible;
      i.price = 1000;
  );
}

ItemAttributes ItemType::Rock::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ROCK;
      i.name = "rock";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::STONE;
      i.weight = 0.3;
  );
}

ItemAttributes ItemType::IronOre::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::IRON_ROCK;
      i.name = "iron ore";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::IRON;
      i.weight = 0.5;
  );
}

ItemAttributes ItemType::AdaOre::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::ADA_ORE;
      i.name = "adamantine ore";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::ADA;
      i.weight = 0.5;
  );
}

ItemAttributes ItemType::WoodPlank::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::WOOD_PLANK;
      i.name = "wood plank";
      i.itemClass = ItemClass::OTHER;
      i.price = 0;
      i.resourceId = CollectiveResourceId::WOOD;
      i.weight = 5;
  );
}

ItemAttributes ItemType::Bone::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::BONE;
      i.name = "bone";
      i.itemClass = ItemClass::CORPSE;
      i.price = 0;
      i.weight = 5;
  );
}

ItemAttributes ItemType::GoldPiece::getAttributes() const {
  return ITATTR(
      i.viewId = ViewId::GOLD;
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
