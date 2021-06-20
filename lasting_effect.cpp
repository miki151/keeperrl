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
#include "vision.h"
#include "collective.h"
#include "territory.h"
#include "health_type.h"
#include "attack_trigger.h"
#include "gender.h"

static optional<LastingEffect> getCancelledOneWay(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::PLAGUE_RESISTANT:
      return LastingEffect::PLAGUE;
    case LastingEffect::POISON_RESISTANT:
      return LastingEffect::POISON;
    case LastingEffect::FIRE_RESISTANT:
      return LastingEffect::ON_FIRE;
    case LastingEffect::COLD_RESISTANT:
      return LastingEffect::FROZEN;
    case LastingEffect::SLEEP_RESISTANT:
      return LastingEffect::SLEEP;
    case LastingEffect::REGENERATION:
      return LastingEffect::BLEEDING;
    default:
      return none;
  }
}

static optional<LastingEffect> getMutuallyExclusiveImpl(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::PANIC:
      return LastingEffect::RAGE;
    case LastingEffect::PEACEFULNESS:
      return LastingEffect::INSANITY;
    case LastingEffect::FROZEN:
      return LastingEffect::ON_FIRE;
    default:
      return none;
  }
}

static optional<LastingEffect> getPreventedBy(LastingEffect effect) {
  static EnumMap<LastingEffect, optional<LastingEffect>> ret(
    [&](LastingEffect e) -> optional<LastingEffect> {
      if (auto other = getCancelledOneWay(e))
        return *other;
      if (auto other = getMutuallyExclusiveImpl(e))
        return *other;
      for (auto other : ENUM_ALL(LastingEffect))
        if (getMutuallyExclusiveImpl(other) == e)
          return other;
      return none;
    }
  );
  return ret[effect];
}

static optional<LastingEffect> getPreventing(LastingEffect effect) {
  static EnumMap<LastingEffect, optional<LastingEffect>> ret(
    [&](LastingEffect e) -> optional<LastingEffect> {
      for (auto other : ENUM_ALL(LastingEffect))
        if (getPreventedBy(other) == e)
          return other;
      return none;
    }
  );
  return ret[effect];
}

static optional<LastingEffect> getRequired(LastingEffect effect, const Creature *c) {
  switch (effect) {
    case LastingEffect::ON_FIRE:
      if (c->getBody().burnsIntrinsically())
        return none;
      return LastingEffect::OIL;
    default:
      return none;
  }
}

void LastingEffects::runTests() {
  CHECK(getPreventing(LastingEffect::POISON) == LastingEffect::POISON_RESISTANT);
  CHECK(!getPreventing(LastingEffect::POISON_RESISTANT));
  CHECK(!getPreventedBy(LastingEffect::POISON));
  CHECK(getPreventedBy(LastingEffect::POISON_RESISTANT) == LastingEffect::POISON);
}

void LastingEffects::onAffected(Creature* c, LastingEffect effect, bool msg) {
  PROFILE;
  if (auto e = getCancelledOneWay(effect))
    c->removeEffect(*e);
  switch (effect) {
    case LastingEffect::LIGHT_SOURCE:
      c->getPosition().addCreatureLight(false);
      break;
    case LastingEffect::DARKNESS_SOURCE:
      c->getPosition().addCreatureLight(true);
      break;
    case LastingEffect::ON_FIRE:
      c->getPosition().addCreatureLight(false);
      break;
    case LastingEffect::SWARMER:
      c->getPosition().addSwarmer();
      break;
    default:
      break;
  }
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
      case LastingEffect::ACID_RESISTANT:
        c->you(MsgType::ARE, "now acid resistant");
        break;
      case LastingEffect::COLD_RESISTANT:
        c->you(MsgType::ARE, "now cold resistant");
        break;
      case LastingEffect::INSANITY:
        c->you(MsgType::BECOME, "insane");
        break;
      case LastingEffect::PEACEFULNESS:
        c->you(MsgType::BECOME, "peaceful");
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
      case LastingEffect::ARCHER_VISION:
        c->you("can see through arrowslits");
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
      case LastingEffect::HATE_DRAGONS:
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
      case LastingEffect::MAGIC_CANCELLATION:
        c->you(MsgType::ARE, "unable to cast any spells!");
        break;
      case LastingEffect::SPELL_DAMAGE:
        c->verb("deal", "deals", "magical damage");
        break;
      case LastingEffect::ON_FIRE:
        c->you(MsgType::ARE, "on fire!");
        break;
      case LastingEffect::FROZEN:
        c->you(MsgType::ARE, "frozen!");
        break;
      case LastingEffect::PLAGUE:
        c->you(MsgType::ARE, "infected by plague!");
        break;
      case LastingEffect::PLAGUE_RESISTANT:
        c->you(MsgType::ARE, "resistant to plague");
        break;
      case LastingEffect::AMBUSH_SKILL:
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
      case LastingEffect::DISTILLATION_SKILL:
      case LastingEffect::NAVIGATION_DIGGING_SKILL:
        c->verb("acquire", "acquires", "the skill of "_s + getName(effect));
        break;
      case LastingEffect::DISAPPEAR_DURING_DAY:
        c->you(MsgType::YOUR, "hours are numbered");
        break;
      case LastingEffect::NO_CARRY_LIMIT:
        c->you(MsgType::YOUR, "carrying capacity increases");
        break;
      case LastingEffect::SPYING:
        c->secondPerson("You put on your sunglasses"_s);
        c->thirdPerson(c->getName().the() + " puts on " + his(c->getAttributes().getGender()) + " sunglasses"_s);
        break;
      case LastingEffect::IMMOBILE:
        c->you("can't move anymore");
        break;
      case LastingEffect::LIFE_SAVED:
        c->you(MsgType::YOUR, "life will be saved");
        break;
      case LastingEffect::UNSTABLE:
        c->you(MsgType::FEEL, "mentally unstable");
        break;
      case LastingEffect::OIL:
        c->you(MsgType::ARE, "covered in oil!");
        break;
      case LastingEffect::SWARMER:
        c->verb("feel", "feels", "like swarming someone");
        break;
      case LastingEffect::PSYCHIATRY:
        c->you(MsgType::BECOME, "more understanding");
        break;
      case LastingEffect::INVULNERABLE:
        c->you(MsgType::ARE, "invulnerable!");
        break;
      case LastingEffect::TURNED_OFF:
        c->you(MsgType::ARE, "turned off");
        break;
      case LastingEffect::DRUNK:
        c->you(MsgType::ARE, "drunk!");
        break;
      case LastingEffect::NO_FRIENDLY_FIRE:
        c->you(MsgType::YOUR, "projectiles won't hit allies");
        break;
      default:
        break;
    }
}

bool LastingEffects::affects(const Creature* c, LastingEffect effect) {
  if (c->getBody().isImmuneTo(effect))
    return false;
  if (auto preventing = getPreventing(effect))
    if (c->isAffected(*preventing))
      return false;
  if (auto required = getRequired(effect, c))
    if (!c->isAffected(*required))
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
    case LastingEffect::PLAGUE:
      if (msg)
        c->you(MsgType::ARE, "cured from plague!");
      break;
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, "cured from poisoning");
      break;
    case LastingEffect::SLEEP:
      if (msg)
        c->you(MsgType::WAKE_UP, "");
      break;
    case LastingEffect::STUNNED:
      if (msg)
        c->you(MsgType::ARE, "no longer unconscious");
      break;
    case LastingEffect::ON_FIRE:
      if (msg)
        c->you(MsgType::YOUR, "flames are extinguished");
      c->getPosition().removeCreatureLight(false);
      break;
    default:
      onTimedOut(c, effect, msg);
      break;
  }
}

void LastingEffects::onTimedOut(Creature* c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::SLEEP:
      c->addEffect(LastingEffect::RESTED, 1000_visible);
      break;
    case LastingEffect::PLAGUE:
      c->addPermanentEffect(LastingEffect::PLAGUE_RESISTANT);
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
    case LastingEffect::ON_FIRE:
      c->getPosition().removeCreatureLight(false);
      if (c->getBody().burnsIntrinsically())
        c->dieNoReason(Creature::DropType::ONLY_INVENTORY);
      break;
    case LastingEffect::SPYING:
      c->setAlternativeViewId(none);
      break;
    case LastingEffect::OIL:
      c->removeEffect(LastingEffect::ON_FIRE);
      break;
    case LastingEffect::SWARMER:
      c->getPosition().removeSwarmer();
      break;
    case LastingEffect::POLYMORPHED:
      c->popAttributes();
      break;
    default:
      break;
  }
  if (msg)
    switch (effect) {
      case LastingEffect::SLOWED:
        c->you(MsgType::ARE, "moving faster again");
        break;
      case LastingEffect::SLEEP:
        c->you(MsgType::WAKE_UP, "");
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
      case LastingEffect::PLAGUE:
        c->you(MsgType::YOUR, "plague infection subsides");
        break;
      case LastingEffect::PLAGUE_RESISTANT:
        c->you(MsgType::ARE, "no longer plague resistant");
        break;
      case LastingEffect::POISON_RESISTANT:
        c->you(MsgType::ARE, "no longer poison resistant");
        break;
      case LastingEffect::FIRE_RESISTANT:
        c->you(MsgType::ARE, "no longer fire resistant");
        break;
      case LastingEffect::ACID_RESISTANT:
        c->you(MsgType::ARE, "no longer acid resistant");
        break;
      case LastingEffect::COLD_RESISTANT:
        c->you(MsgType::ARE, "no longer cold resistant");
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
      case LastingEffect::ARCHER_VISION:
        c->you("can't see through arrowslits anymore");
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
      case LastingEffect::HATE_HUMANS:
      case LastingEffect::HATE_GREENSKINS:
      case LastingEffect::HATE_ELVES:
      case LastingEffect::HATE_UNDEAD:
      case LastingEffect::HATE_DWARVES:
      case LastingEffect::HATE_DRAGONS:
        c->you(MsgType::YOUR, "hatred is gone");
        break;
      case LastingEffect::ENTERTAINER:
        c->you(MsgType::ARE, "no longer funny");
        break;
      case LastingEffect::BAD_BREATH:
        c->verb("smell", "smells", "like flowers again");
        break;
      case LastingEffect::MAGIC_CANCELLATION:
        c->verb("can", "can", "cast spells again");
        break;
      case LastingEffect::SPELL_DAMAGE:
        c->verb("no longer deal", "no longer deals", "magical damage");
        break;
      case LastingEffect::ON_FIRE:
        if (c->getBody().burnsIntrinsically())
          c->verb("burn", "burns", "to death");
        else
          c->verb("stop", "stops", "burning");
        break;
      case LastingEffect::FROZEN:
        c->verb("thaw", "thaws");
        break;
      case LastingEffect::AMBUSH_SKILL:
      case LastingEffect::SWIMMING_SKILL:
      case LastingEffect::DISARM_TRAPS_SKILL:
      case LastingEffect::CONSUMPTION_SKILL:
      case LastingEffect::COPULATION_SKILL:
      case LastingEffect::CROPS_SKILL:
      case LastingEffect::SPIDER_SKILL:
      case LastingEffect::EXPLORE_SKILL:
      case LastingEffect::DISTILLATION_SKILL:
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
      case LastingEffect::SPYING:
        c->secondPerson("You remove your sunglasses"_s);
        c->thirdPerson(c->getName().the() + " removes " + his(c->getAttributes().getGender()) + " sunglasses"_s);
        break;
      case LastingEffect::LIFE_SAVED:
        c->you(MsgType::YOUR, "life will no longer be saved");
        break;
      case LastingEffect::UNSTABLE:
        c->you(MsgType::FEEL, "mentally stable again");
        break;
      case LastingEffect::OIL:
        c->you(MsgType::ARE, "no longer covered in oil");
        break;
      case LastingEffect::SWARMER:
        c->verb("no longer feel", "no longer feels", "like swarming someone");
        break;
      case LastingEffect::PSYCHIATRY:
        c->you(MsgType::BECOME, "less understanding");
        break;
      case LastingEffect::INVULNERABLE:
        c->you(MsgType::ARE, "no longer invulnerable");
        break;
      case LastingEffect::TURNED_OFF:
        c->you(MsgType::ARE, "turned on");
        break;
      case LastingEffect::DRUNK:
        c->verb("sober", "sobers", "up");
        break;
      case LastingEffect::NO_FRIENDLY_FIRE:
        c->you(MsgType::YOUR, "projectiles will hit allies");
        break;
      case LastingEffect::POLYMORPHED:
        c->verb("return to your", "returns to "_s + his(c->getAttributes().getGender()), "previous form");
        c->addFX({FXName::SPAWN});
        break;
      default:
        break;
    }
}

static const int attrBonus = 3;

int LastingEffects::getAttrBonus(const Creature* c, AttrType type) {
  int value = 0;
  auto time = c->getGlobalTime();
  switch (type) {
    case AttrType::DAMAGE:
      if (c->isAffected(LastingEffect::DRUNK, time))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::PANIC, time))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::RAGE, time))
        value += attrBonus;
      if (c->isAffected(LastingEffect::DAM_BONUS, time))
        value += attrBonus;
      if (c->isAffected(LastingEffect::SWARMER, time))
        value += c->getPosition().countSwarmers() - 1;
      if (c->hasAlternativeViewId() && c->isAffected(LastingEffect::SPYING, time))
        value -= 99;
      break;
    case AttrType::RANGED_DAMAGE:
    case AttrType::SPELL_DAMAGE:
      if (c->isAffected(LastingEffect::DRUNK, time))
        value -= attrBonus;
      if (c->hasAlternativeViewId() && c->isAffected(LastingEffect::SPYING, time))
        value -= 99;
      break;
    case AttrType::DEFENSE:
      if (c->isAffected(LastingEffect::DRUNK, time))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::PANIC, time))
        value += attrBonus;
      if (c->isAffected(LastingEffect::RAGE, time))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::SLEEP, time))
        value -= attrBonus;
      if (c->isAffected(LastingEffect::DEF_BONUS, time))
        value += attrBonus;
      if (c->isAffected(LastingEffect::SATIATED, time))
        value += 1;
      if (c->isAffected(LastingEffect::RESTED, time))
        value += 1;
      if (c->isAffected(LastingEffect::SWARMER, time))
        value += c->getPosition().countSwarmers() - 1;
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

const char* LastingEffects::getHatedGroupName(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::HATE_UNDEAD: return "undead";
    case LastingEffect::HATE_DWARVES: return "dwarves";
    case LastingEffect::HATE_HUMANS: return "humans";
    case LastingEffect::HATE_ELVES: return "elves";
    case LastingEffect::HATE_GREENSKINS: return "greenskins";
    case LastingEffect::HATE_DRAGONS: return "dragons";
    default:
      return nullptr;
  }
}

static const vector<LastingEffect>& getHateEffects() {
  static vector<LastingEffect> ret = [] {
    vector<LastingEffect> ret;
    for (auto effect : ENUM_ALL(LastingEffect))
      if (LastingEffects::getHatedGroupName(effect))
        ret.push_back(effect);
    return ret;
  }();
  return ret;
}

static Adjective getAdjective(LastingEffect effect) {
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
    case LastingEffect::PLAGUE_RESISTANT: return "Plague resistant"_good;
    case LastingEffect::FIRE_RESISTANT: return "Fire resistant"_good;
    case LastingEffect::ACID_RESISTANT: return "Acid resistant"_good;
    case LastingEffect::COLD_RESISTANT: return "Cold resistant"_good;
    case LastingEffect::FLYING: return "Flying"_good;
    case LastingEffect::LIGHT_SOURCE: return "Source of light"_good;
    case LastingEffect::DARKNESS_SOURCE: return "Source of darkness"_good;
    case LastingEffect::PREGNANT: return "Pregnant"_good;
    case LastingEffect::MAGIC_RESISTANCE: return "Resistant to magical attacks"_good;
    case LastingEffect::MELEE_RESISTANCE: return "Resistant to melee attacks"_good;
    case LastingEffect::RANGED_RESISTANCE: return "Resistant to ranged attacks"_good;
    case LastingEffect::SPELL_DAMAGE: return "Deals magical damage"_good;
    case LastingEffect::ELF_VISION: return "Can see through trees"_good;
    case LastingEffect::ARCHER_VISION: return "Can see through arrowslits"_good;
    case LastingEffect::NIGHT_VISION: return "Can see in the dark"_good;
    case LastingEffect::REGENERATION: return "Regenerating"_good;
    case LastingEffect::WARNING: return "Aware of danger"_good;
    case LastingEffect::TELEPATHY: return "Telepathic"_good;
    case LastingEffect::SATIATED: return "Satiated"_good;
    case LastingEffect::RESTED: return "Rested"_good;
    case LastingEffect::FAST_CRAFTING: return "Fast craftsman"_good;
    case LastingEffect::FAST_TRAINING: return "Fast trainee"_good;
    case LastingEffect::ENTERTAINER: return "Entertainer"_good;
    case LastingEffect::AMBUSH_SKILL: return "Ambusher"_good;
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
    case LastingEffect::DISTILLATION_SKILL: return "Distiller"_good;
    case LastingEffect::NO_CARRY_LIMIT: return "Infinite carrying capacity"_good;
    case LastingEffect::SPYING: return "Spy"_good;
    case LastingEffect::LIFE_SAVED: return "Life will be saved"_good;
    case LastingEffect::SWARMER: return "Swarmer"_good;
    case LastingEffect::PSYCHIATRY: return "Psychiatrist"_good;
    case LastingEffect::INVULNERABLE: return "Invulnerable"_good;
    case LastingEffect::DRUNK: return "Drunk"_good;
    case LastingEffect::NO_FRIENDLY_FIRE: return "Arrows bypass allies"_good;
    case LastingEffect::POLYMORPHED: return "Polymorphed"_good;

    case LastingEffect::PEACEFULNESS: return "Peaceful"_bad;
    case LastingEffect::POISON: return "Poisoned"_bad;
    case LastingEffect::PLAGUE: return "Infected with plague"_bad;
    case LastingEffect::BLEEDING: return "Bleeding"_bad;
    case LastingEffect::SLEEP: return "Sleeping"_bad;
    case LastingEffect::ENTANGLED: return "Entangled"_bad;
    case LastingEffect::TIED_UP: return "Tied up"_bad;
    case LastingEffect::IMMOBILE: return "Immobile"_bad;
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
    case LastingEffect::HATE_GREENSKINS:
    case LastingEffect::HATE_DRAGONS:
      return Adjective{"Hates all "_s + LastingEffects::getHatedGroupName(effect), true};
    case LastingEffect::SLOW_CRAFTING: return "Slow craftsman"_bad;
    case LastingEffect::SLOW_TRAINING: return "Slow trainee"_bad;
    case LastingEffect::BAD_BREATH: return "Smelly breath"_bad;
    case LastingEffect::ON_FIRE: return "On fire"_bad;
    case LastingEffect::FROZEN: return "Frozen"_bad;
    case LastingEffect::MAGIC_CANCELLATION: return "Cancelled"_bad;
    case LastingEffect::DISAPPEAR_DURING_DAY: return "Disappears at dawn"_bad;
    case LastingEffect::UNSTABLE: return "Mentally unstable"_bad;
    case LastingEffect::OIL: return "Covered in oil"_bad;
    case LastingEffect::TURNED_OFF: return "Turned off"_bad;
  }
}

optional<string> LastingEffects::getGoodAdjective(LastingEffect effect) {
  auto adjective = getAdjective(effect);
  if (!adjective.bad)
    return adjective.name;
  return none;
}

optional<std::string> LastingEffects::getBadAdjective(LastingEffect effect) {
  auto adjective = getAdjective(effect);
  if (adjective.bad)
    return adjective.name;
  return none;
}

bool LastingEffects::losesControl(const Creature* c) {
  return c->isAffected(LastingEffect::SLEEP)
      || c->isAffected(LastingEffect::TURNED_OFF)
      || c->isAffected(LastingEffect::STUNNED);
}

bool LastingEffects::doesntMove(const Creature* c) {
  return losesControl(c)
      || c->isAffected(LastingEffect::FROZEN);
}

bool LastingEffects::restrictedMovement(const Creature* c) {
  return doesntMove(c)
      || c->isAffected(LastingEffect::ENTANGLED)
      || c->isAffected(LastingEffect::TIED_UP)
      || c->isAffected(LastingEffect::IMMOBILE);
}

bool LastingEffects::canSwapPosition(const Creature* c) {
  return !c->isAffected(LastingEffect::SLEEP)
      && !c->isAffected(LastingEffect::FROZEN)
      && !c->isAffected(LastingEffect::ENTANGLED)
      && !c->isAffected(LastingEffect::TIED_UP);
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
    case LastingEffect::INVULNERABLE:
      return 1000000;
    default:
      return defense;
  }
}

void LastingEffects::onAllyKilled(Creature* c) {
  if (Random.chance(0.05) && c->isAffected(LastingEffect::UNSTABLE)) {
    c->verb("have", "has", "gone berskerk!");
    c->addEffect(LastingEffect::INSANITY, 100_visible);
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
  PROFILE_BLOCK("LastingEffects::tick");
  switch (effect) {
    case LastingEffect::SPYING: {
      Collective* enemy = nullptr;
      optional<ViewId> enemyId;
      for (Collective* col : c->getPosition().getModel()->getCollectives())
        if ((col->getTerritory().contains(c->getPosition()) ||
               col->getTerritory().getStandardExtended().contains(c->getPosition())) &&
            col->getTribe()->isEnemy(c) && !col->getCreatures().empty()) {
          enemyId = Random.choose(col->getCreatures())->getViewObject().id();
          enemy = col;
        }
      auto isTriggered = [&] {
        if (auto playerCollective = c->getGame()->getPlayerCollective())
          if (playerCollective->getCreatures().contains(c))
            for (auto& t : enemy->getTriggers(playerCollective))
              if (t.trigger.contains<StolenItems>())
                return true;
        return false;
      };
      if (enemy && isTriggered()) {
        c->you(MsgType::YOUR, "identity is uncovered!");
        if (c->hasAlternativeViewId())
          c->setAlternativeViewId(none);
      } else {
        if (enemyId && !c->hasAlternativeViewId())
          c->setAlternativeViewId(*enemyId);
        if (!enemyId && c->hasAlternativeViewId())
          c->setAlternativeViewId(none);
      }
      break;
    }
    case LastingEffect::BLEEDING:
      if (!c->isAffected(LastingEffect::FROZEN)) {
        c->getBody().bleed(c, 0.03);
        c->secondPerson(PlayerMessage("You are bleeding.", MessagePriority::HIGH));
        c->thirdPerson(PlayerMessage(c->getName().the() + " is bleeding.", MessagePriority::HIGH));
        if (c->getBody().getHealth() <= 0) {
          c->you(MsgType::DIE_OF, "bleeding");
          c->dieWithLastAttacker();
          return true;
        }
      }
      break;
    case LastingEffect::REGENERATION:
      if (!c->isAffected(LastingEffect::FROZEN))
        c->getBody().heal(c, 0.03);
      break;
    case LastingEffect::ON_FIRE:
      c->getPosition().fireDamage(0.1);
      break;
    case LastingEffect::PLAGUE:
      if (!c->isAffected(LastingEffect::FROZEN)) {
        int sample = (int) std::abs(c->getUniqueId().getGenericId() % 10000);
        bool suffers = sample >= 1000;
        bool canDie = sample >= 9000 && !c->getStatus().contains(CreatureStatus::LEADER);
        for (auto pos : c->getPosition().neighbors8())
          if (auto other = pos.getCreature())
            if (Random.roll(10)) {
              other->addEffect(LastingEffect::PLAGUE, getDuration(c, LastingEffect::PLAGUE));
            }
        if (suffers) {
          if (c->getBody().getHealth() > 0.5 || canDie) {
            if (Random.roll(10)) {
              c->getBody().bleed(c, 0.03);
              c->secondPerson(PlayerMessage("You suffer from plague.", MessagePriority::HIGH));
              c->thirdPerson(PlayerMessage(c->getName().the() + " suffers from plague.", MessagePriority::HIGH));
              if (c->getBody().getHealth() <= 0) {
                c->you(MsgType::DIE_OF, "plague");
                c->dieWithReason("plague");
                return true;
              }
            }
          }
        }
      }
      break;
    case LastingEffect::POISON:
      if (!c->isAffected(LastingEffect::FROZEN)) {
        c->getBody().bleed(c, 0.03);
        c->secondPerson(PlayerMessage("You suffer from poisoning.", MessagePriority::HIGH));
        c->thirdPerson(PlayerMessage(c->getName().the() + " suffers from poisoning.", MessagePriority::HIGH));
        if (c->getBody().getHealth() <= 0) {
          c->you(MsgType::DIE_OF, "poisoning");
          c->dieWithLastAttacker();
          return true;
        }
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
      else if (isDanger)
        c->privateMessage(PlayerMessage("You sense danger!", MessagePriority::HIGH));
      break;
    }
    case LastingEffect::SUNLIGHT_VULNERABLE:
      if (c->getPosition().sunlightBurns() && !c->isAffected(LastingEffect::FROZEN)) {
        c->you(MsgType::ARE, "burnt by the sun");
        if (Random.roll(10)) {
          c->you(MsgType::YOUR, "body crumbles to dust");
          c->dieWithReason("killed by sunlight", Creature::DropType::ONLY_INVENTORY);
          return true;
        }
      }
      break;
    case LastingEffect::ENTERTAINER:
      if (!doesntMove(c) && Random.roll(50)) {
        auto others = c->getVisibleCreatures().filter([](const Creature* c) {
          return c->getBody().isHumanoid();
        });
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
          jokeText.append(" about "_s + LastingEffects::getHatedGroupName(*hatedGroup));
        c->verb("crack", "cracks", jokeText);
        for (auto other : others)
          if (other != c && !other->isAffected(LastingEffect::SLEEP)) {
            if (hatedGroup && hatedGroup == other->getAttributes().getHatedByEffect()) {
              other->addMorale(-0.05);
              other->you(MsgType::ARE, "offended");
            } else if (other->getBody().hasBrain()) {
              other->verb("laugh", "laughs");
              other->addMorale(0.01);
            } else
              other->verb("don't", "doesn't", "laugh");
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
    case LastingEffect::PLAGUE: return "plague";
    case LastingEffect::POISON_RESISTANT: return "poison resistance";
    case LastingEffect::PLAGUE_RESISTANT: return "plague resistance";
    case LastingEffect::FLYING: return "levitation";
    case LastingEffect::COLLAPSED: return "collapse";
    case LastingEffect::PANIC: return "panic";
    case LastingEffect::RAGE: return "rage";
    case LastingEffect::HALLU: return "magic";
    case LastingEffect::SLEEP_RESISTANT: return "sleep resistance";
    case LastingEffect::DAM_BONUS: return "damage";
    case LastingEffect::DEF_BONUS: return "defense";
    case LastingEffect::SLEEP: return "sleep";
    case LastingEffect::IMMOBILE: return "immobility";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "stunning";
    case LastingEffect::FIRE_RESISTANT: return "fire resistance";
    case LastingEffect::COLD_RESISTANT: return "cold resistance";
    case LastingEffect::ACID_RESISTANT: return "acid resistance";
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
    case LastingEffect::ARCHER_VISION: return "arrowslit vision";
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
    case LastingEffect::HATE_DRAGONS:
    case LastingEffect::HATE_ELVES: return "hate of "_s + LastingEffects::getHatedGroupName(type);
    case LastingEffect::FAST_CRAFTING: return "fast crafting";
    case LastingEffect::FAST_TRAINING: return "fast training";
    case LastingEffect::SLOW_CRAFTING: return "slow crafting";
    case LastingEffect::SLOW_TRAINING: return "slow training";
    case LastingEffect::ENTERTAINER: return "entertainment";
    case LastingEffect::BAD_BREATH: return "smelly breath";
    case LastingEffect::AMBUSH_SKILL: return "ambushing";
    case LastingEffect::SWIMMING_SKILL: return "swimming";
    case LastingEffect::DISARM_TRAPS_SKILL: return "trap disarming";
    case LastingEffect::CONSUMPTION_SKILL: return "absorption";
    case LastingEffect::COPULATION_SKILL: return "copulation";
    case LastingEffect::CROPS_SKILL: return "farming";
    case LastingEffect::DISTILLATION_SKILL: return "distillation";
    case LastingEffect::SPIDER_SKILL: return "spider web weaving";
    case LastingEffect::EXPLORE_SKILL: return "exploring";
    case LastingEffect::EXPLORE_CAVES_SKILL: return "exploring caves";
    case LastingEffect::EXPLORE_NOCTURNAL_SKILL: return "exploring at night";
    case LastingEffect::BRIDGE_BUILDING_SKILL: return "bridge building";
    case LastingEffect::NAVIGATION_DIGGING_SKILL: return "digging";
    case LastingEffect::ON_FIRE: return "combustion";
    case LastingEffect::FROZEN: return "freezing";
    case LastingEffect::MAGIC_CANCELLATION: return "magic cancellation";
    case LastingEffect::SPELL_DAMAGE: return "magical damage";
    case LastingEffect::DISAPPEAR_DURING_DAY: return "night life";
    case LastingEffect::NO_CARRY_LIMIT: return "infinite carrying capacity";
    case LastingEffect::SPYING: return "spying";
    case LastingEffect::LIFE_SAVED: return "life saving";
    case LastingEffect::UNSTABLE: return "mental instability";
    case LastingEffect::OIL: return "oil";
    case LastingEffect::SWARMER: return "swarming";
    case LastingEffect::PSYCHIATRY: return "psychiatry";
    case LastingEffect::INVULNERABLE: return "invulnerability";
    case LastingEffect::TURNED_OFF: return "power off";
    case LastingEffect::DRUNK: return "booze";
    case LastingEffect::NO_FRIENDLY_FIRE: return "no friendly fire";
    case LastingEffect::POLYMORPHED: return "polymorphed";
  }
}

string LastingEffects::getDescription(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "This is no dream! This is really happening!";
    case LastingEffect::SLOWED: return "Causes unnaturally slow movement.";
    case LastingEffect::BLEEDING: return "Causes loss of health points over time.";
    case LastingEffect::SPEED: return "Grants an extra move every turn.";
    case LastingEffect::BLIND: return "Causes blindness";
    case LastingEffect::INVISIBLE: return "Makes you invisible to enemies.";
    case LastingEffect::PLAGUE: return "Decreases health every turn when it's above 50%. "
                                       "A small percent of creatures can die, others don't suffer from health loss.";
    case LastingEffect::PLAGUE_RESISTANT: return "Protects from plague infection";
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
    case LastingEffect::IMMOBILE: return "Creature does not move.";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "Allows enslaving as a prisoner, otherwise creature will die.";
    case LastingEffect::FIRE_RESISTANT: return "Protects from fire damage.";
    case LastingEffect::COLD_RESISTANT: return "Protects from ice damage.";
    case LastingEffect::ACID_RESISTANT: return "Protects from acid damage.";
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
    case LastingEffect::ARCHER_VISION: return "Allows to see and shoot through arrowslits.";
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
    case LastingEffect::HATE_ELVES:
    case LastingEffect::HATE_DRAGONS:
      return "Makes the target hostile to all "_s + LastingEffects::getHatedGroupName(type);
    case LastingEffect::FAST_CRAFTING: return "Increases crafting speed.";
    case LastingEffect::FAST_TRAINING: return "Increases training and studying speed.";
    case LastingEffect::SLOW_CRAFTING: return "Decreases crafting speed.";
    case LastingEffect::SLOW_TRAINING: return "Decreases training and studying speed.";
    case LastingEffect::ENTERTAINER: return "Makes jokes, increasing morale of nearby creatures.";
    case LastingEffect::BAD_BREATH: return "Decreases morale of all nearby creatures.";
    case LastingEffect::AMBUSH_SKILL: return "Can hide and ambush unsuspecting enemies. Press 'h' to hide on a tile that allows it.";
    case LastingEffect::SWIMMING_SKILL: return "Can cross water without drowning.";
    case LastingEffect::DISARM_TRAPS_SKILL: return "Can evade traps and disarm them.";
    case LastingEffect::CONSUMPTION_SKILL: return "Can absorb other creatures and retain their attributes.";
    case LastingEffect::COPULATION_SKILL: return "Can copulate with other creatures and give birth to hideous spawns.";
    case LastingEffect::CROPS_SKILL: return "Can farm crops.";
    case LastingEffect::SPIDER_SKILL: return "Can weave spider webs.";
    case LastingEffect::EXPLORE_SKILL: return "Can explore surroundings.";
    case LastingEffect::DISTILLATION_SKILL: return "Can distill alcohol.";
    case LastingEffect::EXPLORE_CAVES_SKILL: return "Can explore caves.";
    case LastingEffect::EXPLORE_NOCTURNAL_SKILL: return "Can explore surroundings at night.";
    case LastingEffect::BRIDGE_BUILDING_SKILL: return "Creature will try to build bridges when travelling somewhere.";
    case LastingEffect::NAVIGATION_DIGGING_SKILL: return "Creature will try to dig when travelling somewhere.";
    case LastingEffect::ON_FIRE: return "The creature is burning alive.";
    case LastingEffect::FROZEN: return "The creature is frozen and cannot move.";
    case LastingEffect::MAGIC_CANCELLATION: return "Prevents from casting any spells.";
    case LastingEffect::SPELL_DAMAGE: return "All dealt melee damage is transformed into magical damage.";
    case LastingEffect::DISAPPEAR_DURING_DAY: return "This creature is only active at night and disappears at dawn.";
    case LastingEffect::NO_CARRY_LIMIT: return "The creature can carry items without any weight limit.";
    case LastingEffect::SPYING: return "The creature can infiltrate enemy lines.";
    case LastingEffect::LIFE_SAVED: return "Prevents the death of the creature.";
    case LastingEffect::UNSTABLE: return "Creature may become insane when having witnessed the death of an ally.";
    case LastingEffect::OIL: return "Creature may be set on fire.";
    case LastingEffect::SWARMER: return "Grants damage and defense bonus for every other swarmer in vicinity.";
    case LastingEffect::PSYCHIATRY: return "Creature won't be attacked by insane creatures.";
    case LastingEffect::INVULNERABLE: return "Creature can't be harmed in combat.";
    case LastingEffect::TURNED_OFF: return "Creature requires more automaton engines built.";
    case LastingEffect::DRUNK: return "Compromises fighting abilities.";
    case LastingEffect::NO_FRIENDLY_FIRE: return "Arrows and other projectiles bypass allies and only hit enemies.";
    case LastingEffect::POLYMORPHED: return "Creature will revert to original form.";
  }
}

bool LastingEffects::canSee(const Creature* c1, const Creature* c2, GlobalTime time) {
  PROFILE_BLOCK("LastingEffects::canSee");
  return c1->getPosition().dist8(c2->getPosition()).value_or(5) < 5 && c2->getBody().hasBrain() &&
      c1->isAffected(LastingEffect::TELEPATHY, time);
}

bool LastingEffects::modifyIsEnemyResult(const Creature* c, const Creature* other, GlobalTime time, bool result) {
  PROFILE;
  auto isSpy = [&] (const Creature* c) {
    return c->isAffected(LastingEffect::SPYING, time) && c->hasAlternativeViewId();
  };
  if (c->isAffected(LastingEffect::PEACEFULNESS, time) || isSpy(c) || isSpy(other))
    return false;
  if (c->isAffected(LastingEffect::INSANITY, time) && !other->getStatus().contains(CreatureStatus::LEADER)
      && !other->isAffected(LastingEffect::PSYCHIATRY))
    return true;
  if (auto effect = other->getAttributes().getHatedByEffect())
    if (c->isAffected(*effect, time))
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
    case LastingEffect::HATE_DRAGONS:
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
    case LastingEffect::ARCHER_VISION:
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
    case LastingEffect::COLD_RESISTANT:
    case LastingEffect::ACID_RESISTANT:
    case LastingEffect::POISON:
    case LastingEffect::TELEPATHY:
      return 20;
    case LastingEffect::LIFE_SAVED:
      return 400;
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

double LastingEffects::getMoraleIncrease(const Creature* c, optional<GlobalTime> time) {
  PROFILE;
  double ret = 0;
  if (c->isAffected(LastingEffect::RESTED, time))
    ret += 0.1;
  else
    ret -= 0.1;
  if (c->isAffected(LastingEffect::SATIATED, time))
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
    case LastingEffect::FROZEN:
    case LastingEffect::PLAGUE:
    case LastingEffect::PLAGUE_RESISTANT:
    case LastingEffect::SPYING:
    case LastingEffect::LIFE_SAVED:
    case LastingEffect::INVULNERABLE:
    case LastingEffect::TURNED_OFF:
    case LastingEffect::DRUNK:
      return false;
    default:
      return true;
  }
}

bool LastingEffects::canWishFor(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::STUNNED:
    case LastingEffect::SUMMONED:
    case LastingEffect::INVULNERABLE:
    case LastingEffect::TURNED_OFF:
    case LastingEffect::DISAPPEAR_DURING_DAY:
      return false;
    default:
      return true;
  }
}

optional<FXVariantName> LastingEffects::getFX(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::SLEEP:
      return FXVariantName::SLEEP;
    case LastingEffect::INVULNERABLE:
    case LastingEffect::LIFE_SAVED:
      return FXVariantName::BUFF_WHITE;
    case LastingEffect::SPEED:
      return FXVariantName::BUFF_BLUE;
    case LastingEffect::SLOWED:
      return FXVariantName::DEBUFF_BLUE;

    case LastingEffect::COLD_RESISTANT:
    case LastingEffect::REGENERATION:
      return FXVariantName::BUFF_RED;
    case LastingEffect::BLEEDING:
      return FXVariantName::DEBUFF_RED;

    case LastingEffect::DAM_BONUS:
    case LastingEffect::DEF_BONUS:
      return FXVariantName::BUFF_YELLOW;
    case LastingEffect::SPELL_DAMAGE:
      return FXVariantName::BUFF_PURPLE;
    case LastingEffect::ACID_RESISTANT:
      return FXVariantName::BUFF_ORANGE;
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
      return FXVariantName::BUFF_SKY_BLUE;
    case LastingEffect::FAST_CRAFTING:
      return FXVariantName::BUFF_BROWN;
    case LastingEffect::FAST_TRAINING:
      return FXVariantName::BUFF_BROWN;
    case LastingEffect::SLOW_CRAFTING:
    case LastingEffect::SLOW_TRAINING:
    case LastingEffect::MAGIC_CANCELLATION:
      return FXVariantName::DEBUFF_BROWN;
    case LastingEffect::OIL:
      return FXVariantName::DEBUFF_BLACK;
    case LastingEffect::SWARMER:
      return FXVariantName::BUFF_GREEN1;
    case LastingEffect::PLAGUE_RESISTANT:
    case LastingEffect::POISON_RESISTANT:
      return FXVariantName::BUFF_GREEN2;
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
      return FXVariantName::DEBUFF_ORANGE;

    case LastingEffect::TELEPATHY:
    case LastingEffect::ELF_VISION:
    case LastingEffect::ARCHER_VISION:
    case LastingEffect::NIGHT_VISION:
      return FXVariantName::BUFF_PINK;
    case LastingEffect::BLIND:
    case LastingEffect::INSANITY:
      return FXVariantName::DEBUFF_PINK;

    case LastingEffect::PLAGUE:
    case LastingEffect::POISON:
      return FXVariantName::DEBUFF_GREEN2;
    case LastingEffect::PEACEFULNESS:
      return FXVariantName::PEACEFULNESS;
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED:
      return FXVariantName::DEBUFF_GREEN1;
    case LastingEffect::ON_FIRE:
      return FXVariantName::FIRE;
    case LastingEffect::DRUNK:
      return FXVariantName::DEBUFF_YELLOW;
    default:
      return none;
  }
}

Color LastingEffects::getColor(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::OIL:
      return Color::BLACK;
    case LastingEffect::INVULNERABLE:
    case LastingEffect::LIFE_SAVED:
      return Color::WHITE;
    case LastingEffect::SPEED:
      return Color::LIGHT_BLUE;
    case LastingEffect::DAM_BONUS:
    case LastingEffect::DEF_BONUS:
    case LastingEffect::DRUNK:
      return Color::YELLOW;
    case LastingEffect::INVISIBLE:
      return Color::WHITE;
    case LastingEffect::PLAGUE:
    case LastingEffect::POISON:
      return Color::GREEN;
    case LastingEffect::POISON_RESISTANT:
      return Color::YELLOW;
    case LastingEffect::ACID_RESISTANT:
      return Color::ORANGE;
    case LastingEffect::INSANITY:
      return Color::PINK;
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
      return Color::SKY_BLUE;
    case LastingEffect::COLD_RESISTANT:
    case LastingEffect::REGENERATION:
      return Color::RED;
    case LastingEffect::MAGIC_CANCELLATION:
      return Color::BROWN;
    case LastingEffect::SPELL_DAMAGE:
      return Color::PURPLE;
    case LastingEffect::FROZEN:
      return Color::BLUE;
    default:
      return Color::LIGHT_BLUE;
  }
}

optional<FXInfo> LastingEffects::getApplicationFX(LastingEffect effect) {
  return FXInfo(FXName::CIRCULAR_SPELL, getColor(effect));
}

bool LastingEffects::canProlong(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::PLAGUE:
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

static bool shouldAllyApplyInDanger(const Creature* victim, LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVULNERABLE:
    case LastingEffect::LIFE_SAVED:
    case LastingEffect::INVISIBLE:
    case LastingEffect::DAM_BONUS:
    case LastingEffect::DEF_BONUS:
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::SPEED:
    case LastingEffect::FLYING:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
    case LastingEffect::ELF_VISION:
    case LastingEffect::ARCHER_VISION:
    case LastingEffect::REGENERATION:
    case LastingEffect::WARNING:
      return true;
    case LastingEffect::TELEPATHY:
      return victim->isAffected(LastingEffect::BLIND);
    case LastingEffect::NIGHT_VISION:
      return victim->getPosition().getLight() < Vision::getDarknessVisionThreshold();
    default:
      return false;
  }
}

static bool shouldAllyApplyInNoDanger(const Creature* victim, LastingEffect effect) {
  switch (effect) {
    case LastingEffect::DRUNK:
      return true;
    default:
      return false;
  }
}

static bool shouldAllyApply(const Creature* victim, LastingEffect effect) {
  if (auto cancelled = getCancelledOneWay(effect))
    if (victim->isAffected(*cancelled) && getAdjective(*cancelled).bad)
      return true;
  switch (effect) {
    case LastingEffect::REGENERATION:
      return victim->getBody().canHeal(HealthType::FLESH);
    case LastingEffect::SATIATED:
    case LastingEffect::RESTED:
    case LastingEffect::FAST_CRAFTING:
    case LastingEffect::FAST_TRAINING:
    case LastingEffect::ENTERTAINER:
    case LastingEffect::AMBUSH_SKILL:
    case LastingEffect::SWIMMING_SKILL:
    case LastingEffect::DISARM_TRAPS_SKILL:
    case LastingEffect::CONSUMPTION_SKILL:
    case LastingEffect::COPULATION_SKILL:
    case LastingEffect::CROPS_SKILL:
    case LastingEffect::SPIDER_SKILL:
    case LastingEffect::EXPLORE_SKILL:
    case LastingEffect::EXPLORE_NOCTURNAL_SKILL:
    case LastingEffect::EXPLORE_CAVES_SKILL:
    case LastingEffect::BRIDGE_BUILDING_SKILL:
    case LastingEffect::NAVIGATION_DIGGING_SKILL:
    case LastingEffect::NO_CARRY_LIMIT:
      return true;
    default:
      return false;
  }
}

static bool shouldEnemyApply(const Creature* victim, LastingEffect effect) {
  if (auto cancelled = getCancelledOneWay(effect))
    if (victim->isAffected(*cancelled) && !getAdjective(*cancelled).bad)
      return true;
  return getAdjective(effect).bad;
}

EffectAIIntent LastingEffects::shouldAIApply(const Creature* victim, LastingEffect effect, bool isEnemy) {
  PROFILE_BLOCK("LastingEffects::shouldAIApply");
  if (!affects(victim, effect))
    return 0;
  if (shouldEnemyApply(victim, effect))
    return isEnemy ? 1 : -1;
  if (isEnemy)
    return -1;
  bool isDanger = [&] {
    if (auto intent = victim->getLastCombatIntent())
      return intent->time > *victim->getGlobalTime() - 5_visible;
    return false;
  }();
  if (isDanger) {
    if (shouldAllyApplyInDanger(victim, effect))
      return 1;
  } else
    if (shouldAllyApplyInNoDanger(victim, effect))
      return 1;
  return shouldAllyApply(victim, effect) ? 1 : 0;
}

AttrType LastingEffects::modifyMeleeDamageAttr(const Creature* attacker, AttrType type) {
  if (attacker->isAffected(LastingEffect::SPELL_DAMAGE) && type == AttrType::DAMAGE)
    return AttrType::SPELL_DAMAGE;
  return type;
}

static TimeInterval entangledTime(int strength) {
  return TimeInterval(max(5, 30 - strength / 2));
}

TimeInterval LastingEffects::getDuration(const Creature* c, LastingEffect e) {
  switch (e) {
    case LastingEffect::PLAGUE:
      return 1500_visible;
    case LastingEffect::SUMMONED:
      return 900_visible;
    case LastingEffect::PREGNANT:
      return 900_visible;
    case LastingEffect::NIGHT_VISION:
    case LastingEffect::ELF_VISION:
    case LastingEffect::ARCHER_VISION:
      return 60_visible;
    case LastingEffect::TIED_UP:
    case LastingEffect::WARNING:
    case LastingEffect::REGENERATION:
    case LastingEffect::TELEPATHY:
    case LastingEffect::BLEEDING:
      return 50_visible;
    case LastingEffect::ENTANGLED:
      return entangledTime(c->getAttr(AttrType::DAMAGE));
    case LastingEffect::HALLU:
    case LastingEffect::SLOWED:
    case LastingEffect::SPEED:
    case LastingEffect::RAGE:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::PANIC:
      return 15_visible;
    case LastingEffect::POISON:
      return 60_visible;
    case LastingEffect::DEF_BONUS:
    case LastingEffect::DAM_BONUS:
      return 40_visible;
    case LastingEffect::BLIND:
      return 15_visible;
    case LastingEffect::INVULNERABLE:
    case LastingEffect::INVISIBLE:
      return 15_visible;
    case LastingEffect::LIFE_SAVED:
    case LastingEffect::FROZEN:
    case LastingEffect::STUNNED:
    case LastingEffect::IMMOBILE:
      return 7_visible;
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::ACID_RESISTANT:
    case LastingEffect::FIRE_RESISTANT:
    case LastingEffect::SWARMER:
    case LastingEffect::POISON_RESISTANT:
    case LastingEffect::FLYING:
      return 60_visible;
    case LastingEffect::COLLAPSED:
      return 2_visible;
    case LastingEffect::FAST_CRAFTING:
    case LastingEffect::FAST_TRAINING:
    case LastingEffect::SLOW_CRAFTING:
    case LastingEffect::SLOW_TRAINING:
    case LastingEffect::SLEEP:
      return 200_visible;
    case LastingEffect::PEACEFULNESS:
    case LastingEffect::INSANITY:
      return 20_visible;
    case LastingEffect::MAGIC_VULNERABILITY:
    case LastingEffect::MELEE_VULNERABILITY:
    case LastingEffect::RANGED_VULNERABILITY:
    case LastingEffect::MAGIC_CANCELLATION:
    case LastingEffect::MAGIC_RESISTANCE:
    case LastingEffect::MELEE_RESISTANCE:
    case LastingEffect::RANGED_RESISTANCE:
    case LastingEffect::SUNLIGHT_VULNERABLE:
    case LastingEffect::OIL:
      return  25_visible;
    case LastingEffect::DRUNK:
      return  300_visible;
    case LastingEffect::SATIATED:
      return  500_visible;
    default:
      return  1000_visible;
  }
}
