#include "stdafx.h"
#include "lasting_effect.h"
#include "creature.h"
#include "view_object.h"
#include "player_message.h"
#include "creature_attributes.h"
#include "body.h"
#include "furniture.h"
#include "level.h"
#include "fx_variant_name.h"
#include "fx_name.h"
#include "fx_info.h"
#include "game.h"

static optional<LastingEffect> getCancelledOneWay(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::POISON_RESISTANT:
      return LastingEffect::POISON;
    case LastingEffect::FIRE_RESISTANT:
      return LastingEffect::ON_FIRE;
    case LastingEffect::SLEEP_RESISTANT:
      return LastingEffect::SLEEP;
    case LastingEffect::REGENERATION:
      return LastingEffect::BLEEDING;
    default:
      return none;
  }
}

static optional<LastingEffect> getCancelledBothWays(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::PANIC:
      return LastingEffect::RAGE;
    case LastingEffect::SPEED:
      return LastingEffect::SLOWED;
    case LastingEffect::PEACEFULNESS:
      return LastingEffect::INSANITY;
    default:
      return none;
  }
}

static optional<LastingEffect> getCancelled(LastingEffect effect) {
  static EnumMap<LastingEffect, optional<LastingEffect>> ret(
    [&](LastingEffect e) -> optional<LastingEffect> {
      if (auto other = getCancelledOneWay(e))
        return *other;
      if (auto other = getCancelledBothWays(e))
        return *other;
      for (auto other : ENUM_ALL(LastingEffect))
        if (getCancelledBothWays(other) == e)
          return other;
      return none;
    }
  );
  return ret[effect];
}

static optional<LastingEffect> getCancelling(LastingEffect effect) {
  static EnumMap<LastingEffect, optional<LastingEffect>> ret(
    [&](LastingEffect e) -> optional<LastingEffect> {
      for (auto other : ENUM_ALL(LastingEffect))
        if (getCancelled(other) == e)
          return other;
      return none;
    }
  );
  return ret[effect];
}

static optional<ViewObject::Modifier> getViewObjectModifier(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVISIBLE:
      return ViewObjectModifier::INVISIBLE;
    default:
      return none;
  }
}

void LastingEffects::onAffected(Creature* c, LastingEffect effect, bool msg) {
  PROFILE;
  if (auto e = getCancelled(effect))
    c->removeEffect(*e, true);
  if (auto mod = getViewObjectModifier(effect))
    c->modViewObject().setModifier(*mod);
  if (msg)
    switch (effect) {
      case LastingEffect::FLYING:
        c->you(MsgType::ARE, "flying!");
        break;
      case LastingEffect::BLEEDING:
        c->secondPerson("You start bleeding");
        c->thirdPerson(c->getName().the() + " starts bleeding");
        break;
      case LastingEffect::COLLAPSED:
        c->you(MsgType::COLLAPSE);
        break;
      case LastingEffect::PREGNANT:
        c->you(MsgType::ARE, "pregnant!");
        break;
      case LastingEffect::STUNNED:
        c->you(MsgType::ARE, "knocked out");
        break;
      case LastingEffect::PANIC:
        c->you(MsgType::PANIC, "");
        break;
      case LastingEffect::RAGE:
        c->you(MsgType::RAGE, "");
        break;
      case LastingEffect::HALLU:
        if (!c->isAffected(LastingEffect::BLIND))
          c->privateMessage("The world explodes into colors!");
        else
          c->privateMessage("You feel as if a party has started without you.");
        break;
      case LastingEffect::BLIND:
        c->you(MsgType::ARE, "blind!");
        break;
      case LastingEffect::INVISIBLE:
        if (!c->isAffected(LastingEffect::BLIND))
          c->you(MsgType::TURN_INVISIBLE, "");
        else
          c->privateMessage("You feel like a child again.");
        break;
      case LastingEffect::POISON:
        c->you(MsgType::ARE, "poisoned");
        break;
      case LastingEffect::DAM_BONUS:
        c->you(MsgType::FEEL, "more dangerous");
        break;
      case LastingEffect::DEF_BONUS:
        c->you(MsgType::FEEL, "more protected");
        break;
      case LastingEffect::SPEED:
        c->you(MsgType::ARE, "moving faster");
        break;
      case LastingEffect::SLOWED:
        c->you(MsgType::ARE, "moving more slowly");
        break;
      case LastingEffect::SLEEP_RESISTANT:
        c->you(MsgType::ARE, "now sleep resistant");
        break;
      case LastingEffect::TIED_UP:
        c->you(MsgType::ARE, "tied up");
        break;
      case LastingEffect::ENTANGLED:
        c->you(MsgType::ARE, "entangled in a web");
        break;
      case LastingEffect::SLEEP:
        c->you(MsgType::FALL_ASLEEP, "");
        break;
      case LastingEffect::POISON_RESISTANT:
        c->you(MsgType::ARE, "now poison resistant");
        break;
      case LastingEffect::FIRE_RESISTANT:
        c->you(MsgType::ARE, "now fire resistant");
        break;
      case LastingEffect::INSANITY:
        c->you(MsgType::BECOME, "insane");
        break;
      case LastingEffect::PEACEFULNESS:
        c->you(MsgType::BECOME, "peaceful");
        break;
      case LastingEffect::LIGHT_SOURCE:
        c->getPosition().addCreatureLight(false);
        break;
      case LastingEffect::DARKNESS_SOURCE:
        c->getPosition().addCreatureLight(true);
        break;
      case LastingEffect::MAGIC_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to magical attacks");
        break;
      case LastingEffect::MELEE_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to melee attacks");
        break;
      case LastingEffect::RANGED_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to ranged attacks");
        break;
      case LastingEffect::MAGIC_VULNERABILITY:
        c->you(MsgType::ARE, "now vulnerable to magical attacks");
        break;
      case LastingEffect::MELEE_VULNERABILITY:
        c->you(MsgType::ARE, "now vulnerable to melee attacks");
        break;
      case LastingEffect::RANGED_VULNERABILITY:
        c->you(MsgType::ARE, "now vulnerable to ranged attacks");
        break;
      case LastingEffect::ELF_VISION:
        c->you("can see through trees");
        break;
      case LastingEffect::NIGHT_VISION:
        c->you("can see in the dark");
        break;
      case LastingEffect::REGENERATION:
        c->you(MsgType::ARE, "regenerating");
        break;
      case LastingEffect::WARNING:
        c->you(MsgType::FEEL, "more aware of danger");
        break;
      case LastingEffect::TELEPATHY:
        c->you(MsgType::ARE, "telepathic");
        break;
      case LastingEffect::SUNLIGHT_VULNERABLE:
        c->you(MsgType::ARE, "vulnerable to sunlight");
        break;
      case LastingEffect::SATIATED:
        c->you(MsgType::ARE, "satiated");
        break;
      case LastingEffect::RESTED:
        c->you(MsgType::ARE, "well rested");
        break;
      case LastingEffect::SUMMONED:
        c->you(MsgType::YOUR, "days are numbered");
        break;
      case LastingEffect::HATE_HUMANS:
      case LastingEffect::HATE_GREENSKINS:
      case LastingEffect::HATE_ELVES:
      case LastingEffect::HATE_UNDEAD:
      case LastingEffect::HATE_DWARVES:
        c->you(MsgType::FEEL, "full of hatred");
        break;
      case LastingEffect::FAST_TRAINING:
        c->you(MsgType::FEEL, "like working out");
        break;
      case LastingEffect::FAST_CRAFTING:
        c->you(MsgType::FEEL, "like doing some work");
        break;
      case LastingEffect::SLOW_TRAINING:
        c->you(MsgType::FEEL, "lazy");
        break;
      case LastingEffect::SLOW_CRAFTING:
        c->you(MsgType::FEEL, "lazy");
        break;
      case LastingEffect::ENTERTAINER:
        c->you(MsgType::FEEL, "like cracking a joke");
        break;
      case LastingEffect::BAD_BREATH:
        c->you(MsgType::YOUR, "breath stinks!");
        break;
      case LastingEffect::ON_FIRE:
        c->you(MsgType::ARE, "on fire!");
        c->getPosition().addCreatureLight(false);
        break;
      case LastingEffect::AMBUSH_SKILL:
      case LastingEffect::STEALING_SKILL:
      case LastingEffect::SWIMMING_SKILL:
      case LastingEffect::DISARM_TRAPS_SKILL:
      case LastingEffect::CONSUMPTION_SKILL:
      case LastingEffect::COPULATION_SKILL:
      case LastingEffect::CROPS_SKILL:
      case LastingEffect::SPIDER_SKILL:
      case LastingEffect::EXPLORE_SKILL:
      case LastingEffect::EXPLORE_CAVES_SKILL:
      case LastingEffect::EXPLORE_NOCTURNAL_SKILL:
      case LastingEffect::BRIDGE_BUILDING_SKILL:
      case LastingEffect::NAVIGATION_DIGGING_SKILL:
        c->verb("acquire", "acquires", "the skill of "_s + getName(effect));
        break;
      case LastingEffect::DISAPPEAR_DURING_DAY:
        c->you(MsgType::YOUR, "hours are numbered");
        break;
      case LastingEffect::NO_CARRY_LIMIT:
        c->you(MsgType::YOUR, "carrying capacity increases");
        break;
    }
}

bool LastingEffects::affects(const Creature* c, LastingEffect effect) {
  if (auto cancelling = getCancelling(effect))
    if (c->isAffected(*cancelling))
      return false;
  return true;
}

optional<LastingEffect> LastingEffects::getSuppressor(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::COLLAPSED:
      return LastingEffect::FLYING;
    case LastingEffect::SUNLIGHT_VULNERABLE:
      return LastingEffect::DARKNESS_SOURCE;
    case LastingEffect::FLYING:
      return LastingEffect::SLEEP;
    default:
      return none;
  }
}

void LastingEffects::onRemoved(Creature* c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, "cured from poisoning");
      break;
    case LastingEffect::SLEEP:
      c->you(MsgType::WAKE_UP, "");
      break;
    case LastingEffect::STUNNED:
      c->you(MsgType::ARE, "no longer unconscious");
      break;
    case LastingEffect::ON_FIRE:
      c->you(MsgType::YOUR, "flames are extinguished");
      c->getPosition().removeCreatureLight(false);
      break;
    default:
      onTimedOut(c, effect, msg); break;
  }
}

void LastingEffects::onTimedOut(Creature* c, LastingEffect effect, bool msg) {
  if (auto mod = getViewObjectModifier(effect))
    c->modViewObject().setModifier(*mod, false);
  if (msg)
    switch (effect) {
      case LastingEffect::SLOWED:
        c->you(MsgType::ARE, "moving faster again");
        break;
      case LastingEffect::SLEEP:
        c->you(MsgType::WAKE_UP, "");
        c->addEffect(LastingEffect::RESTED, 1000_visible);
        break;
      case LastingEffect::SPEED:
        c->you(MsgType::ARE, "moving more slowly again");
        break;
      case LastingEffect::DAM_BONUS:
        c->you(MsgType::ARE, "less dangerous again");
        break;
      case LastingEffect::DEF_BONUS:
        c->you(MsgType::ARE, "less protected again");
        break;
      case LastingEffect::PANIC:
      case LastingEffect::RAGE:
      case LastingEffect::HALLU:
        c->you(MsgType::YOUR, "mind is clear again");
        break;
      case LastingEffect::ENTANGLED:
        c->you(MsgType::BREAK_FREE, "the web");
        break;
      case LastingEffect::TIED_UP:
        c->you(MsgType::BREAK_FREE, "");
        break;
      case LastingEffect::BLEEDING:
        c->you(MsgType::YOUR, "bleeding stops");
        break;
      case LastingEffect::BLIND:
        c->you("can see again");
        break;
      case LastingEffect::INVISIBLE:
        c->you(MsgType::TURN_VISIBLE, "");
        break;
      case LastingEffect::SLEEP_RESISTANT:
        c->you(MsgType::ARE, "no longer sleep resistant");
        break;
      case LastingEffect::POISON:
        c->you(MsgType::ARE, "no longer poisoned");
        break;
      case LastingEffect::POISON_RESISTANT:
        c->you(MsgType::ARE, "no longer poison resistant");
        break;
      case LastingEffect::FIRE_RESISTANT:
        c->you(MsgType::ARE, "no longer fire resistant");
        break;
      case LastingEffect::FLYING:
        c->you(MsgType::FALL, "on the " + c->getPosition().getName());
        break;
      case LastingEffect::COLLAPSED:
        c->you(MsgType::STAND_UP);
        break;
      case LastingEffect::INSANITY:
        c->you(MsgType::BECOME, "sane again");
        break;
      case LastingEffect::PEACEFULNESS:
        c->you(MsgType::BECOME, "less peaceful");
        break;
      case LastingEffect::MAGIC_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to magical attacks");
        break;
      case LastingEffect::MELEE_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to melee attacks");
        break;
      case LastingEffect::RANGED_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to ranged attacks");
        break;
      case LastingEffect::MAGIC_VULNERABILITY:
        c->you(MsgType::FEEL, "less vulnerable to magical attacks");
        break;
      case LastingEffect::MELEE_VULNERABILITY:
        c->you(MsgType::FEEL, "less vulnerable to melee attacks");
        break;
      case LastingEffect::RANGED_VULNERABILITY:
        c->you(MsgType::FEEL, "less vulnerable to ranged attacks");
        break;
      case LastingEffect::ELF_VISION:
        c->you("can't see through trees anymore");
        break;
      case LastingEffect::NIGHT_VISION:
        c->you("can't see in the dark anymore");
        break;
      case LastingEffect::REGENERATION:
        c->you(MsgType::ARE, "no longer regenerating");
        break;
      case LastingEffect::WARNING:
        c->you(MsgType::FEEL, "less aware of danger");
        break;
      case LastingEffect::TELEPATHY:
        c->you(MsgType::ARE, "no longer telepathic");
        break;
      case LastingEffect::SUNLIGHT_VULNERABLE:
        c->you(MsgType::ARE, "no longer vulnerable to sunlight");
        break;
      case LastingEffect::SATIATED:
        c->you(MsgType::ARE, "no longer satiated");
        break;
      case LastingEffect::RESTED:
        c->you(MsgType::ARE, "no longer rested");
        break;
      case LastingEffect::SUMMONED:
        c->dieNoReason(Creature::DropType::ONLY_INVENTORY);
        break;
      case LastingEffect::STUNNED:
        c->dieWithLastAttacker();
        break;
      case LastingEffect::LIGHT_SOURCE:
        c->getPosition().removeCreatureLight(false);
        break;
      case LastingEffect::DARKNESS_SOURCE:
        c->getPosition().removeCreatureLight(true);
        break;
      case LastingEffect::HATE_HUMANS:
      case LastingEffect::HATE_GREENSKINS:
      case LastingEffect::HATE_ELVES:
      case LastingEffect::HATE_UNDEAD:
      case LastingEffect::HATE_DWARVES:
        c->you(MsgType::YOUR, "hatred is gone");
        break;
      case LastingEffect::ENTERTAINER:
        c->you(MsgType::ARE, "no longer funny");
        break;
      case LastingEffect::BAD_BREATH:
        c->you(MsgType::YOUR, "breath is back to normal");
        break;
      case LastingEffect::ON_FIRE:
        c->getPosition().removeCreatureLight(false);
        c->verb("burn", "burns", "to death");
        c->dieNoReason(Creature::DropType::ONLY_INVENTORY);
        break;
      case LastingEffect::AMBUSH_SKILL:
      case LastingEffect::STEALING_SKILL:
      case LastingEffect::SWIMMING_SKILL:
      case LastingEffect::DISARM_TRAPS_SKILL:
      case LastingEffect::CONSUMPTION_SKILL:
      case LastingEffect::COPULATION_SKILL:
      case LastingEffect::CROPS_SKILL:
      case LastingEffect::SPIDER_SKILL:
      case LastingEffect::EXPLORE_SKILL:
      case LastingEffect::EXPLORE_CAVES_SKILL:
      case LastingEffect::EXPLORE_NOCTURNAL_SKILL:
      case LastingEffect::BRIDGE_BUILDING_SKILL:
      case LastingEffect::NAVIGATION_DIGGING_SKILL:
        c->verb("lose", "loses", "the skill of "_s + getName(effect));
        break;
      case LastingEffect::DISAPPEAR_DURING_DAY:
        break;
      case LastingEffect::NO_CARRY_LIMIT:
        c->you(MsgType::YOUR, "carrying capacity decreases");
        break;
      default:
        break;
    }
}

static const int attrBonus = 3;

int LastingEffects::getAttrBonus(const Creature* c, AttrType type) {
  int value = 0;
  switch (type) {
    case AttrType::DAMAGE:
      if (c->isAffected(LastingEffect::PANIC))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::RAGE))
        value += attrBonus;
      if (c->isAffected(LastingEffect::DAM_BONUS))
        value += attrBonus;
      break;
    case AttrType::DEFENSE:
      if (c->isAffected(LastingEffect::PANIC))
        value += attrBonus;
      if (c->isAffected(LastingEffect::RAGE))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::SLEEP))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::DEF_BONUS))
        value += attrBonus;
      if (c->isAffected(LastingEffect::SATIATED))
        value += 1;
      if (c->isAffected(LastingEffect::RESTED))
        value += 1;
      break;
    default: break;
  }
  return value;
}

namespace {
  struct Adjective {
    string name;
    bool bad;
  };
  Adjective operator "" _bad(const char* n, size_t) {\
    return {n, true};
  }

  Adjective operator "" _good(const char* n, size_t) {\
    return {n, false};
  }
}

static const char* getHatedGroupName(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::HATE_UNDEAD: return "undead";
    case LastingEffect::HATE_DWARVES: return "dwarves";
    case LastingEffect::HATE_HUMANS: return "humans";
    case LastingEffect::HATE_ELVES: return "elves";
    case LastingEffect::HATE_GREENSKINS: return "greenskins";
    default:
      return nullptr;
  }
}

static const vector<LastingEffect>& getHateEffects() {
  static vector<LastingEffect> ret = [] {
    vector<LastingEffect> ret;
    for (auto effect : ENUM_ALL(LastingEffect))
      if (getHatedGroupName(effect))
        ret.push_back(effect);
    return ret;
  }();
  return ret;
}

static optional<Adjective> getAdjective(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVISIBLE: return "Invisible"_good;
    case LastingEffect::PANIC: return "Panic"_good;
    case LastingEffect::RAGE: return "Enraged"_good;
    case LastingEffect::HALLU: return "Hallucinating"_good;
    case LastingEffect::DAM_BONUS: return "Damage bonus"_good;
    case LastingEffect::DEF_BONUS: return "Defense bonus"_good;
    case LastingEffect::SLEEP_RESISTANT: return "Sleep resistant"_good;
    case LastingEffect::SPEED: return "Speed bonus"_good;
    case LastingEffect::POISON_RESISTANT: return "Poison resistant"_good;
    case LastingEffect::FIRE_RESISTANT: return "Fire resistant"_good;
    case LastingEffect::FLYING: return "Flying"_good;
    case LastingEffect::LIGHT_SOURCE: return "Source of light"_good;
    case LastingEffect::DARKNESS_SOURCE: return "Source of darkness"_good;
    case LastingEffect::PREGNANT: return "Pregnant"_good;
    case LastingEffect::MAGIC_RESISTANCE: return "Resistant to magical attacks"_good;
    case LastingEffect::MELEE_RESISTANCE: return "Resistant to melee attacks"_good;
    case LastingEffect::RANGED_RESISTANCE: return "Resistant to ranged attacks"_good;
    case LastingEffect::ELF_VISION: return "Can see through trees"_good;
    case LastingEffect::NIGHT_VISION: return "Can see in the dark"_good;
    case LastingEffect::REGENERATION: return "Regenerating"_good;
    case LastingEffect::WARNING: return "Aware of danger"_good;
    case LastingEffect::TELEPATHY: return "Telepathic"_good;
    case LastingEffect::SATIATED: return "Satiated"_good;
    case LastingEffect::RESTED: return "Rested"_good;
    case LastingEffect::PEACEFULNESS: return "Peaceful"_good;
    case LastingEffect::FAST_CRAFTING: return "Fast craftsman"_good;
    case LastingEffect::FAST_TRAINING: return "Fast trainee"_good;
    case LastingEffect::ENTERTAINER: return "Entertainer"_good;
    case LastingEffect::AMBUSH_SKILL: return "Ambusher"_good;
    case LastingEffect::STEALING_SKILL: return "Thief"_good;
    case LastingEffect::SWIMMING_SKILL: return "Swimmer"_good;
    case LastingEffect::DISARM_TRAPS_SKILL: return "Disarms traps"_good;
    case LastingEffect::CONSUMPTION_SKILL: return "Absorbs"_good;
    case LastingEffect::COPULATION_SKILL: return "Copulates"_good;
    case LastingEffect::CROPS_SKILL: return "Farmer"_good;
    case LastingEffect::SPIDER_SKILL: return "Weaves spider webs"_good;
    case LastingEffect::EXPLORE_SKILL: return "Explores"_good;
    case LastingEffect::EXPLORE_NOCTURNAL_SKILL: return "Explores at night"_good;
    case LastingEffect::EXPLORE_CAVES_SKILL: return "Explores caves"_good;
    case LastingEffect::BRIDGE_BUILDING_SKILL: return "Builds bridges"_good;
    case LastingEffect::NAVIGATION_DIGGING_SKILL: return "Digs"_good;
    case LastingEffect::NO_CARRY_LIMIT: return "Infinite carrying capacity"_good;

    case LastingEffect::POISON: return "Poisoned"_bad;
    case LastingEffect::BLEEDING: return "Bleeding"_bad;
    case LastingEffect::SLEEP: return "Sleeping"_bad;
    case LastingEffect::ENTANGLED: return "Entangled"_bad;
    case LastingEffect::TIED_UP: return "Tied up"_bad;
    case LastingEffect::SLOWED: return "Slowed"_bad;
    case LastingEffect::INSANITY: return "Insane"_bad;
    case LastingEffect::BLIND: return "Blind"_bad;
    case LastingEffect::STUNNED: return "Unconscious"_bad;
    case LastingEffect::COLLAPSED: return "Collapsed"_bad;
    case LastingEffect::MAGIC_VULNERABILITY: return "Vulnerable to magical attacks"_bad;
    case LastingEffect::MELEE_VULNERABILITY: return "Vulnerable to melee attacks"_bad;
    case LastingEffect::RANGED_VULNERABILITY: return "Vulnerable to ranged attacks"_bad;
    case LastingEffect::SUNLIGHT_VULNERABLE: return "Vulnerable to sunlight"_bad;
    case LastingEffect::SUMMONED: return "Time to live"_bad;
    case LastingEffect::HATE_UNDEAD:
    case LastingEffect::HATE_DWARVES:
    case LastingEffect::HATE_HUMANS:
    case LastingEffect::HATE_ELVES:
    case LastingEffect::HATE_GREENSKINS: return Adjective{"Hates all "_s + getHatedGroupName(effect), true};
    case LastingEffect::SLOW_CRAFTING: return "Slow craftsman"_bad;
    case LastingEffect::SLOW_TRAINING: return "Slow trainee"_bad;
    case LastingEffect::BAD_BREATH: return "Smelly breath"_bad;
    case LastingEffect::ON_FIRE: return "On fire"_bad;
    case LastingEffect::DISAPPEAR_DURING_DAY: return "Disappears at dawn"_bad;
  }
}

optional<string> LastingEffects::getGoodAdjective(LastingEffect effect) {
  if (auto adjective = getAdjective(effect))
    if (!adjective->bad)
      return adjective->name;
  return none;
}

optional<std::string> LastingEffects::getBadAdjective(LastingEffect effect) {
  if (auto adjective = getAdjective(effect))
    if (adjective->bad)
      return adjective->name;
  return none;
}

const vector<LastingEffect>& LastingEffects::getCausingCondition(CreatureCondition condition) {
  switch (condition) {
    case CreatureCondition::RESTRICTED_MOVEMENT: {
      static vector<LastingEffect> ret {LastingEffect::ENTANGLED, LastingEffect::TIED_UP,
          LastingEffect::SLEEP};
      return ret;
    }
    case CreatureCondition::SLEEPING: {
      static vector<LastingEffect> ret { LastingEffect::SLEEP, LastingEffect::STUNNED};
      return ret;
    }
  }
}

double LastingEffects::modifyCreatureDefense(LastingEffect e, double defense, AttrType damageAttr) {
  auto multiplyFor = [&](AttrType attr, double m) {
    if (damageAttr == attr)
      return defense * m;
    else
      return defense;
  };
  double baseMultiplier = 1.3;
  switch (e) {
    case LastingEffect::MAGIC_RESISTANCE:
      return multiplyFor(AttrType::SPELL_DAMAGE, baseMultiplier);
    case LastingEffect::MELEE_RESISTANCE:
      return multiplyFor(AttrType::DAMAGE, baseMultiplier);
    case LastingEffect::RANGED_RESISTANCE:
      return multiplyFor(AttrType::RANGED_DAMAGE, baseMultiplier);
    case LastingEffect::MAGIC_VULNERABILITY:
      return multiplyFor(AttrType::SPELL_DAMAGE, 1.0 / baseMultiplier);
    case LastingEffect::MELEE_VULNERABILITY:
      return multiplyFor(AttrType::DAMAGE, 1.0 / baseMultiplier);
    case LastingEffect::RANGED_VULNERABILITY:
      return multiplyFor(AttrType::RANGED_DAMAGE, 1.0 / baseMultiplier);
    default:
      return defense;
  }
}

void LastingEffects::afterCreatureDamage(Creature* c, LastingEffect e) {
  switch (e) {
    case LastingEffect::SLEEP:
      c->removeEffect(e);
      break;
    default: break;
  }
}

bool LastingEffects::tick(Creature* c, LastingEffect effect) {
  switch (effect) {
    case LastingEffect::BLEEDING:
      c->getBody().bleed(c, 0.03);
      c->secondPerson(PlayerMessage("You are bleeding.", MessagePriority::HIGH));
      c->thirdPerson(PlayerMessage(c->getName().the() + " is bleeding.", MessagePriority::HIGH));
      if (c->getBody().getHealth() <= 0) {
        c->you(MsgType::DIE_OF, "bleeding");
        c->dieWithLastAttacker();
        return true;
      }
      break;
    case LastingEffect::REGENERATION:
      c->getBody().heal(c, 0.03);
      break;
    case LastingEffect::ON_FIRE:
      c->getPosition().fireDamage(0.1);
      break;
    case LastingEffect::POISON:
      c->getBody().bleed(c, 0.03);
      c->secondPerson(PlayerMessage("You suffer from poisoning.", MessagePriority::HIGH));
      c->thirdPerson(PlayerMessage(c->getName().the() + " suffers from poisoning.", MessagePriority::HIGH));
      if (c->getBody().getHealth() <= 0) {
        c->you(MsgType::DIE_OF, "poisoning");
        c->dieWithLastAttacker();
        return true;
      }
      break;
    case LastingEffect::WARNING: {
      const int radius = 5;
      bool isDanger = false;
      bool isBigDanger = false;
      auto position = c->getPosition();
      for (Position v : position.getRectangle(Rectangle(-radius, -radius, radius + 1, radius + 1))) {
        for (auto f : v.getFurniture())
          if (f->emitsWarning(c)) {
            if (v.dist8(position).value_or(2) <= 1)
              isBigDanger = true;
            else
              isDanger = true;
          }
        if (Creature* enemy = v.getCreature()) {
          if (!c->canSee(enemy) && c->isEnemy(enemy)) {
            int diff = enemy->getAttr(AttrType::DAMAGE) - c->getAttr(AttrType::DEFENSE);
            if (diff > 5)
              isBigDanger = true;
            else
              if (diff > 0)
                isDanger = true;
          }
        }
      }
      if (isBigDanger)
        c->privateMessage(PlayerMessage("You sense big danger!", MessagePriority::HIGH));
      else
      if (isDanger)
        c->privateMessage(PlayerMessage("You sense danger!", MessagePriority::HIGH));
      break;
    }
    case LastingEffect::SUNLIGHT_VULNERABLE:
      if (c->getPosition().sunlightBurns()) {
        c->you(MsgType::ARE, "burnt by the sun");
        if (Random.roll(10)) {
          c->you(MsgType::YOUR, "body crumbles to dust");
          c->dieWithReason("killed by sunlight", Creature::DropType::ONLY_INVENTORY);
          return true;
        }
      }
      break;
    case LastingEffect::ENTERTAINER:
      if (!c->isAffected(LastingEffect::SLEEP) && Random.roll(50)) {
        auto others = c->getVisibleCreatures().filter([](const Creature* c) { return c->getBody().hasBrain() && c->getBody().isHumanoid(); });
        if (others.empty())
          break;
        string jokeText = "a joke";
        optional<LastingEffect> hatedGroup;
        for (auto effect : getHateEffects())
          if (c->isAffected(effect) || (c->getAttributes().getHatedByEffect() != effect && Random.roll(10 * getHateEffects().size()))) {
            hatedGroup = effect;
            break;
          }
        if (hatedGroup)
          jokeText.append(" about "_s + getHatedGroupName(*hatedGroup));
        c->verb("crack", "cracks", jokeText);
        for (auto other : others)
          if (other != c && !other->isAffected(LastingEffect::SLEEP)) {
            if (hatedGroup && hatedGroup == other->getAttributes().getHatedByEffect()) {
              other->addMorale(-0.05);
              other->you(MsgType::ARE, "offended");
            } else {
              other->verb("laugh", "laughs");
              other->addMorale(0.01);
            }
          }
      }
      break;
    case LastingEffect::BAD_BREATH:
      for (auto pos : c->getPosition().getRectangle(Rectangle::centered(7)))
        if (auto other = pos.getCreature())
          other->addMorale(-0.002);
      break;
    case LastingEffect::DISAPPEAR_DURING_DAY:
      if (c->getGame()->getSunlightInfo().getState() == SunlightState::DAY)
        c->dieNoReason(Creature::DropType::ONLY_INVENTORY);
      break;
    default:
      break;
  }
  return false;
}

string LastingEffects::getName(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "pregnant";
    case LastingEffect::BLEEDING: return "bleeding";
    case LastingEffect::SLOWED: return "slowness";
    case LastingEffect::SPEED: return "speed";
    case LastingEffect::BLIND: return "blindness";
    case LastingEffect::INVISIBLE: return "invisibility";
    case LastingEffect::POISON: return "poison";
    case LastingEffect::POISON_RESISTANT: return "poison resistance";
    case LastingEffect::FLYING: return "levitation";
    case LastingEffect::COLLAPSED: return "collapse";
    case LastingEffect::PANIC: return "panic";
    case LastingEffect::RAGE: return "rage";
    case LastingEffect::HALLU: return "magic";
    case LastingEffect::SLEEP_RESISTANT: return "sleep resistance";
    case LastingEffect::DAM_BONUS: return "damage";
    case LastingEffect::DEF_BONUS: return "defense";
    case LastingEffect::SLEEP: return "sleep";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "stunning";
    case LastingEffect::FIRE_RESISTANT: return "fire resistance";
    case LastingEffect::INSANITY: return "insanity";
    case LastingEffect::PEACEFULNESS: return "love";
    case LastingEffect::MAGIC_RESISTANCE: return "magic resistance";
    case LastingEffect::MELEE_RESISTANCE: return "melee resistance";
    case LastingEffect::RANGED_RESISTANCE: return "ranged resistance";
    case LastingEffect::MAGIC_VULNERABILITY: return "magic vulnerability";
    case LastingEffect::MELEE_VULNERABILITY: return "melee vulnerability";
    case LastingEffect::RANGED_VULNERABILITY: return "ranged vulnerability";
    case LastingEffect::LIGHT_SOURCE: return "light";
    case LastingEffect::DARKNESS_SOURCE: return "darkness";
    case LastingEffect::NIGHT_VISION: return "night vision";
    case LastingEffect::ELF_VISION: return "elf vision";
    case LastingEffect::REGENERATION: return "regeneration";
    case LastingEffect::WARNING: return "warning";
    case LastingEffect::TELEPATHY: return "telepathy";
    case LastingEffect::SUNLIGHT_VULNERABLE: return "sunlight vulnerability";
    case LastingEffect::SATIATED: return "satiety";
    case LastingEffect::RESTED: return "wakefulness";
    case LastingEffect::SUMMONED: return "time to live";
    case LastingEffect::HATE_UNDEAD:
    case LastingEffect::HATE_DWARVES:
    case LastingEffect::HATE_HUMANS:
    case LastingEffect::HATE_GREENSKINS:
    case LastingEffect::HATE_ELVES: return "hate of "_s + getHatedGroupName(type);
    case LastingEffect::FAST_CRAFTING: return "fast crafting";
    case LastingEffect::FAST_TRAINING: return "fast training";
    case LastingEffect::SLOW_CRAFTING: return "slow crafting";
    case LastingEffect::SLOW_TRAINING: return "slow training";
    case LastingEffect::ENTERTAINER: return "entertainment";
    case LastingEffect::BAD_BREATH: return "smelly breath";
    case LastingEffect::AMBUSH_SKILL: return "ambush";
    case LastingEffect::STEALING_SKILL: return "stealing";
    case LastingEffect::SWIMMING_SKILL: return "swimming";
    case LastingEffect::DISARM_TRAPS_SKILL: return "trap disarming";
    case LastingEffect::CONSUMPTION_SKILL: return "absorbtion";
    case LastingEffect::COPULATION_SKILL: return "copulatation";
    case LastingEffect::CROPS_SKILL: return "farming";
    case LastingEffect::SPIDER_SKILL: return "spider web weaving";
    case LastingEffect::EXPLORE_SKILL: return "exploring";
    case LastingEffect::EXPLORE_CAVES_SKILL: return "exploring caves";
    case LastingEffect::EXPLORE_NOCTURNAL_SKILL: return "exploring at night";
    case LastingEffect::BRIDGE_BUILDING_SKILL: return "bridge building";
    case LastingEffect::NAVIGATION_DIGGING_SKILL: return "digging";
    case LastingEffect::ON_FIRE: return "combustion";
    case LastingEffect::DISAPPEAR_DURING_DAY: return "night life";
    case LastingEffect::NO_CARRY_LIMIT: return "infinite carrying capacity";
  }
}

string LastingEffects::getDescription(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "This is no dream! This is really happening!";
    case LastingEffect::SLOWED: return "Causes unnaturally slow movement.";
    case LastingEffect::BLEEDING: return "Causes loss of health points over time.";
    case LastingEffect::SPEED: return "Causes unnaturally quick movement.";
    case LastingEffect::BLIND: return "Causes blindness";
    case LastingEffect::INVISIBLE: return "Makes you invisible to enemies.";
    case LastingEffect::POISON: return "Decreases health every turn by a little bit.";
    case LastingEffect::POISON_RESISTANT: return "Gives poison resistance.";
    case LastingEffect::FLYING: return "Causes levitation.";
    case LastingEffect::COLLAPSED: return "Moving across tiles takes three times longer.";
    case LastingEffect::PANIC: return "Increases defense and lowers damage.";
    case LastingEffect::RAGE: return "Increases damage and lowers defense.";
    case LastingEffect::HALLU: return "Causes hallucinations.";
    case LastingEffect::DAM_BONUS: return "Gives a damage bonus.";
    case LastingEffect::DEF_BONUS: return "Gives a defense bonus.";
    case LastingEffect::SLEEP_RESISTANT: return "Prevents being put to sleep.";
    case LastingEffect::SLEEP: return "Puts to sleep.";
    case LastingEffect::TIED_UP:
      FALLTHROUGH;
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "Allows enslaving as a prisoner, otherwise creature will die.";
    case LastingEffect::FIRE_RESISTANT: return "Gives fire resistance.";
    case LastingEffect::INSANITY: return "Makes the target hostile to every creature.";
    case LastingEffect::PEACEFULNESS: return "Makes the target friendly to every creature.";
    case LastingEffect::MAGIC_RESISTANCE: return "Increases defense against magical attacks by 30%.";
    case LastingEffect::MELEE_RESISTANCE: return "Increases defense against melee attacks by 30%.";
    case LastingEffect::RANGED_RESISTANCE: return "Increases defense against ranged attacks by 30%.";
    case LastingEffect::MAGIC_VULNERABILITY: return "Decreases defense against magical attacks by 23%.";
    case LastingEffect::MELEE_VULNERABILITY: return "Decreases defense against melee attacks by 23%.";
    case LastingEffect::RANGED_VULNERABILITY: return "Decreases defense against ranged attacks by 23%.";
    case LastingEffect::DARKNESS_SOURCE: return "Causes the closest vicinity to become dark. Protects undead from sunlight.";
    case LastingEffect::LIGHT_SOURCE: return "Casts light on the closest surroundings.";
    case LastingEffect::NIGHT_VISION: return "Gives vision in the dark at full distance.";
    case LastingEffect::ELF_VISION: return "Allows to see and shoot through trees.";
    case LastingEffect::REGENERATION: return "Recovers a little bit of health every turn.";
    case LastingEffect::WARNING: return "Warns about dangerous enemies and traps.";
    case LastingEffect::TELEPATHY: return "Allows you to detect other creatures with brains.";
    case LastingEffect::SUNLIGHT_VULNERABLE: return "Sunlight makes your body crumble to dust.";
    case LastingEffect::SATIATED: return "Increases morale and improves defense by +1.";
    case LastingEffect::RESTED: return "Increases morale and improves defense by +1.";
    case LastingEffect::SUMMONED: return "Will disappear after the given number of turns.";
    case LastingEffect::HATE_UNDEAD:
    case LastingEffect::HATE_DWARVES:
    case LastingEffect::HATE_HUMANS:
    case LastingEffect::HATE_GREENSKINS:
    case LastingEffect::HATE_ELVES: return "Makes the target hostile to all "_s + getHatedGroupName(type);
    case LastingEffect::FAST_CRAFTING: return "Increases crafting speed";
    case LastingEffect::FAST_TRAINING: return "Increases training and learning speed";
    case LastingEffect::SLOW_CRAFTING: return "Decreases crafting speed";
    case LastingEffect::SLOW_TRAINING: return "Decreases training and learning speed";
    case LastingEffect::ENTERTAINER: return "Makes jokes, increasing morale of nearby creatures";
    case LastingEffect::BAD_BREATH: return "Decreases morale of all nearby creatures";
    case LastingEffect::AMBUSH_SKILL: return "Can hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it.";
    case LastingEffect::STEALING_SKILL: return "Can steal from other monsters";
    case LastingEffect::SWIMMING_SKILL: return "Can cross water without drowning.";
    case LastingEffect::DISARM_TRAPS_SKILL: return "Can evade traps and disarm them.";
    case LastingEffect::CONSUMPTION_SKILL: return "Can absorb other creatures and retain their attributes.";
    case LastingEffect::COPULATION_SKILL: return "Can copulate with other creatures and give birth to hideous spawns.";
    case LastingEffect::CROPS_SKILL: return "Can farm crops";
    case LastingEffect::SPIDER_SKILL: return "Can weave spider webs.";
    case LastingEffect::EXPLORE_SKILL: return "Can explore surroundings";
    case LastingEffect::EXPLORE_CAVES_SKILL: return "Can explore caves";
    case LastingEffect::EXPLORE_NOCTURNAL_SKILL: return "Can explore surroundings at night";
    case LastingEffect::BRIDGE_BUILDING_SKILL: return "Creature will try to build bridges when travelling somewhere";
    case LastingEffect::NAVIGATION_DIGGING_SKILL: return "Creature will try to dig when travelling somewhere";
    case LastingEffect::ON_FIRE: return "The creature is burning alive";
    case LastingEffect::DISAPPEAR_DURING_DAY: return "This creature is only active at night and disappears at dawn";
    case LastingEffect::NO_CARRY_LIMIT: return "The creature can carry items without any weight limit";
  }
}

bool LastingEffects::canSee(const Creature* c1, const Creature* c2) {
  PROFILE;
  return c1->getPosition().dist8(c2->getPosition()).value_or(5) < 5 && c2->getBody().hasBrain() &&
      c1->isAffected(LastingEffect::TELEPATHY);
}

bool LastingEffects::modifyIsEnemyResult(const Creature* c, const Creature* other, bool result) {
  PROFILE;
  if (c->isAffected(LastingEffect::PEACEFULNESS))
    return false;
  if (c->isAffected(LastingEffect::INSANITY))
    return true;
  if (auto effect = other->getAttributes().getHatedByEffect())
    if (c->isAffected(*effect))
      return true;
  return result;
}

int LastingEffects::getPrice(LastingEffect e) {
  switch (e) {
    case LastingEffect::INSANITY:
    case LastingEffect::HATE_UNDEAD:
    case LastingEffect::HATE_DWARVES:
    case LastingEffect::HATE_HUMANS:
    case LastingEffect::HATE_GREENSKINS:
    case LastingEffect::HATE_ELVES:
    case LastingEffect::PEACEFULNESS:
    case LastingEffect::HALLU:
    case LastingEffect::BLEEDING:
    case LastingEffect::SUNLIGHT_VULNERABLE:
    case LastingEffect::SATIATED:
    case LastingEffect::RESTED:
    case LastingEffect::SUMMONED:
    case LastingEffect::SLOW_CRAFTING:
    case LastingEffect::SLOW_TRAINING:
      return 2;
    case LastingEffect::WARNING:
      return 5;
    case LastingEffect::SPEED:
    case LastingEffect::PANIC:
    case LastingEffect::SLEEP:
    case LastingEffect::ENTANGLED:
    case LastingEffect::TIED_UP:
    case LastingEffect::STUNNED:
    case LastingEffect::RAGE:
    case LastingEffect::COLLAPSED:
    case LastingEffect::NIGHT_VISION:
    case LastingEffect::ELF_VISION:
    case LastingEffect::REGENERATION:
    case LastingEffect::FAST_CRAFTING:
    case LastingEffect::FAST_TRAINING:
      return 12;
    case LastingEffect::BLIND:
      return 16;
    case LastingEffect::DAM_BONUS:
    case LastingEffect::DEF_BONUS:
      return 20;
    case LastingEffect::SLOWED:
    case LastingEffect::POISON_RESISTANT:
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::POISON:
    case LastingEffect::TELEPATHY:
      return 20;
    case LastingEffect::INVISIBLE:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::PREGNANT:
    case LastingEffect::FLYING:
    default:
      return 24;
  }
}

double LastingEffects::getMoraleIncrease(const Creature* c) {
  PROFILE;
  double ret = 0;
  if (c->isAffected(LastingEffect::RESTED))
    ret += 0.1;
  else
    ret -= 0.1;
  if (c->isAffected(LastingEffect::SATIATED))
    ret += 0.1;
  else
    ret -= 0.1;
  return ret;
}

double LastingEffects::getCraftingSpeed(const Creature* c) {
  double ret = 1;
  if (c->isAffected(LastingEffect::FAST_CRAFTING))
    ret *= 1.3;
  if (c->isAffected(LastingEffect::SLOW_CRAFTING))
    ret /= 1.3;
  return ret;
}

double LastingEffects::getTrainingSpeed(const Creature* c) {
  double ret = 1;
  if (c->isAffected(LastingEffect::FAST_TRAINING))
    ret *= 1.3;
  if (c->isAffected(LastingEffect::SLOW_TRAINING))
    ret /= 1.3;
  return ret;
}

bool LastingEffects::canConsume(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::COLLAPSED:
    case LastingEffect::STUNNED:
    case LastingEffect::SLEEP:
    case LastingEffect::ENTANGLED:
    case LastingEffect::TIED_UP:
    case LastingEffect::BLEEDING:
    case LastingEffect::SATIATED:
    case LastingEffect::RESTED:
    case LastingEffect::SUMMONED:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::PREGNANT:
    case LastingEffect::CONSUMPTION_SKILL:
    case LastingEffect::COPULATION_SKILL:
    case LastingEffect::CROPS_SKILL:
    case LastingEffect::ON_FIRE:
      return false;
    default:
      return true;
  }
}

optional<FXVariantName> LastingEffects::getFX(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::SLEEP:
      return FXVariantName::SLEEP;

    case LastingEffect::SPEED:
      return FXVariantName::BUFF_BLUE;
    case LastingEffect::SLOWED:
      return FXVariantName::DEBUFF_BLUE;

    case LastingEffect::REGENERATION:
      return FXVariantName::BUFF_RED;
    case LastingEffect::BLEEDING:
      return FXVariantName::DEBUFF_RED;

    case LastingEffect::DAM_BONUS:
    case LastingEffect::DEF_BONUS:
      return FXVariantName::BUFF_YELLOW;

    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
      return FXVariantName::BUFF_SKY_BLUE;
    case LastingEffect::FAST_CRAFTING:
    case LastingEffect::SLOW_CRAFTING:
      return FXVariantName::BUFF_BROWN;
    case LastingEffect::FAST_TRAINING:
      return FXVariantName::BUFF_BROWN;
    case LastingEffect::SLOW_TRAINING:
      return FXVariantName::DEBUFF_BROWN;
    case LastingEffect::POISON_RESISTANT:
      return FXVariantName::BUFF_GREEN2;
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
      return FXVariantName::DEBUFF_ORANGE;

    case LastingEffect::TELEPATHY:
    case LastingEffect::ELF_VISION:
    case LastingEffect::NIGHT_VISION:
      return FXVariantName::BUFF_PINK;
    case LastingEffect::BLIND:
    case LastingEffect::INSANITY:
      return FXVariantName::DEBUFF_PINK;

    case LastingEffect::POISON:
      return FXVariantName::DEBUFF_GREEN2;

    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED:
      return FXVariantName::DEBUFF_GREEN1;
    case LastingEffect::ON_FIRE:
      return FXVariantName::FIRE;
    default:
      return none;
  }
}

optional<FXInfo> LastingEffects::getApplicationFX(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::SPEED:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::LIGHT_BLUE);
    case LastingEffect::DAM_BONUS:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::YELLOW);
    case LastingEffect::DEF_BONUS:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::YELLOW);
    case LastingEffect::INVISIBLE:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::WHITE);
    case LastingEffect::POISON:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::GREEN);
    case LastingEffect::POISON_RESISTANT:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::YELLOW);
    case LastingEffect::INSANITY:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::PINK);
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::SKY_BLUE);
    case LastingEffect::REGENERATION:
      return FXInfo(FXName::CIRCULAR_SPELL, Color::RED);
    default:
      return none;
  }
}

bool LastingEffects::canProlong(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::ON_FIRE:
      return false;
    default:
      return true;
  }
}

bool LastingEffects::obeysFormation(const Creature* c, const Creature* against) {
  if (c->isAffected(LastingEffect::RAGE) || c->isAffected(LastingEffect::INSANITY))
    return false;
  else if (auto e = against->getAttributes().getHatedByEffect())
    if (c->isAffected(*e))
      return false;
   return true;
}
