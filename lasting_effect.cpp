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
#include "content_factory.h"

static optional<LastingEffect> getCancelledOneWay(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::PLAGUE_RESISTANT:
      return LastingEffect::PLAGUE;
    case LastingEffect::POISON_RESISTANT:
      return LastingEffect::POISON;
    case LastingEffect::SLEEP_RESISTANT:
      return LastingEffect::SLEEP;
    default:
      return none;
  }
}

static optional<LastingEffect> getMutuallyExclusiveImpl(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::STEED:
      return LastingEffect::RIDER;
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

static optional<LastingEffect> getRequired(LastingEffect effect, const Creature *c, const ContentFactory* factory) {
  switch (effect) {
    case LastingEffect::ON_FIRE:
      if (c->getBody().burnsIntrinsically(factory))
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
    case LastingEffect::RESTED:
    case LastingEffect::SATIATED:
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), 2);
      break;
    case LastingEffect::SLEEP:
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), -10);
      break;
    case LastingEffect::PANIC:
      c->getAttributes().increaseBaseAttr(AttrType("DAMAGE"), -5);
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), 5);
      break;
    case LastingEffect::RAGE:
      c->getAttributes().increaseBaseAttr(AttrType("DAMAGE"), 5);
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), -5);
      break;
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
    case LastingEffect::COLLAPSED:
      if (c->getSteed())
        c->tryToDismount();
      break;
    default:
      break;
  }
  if (msg)
    switch (effect) {
      case LastingEffect::FLYING:
        c->you(MsgType::ARE, "flying!");
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
      case LastingEffect::INSANITY:
        c->you(MsgType::BECOME, "insane");
        break;
      case LastingEffect::PEACEFULNESS:
        c->you(MsgType::BECOME, "peaceful");
        break;
      case LastingEffect::CAPTURE_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to capturing");
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
      case LastingEffect::TURNED_OFF:
        c->you(MsgType::ARE, "turned off");
        break;
      case LastingEffect::DRUNK:
        c->you(MsgType::ARE, "drunk!");
        break;
      case LastingEffect::NO_FRIENDLY_FIRE:
        c->you(MsgType::YOUR, "projectiles won't hit allies");
        break;
      case LastingEffect::AGGRAVATES:
        c->verb("aggravate", "aggravates", "enemies");
        break;
      case LastingEffect::CAN_DANCE:
        c->you(MsgType::FEEL, "like a dancing fool");
        break;
      case LastingEffect::STEED:
        c->you(MsgType::FEEL, "tacked up");
        break;
      case LastingEffect::RIDER:
        c->verb("spin", "spins", his(c->getAttributes().getGender()) + " spurs"_s);
        break;
      default:
        break;
    }
}

bool LastingEffects::affects(const Creature* c, LastingEffect effect, const ContentFactory* factory) {
  if (c->getBody().isImmuneTo(effect, factory))
    return false;
  if (auto preventing = getPreventing(effect))
    if (c->isAffected(*preventing))
      return false;
  if (auto required = getRequired(effect, c, factory))
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
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), 10);
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
  auto factory = !!c->getGame() ? c->getGame()->getContentFactory() : nullptr;
  switch (effect) {
    case LastingEffect::RESTED:
    case LastingEffect::SATIATED:
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), -2);
      break;
    case LastingEffect::PANIC:
      c->getAttributes().increaseBaseAttr(AttrType("DAMAGE"), 5);
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), -5);
      break;
    case LastingEffect::RAGE:
      c->getAttributes().increaseBaseAttr(AttrType("DAMAGE"), -5);
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), 5);
      break;
    case LastingEffect::SLEEP:
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), 10);
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
      if (factory && c->getBody().burnsIntrinsically(factory))
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
      case LastingEffect::CAPTURE_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to capturing");
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
      case LastingEffect::ENTERTAINER:
        c->you(MsgType::ARE, "no longer funny");
        break;
      case LastingEffect::BAD_BREATH:
        c->verb("smell", "smells", "like flowers again");
        break;
      case LastingEffect::MAGIC_CANCELLATION:
        c->verb("can", "can", "cast spells again");
        break;
      case LastingEffect::ON_FIRE:
        if (factory && c->getBody().burnsIntrinsically(factory))
          c->verb("burn", "burns", "to death");
        else
          c->verb("stop", "stops", "burning");
        break;
      case LastingEffect::FROZEN:
        c->verb("thaw", "thaws");
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
      case LastingEffect::AGGRAVATES:
        c->verb("no longer aggravate", "no longer aggravates", "enemies");
        break;
      case LastingEffect::CAN_DANCE:
        c->you(MsgType::YOUR, "legs have the same length again");
        break;
      case LastingEffect::STEED:
        c->verb("no longer feel", "no longer feels", "tacked up");
        break;
      case LastingEffect::RIDER:
        c->you(MsgType::YOUR, "spurs stop spinning");
        break;
      default:
        break;
    }
}

static const int attrBonus = 3;

int LastingEffects::getAttrBonus(const Creature* c, AttrType type) {
  PROFILE_BLOCK("LastingEffects::getAttrBonus")
  int value = 0;
  auto time = c->getGlobalTime();
  auto modifyWithSpying = [c, time](int& value) {
    if (c->hasAlternativeViewId() && c->isAffected(LastingEffect::SPYING, time))
      value -= 99;
    else
      if (auto rider = c->getRider())
        if (rider->hasAlternativeViewId() && rider->isAffected(LastingEffect::SPYING, time))
          value -= 99;
  };
  static auto damageType = AttrType("DAMAGE");
  static auto rangedDamageType = AttrType("RANGED_DAMAGE");
  static auto spellDamageType = AttrType("SPELL_DAMAGE");
  static auto defenseType = AttrType("DEFENSE");
  if (type == damageType) {
    if (c->isAffected(LastingEffect::SWARMER, time))
      value += c->getPosition().countSwarmers() - 1;
    modifyWithSpying(value);
  } else
  if (type == rangedDamageType || type == spellDamageType) {
    modifyWithSpying(value);
  } else
  if (type == defenseType) {
    if (c->isAffected(LastingEffect::SWARMER, time))
      value += c->getPosition().countSwarmers() - 1;
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

static Adjective getAdjective(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVISIBLE: return "Invisible"_good;
    case LastingEffect::RAGE: return "Enraged"_good;
    case LastingEffect::HALLU: return "Hallucinating"_good;
    case LastingEffect::SLEEP_RESISTANT: return "Sleep resistant"_good;
    case LastingEffect::SPEED: return "Speed bonus"_good;
    case LastingEffect::POISON_RESISTANT: return "Poison resistant"_good;
    case LastingEffect::PLAGUE_RESISTANT: return "Plague resistant"_good;
    case LastingEffect::FLYING: return "Flying"_good;
    case LastingEffect::LIGHT_SOURCE: return "Source of light"_good;
    case LastingEffect::DARKNESS_SOURCE: return "Source of darkness"_good;
    case LastingEffect::PREGNANT: return "Pregnant"_good;
    case LastingEffect::CAPTURE_RESISTANCE: return "Resistant to capturing"_good;
    case LastingEffect::ELF_VISION: return "Can see through trees"_good;
    case LastingEffect::ARCHER_VISION: return "Can see through arrowslits"_good;
    case LastingEffect::NIGHT_VISION: return "Can see in the dark"_good;
    case LastingEffect::WARNING: return "Aware of danger"_good;
    case LastingEffect::TELEPATHY: return "Telepathic"_good;
    case LastingEffect::SATIATED: return "Satiated"_good;
    case LastingEffect::RESTED: return "Rested"_good;
    case LastingEffect::FAST_CRAFTING: return "Fast craftsman"_good;
    case LastingEffect::FAST_TRAINING: return "Fast trainee"_good;
    case LastingEffect::ENTERTAINER: return "Entertainer"_good;
    case LastingEffect::NO_CARRY_LIMIT: return "Infinite carrying capacity"_good;
    case LastingEffect::SPYING: return "Spy"_good;
    case LastingEffect::LIFE_SAVED: return "Life will be saved"_good;
    case LastingEffect::SWARMER: return "Swarmer"_good;
    case LastingEffect::PSYCHIATRY: return "Psychiatrist"_good;
    case LastingEffect::DRUNK: return "Drunk"_good;
    case LastingEffect::NO_FRIENDLY_FIRE: return "Arrows bypass allies"_good;
    case LastingEffect::POLYMORPHED: return "Polymorphed"_good;
    case LastingEffect::CAN_DANCE: return "Has rhythm"_good;
    case LastingEffect::STEED: return "Steed"_good;
    case LastingEffect::RIDER: return "Rider"_good;

    case LastingEffect::PANIC: return "Panic"_bad;
    case LastingEffect::PEACEFULNESS: return "Peaceful"_bad;
    case LastingEffect::POISON: return "Poisoned"_bad;
    case LastingEffect::PLAGUE: return "Infected with plague"_bad;
    case LastingEffect::SLEEP: return "Sleeping"_bad;
    case LastingEffect::ENTANGLED: return "Entangled"_bad;
    case LastingEffect::TIED_UP: return "Tied up"_bad;
    case LastingEffect::IMMOBILE: return "Immobile"_bad;
    case LastingEffect::SLOWED: return "Slowed"_bad;
    case LastingEffect::INSANITY: return "Insane"_bad;
    case LastingEffect::BLIND: return "Blind"_bad;
    case LastingEffect::STUNNED: return "Unconscious"_bad;
    case LastingEffect::COLLAPSED: return "Collapsed"_bad;
    case LastingEffect::SUNLIGHT_VULNERABLE: return "Vulnerable to sunlight"_bad;
    case LastingEffect::SUMMONED: return "Time to live"_bad;
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
    case LastingEffect::AGGRAVATES: return "Aggravates enemies"_bad;
    case LastingEffect::LOCKED_POSITION: return "Disabled position swap"_bad;
  }
}

bool LastingEffects::isConsideredBad(LastingEffect effect) {
  return ::getAdjective(effect).bad;
}

string LastingEffects::getAdjective(LastingEffect effect) {
  return ::getAdjective(effect).name;
}

optional<string> LastingEffects::getGoodAdjective(LastingEffect effect) {
  auto adjective = ::getAdjective(effect);
  if (!adjective.bad)
    return adjective.name;
  return none;
}

optional<std::string> LastingEffects::getBadAdjective(LastingEffect effect) {
  auto adjective = ::getAdjective(effect);
  if (adjective.bad)
    return adjective.name;
  return none;
}

bool LastingEffects::losesControl(const Creature* c, bool homeSite) {
  return c->isAffected(LastingEffect::TURNED_OFF)
      || c->isAffected(LastingEffect::STUNNED)
      || (homeSite && c->isAffected(LastingEffect::SLEEP));
}

bool LastingEffects::doesntMove(const Creature* c) {
  return losesControl(c, false)
      || c->isAffected(LastingEffect::SLEEP)
      || c->isAffected(LastingEffect::FROZEN);
}

bool LastingEffects::restrictedMovement(const Creature* c) {
  auto steed = c->getSteed();
  return doesntMove(c) || (steed && restrictedMovement(steed))
      || (c->getSteed() && restrictedMovement(c->getSteed()))
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

bool LastingEffects::inheritsFromSteed(LastingEffect e) {
  switch (e) {
    case LastingEffect::SPEED:
    case LastingEffect::SLOWED:
    case LastingEffect::FLYING:
      return true;
    default:
      return false;
  }
}

double LastingEffects::modifyCreatureDefense(const Creature* c, LastingEffect e, double defense, AttrType damageAttr) {
  auto multiplyFor = [&](AttrType attr, double m) {
    if (damageAttr == attr)
      return defense * m;
    else
      return defense;
  };
  double baseMultiplier = 1.3;
  switch (e) {
    case LastingEffect::CAPTURE_RESISTANCE:
      if (c->isCaptureOrdered())
        return defense * baseMultiplier;
      break;
    default:
      break;
  }
  return defense;
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
  auto factory = c->getGame()->getContentFactory();
  switch (effect) {
    case LastingEffect::ENTANGLED:
      if (Random.chance(0.002 * max(15, c->getAttr(AttrType("DAMAGE")))))
        c->removeEffect(LastingEffect::ENTANGLED);
      break;
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
    case LastingEffect::ON_FIRE:
      c->getBody().bleed(c, 0.03);
      c->secondPerson(PlayerMessage("You are burning alive!", MessagePriority::HIGH));
      c->thirdPerson(PlayerMessage(c->getName().the() + " is burning alive.", MessagePriority::NORMAL));
      if (c->getBody().getHealth() <= 0) {
        c->verb("burn", "burns", "to death");
        c->dieWithLastAttacker();
        return true;
      }
      break;
    case LastingEffect::PLAGUE:
      if (!c->isAffected(LastingEffect::FROZEN)) {
        int sample = (int) std::abs(c->getUniqueId().getGenericId() % 10000);
        bool suffers = sample >= 1000;
        bool canDie = sample >= 9000 && !c->getStatus().contains(CreatureStatus::LEADER);
        for (auto pos : c->getPosition().neighbors8())
          if (auto other = pos.getCreature())
            if (Random.roll(10)) {
              other->addEffect(LastingEffect::PLAGUE, getDuration(LastingEffect::PLAGUE));
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
            int diff = enemy->getAttr(AttrType("DAMAGE")) - c->getAttr(AttrType("DEFENSE"));
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
        optional<BuffId> hatedGroup;
        vector<BuffId> hateEffects;
        for (auto& buff : factory->buffs)
          if (buff.second.hatedGroupName)
            hateEffects.push_back(buff.first);
        for (auto& buff : hateEffects)
          if (c->isAffected(buff) || (c->getAttributes().getHatedByEffect() != buff &&
              Random.roll(10 * hateEffects.size()))) {
            hatedGroup = buff;
            break;
          }
        if (hatedGroup)
          jokeText.append(" about "_s + *factory->buffs.at(*hatedGroup).hatedGroupName);
        c->verb("crack", "cracks", jokeText);
        for (auto other : others)
          if (other != c && !other->isAffected(LastingEffect::SLEEP)) {
            if (hatedGroup && *hatedGroup == other->getAttributes().getHatedByEffect()) {
              other->you(MsgType::ARE, "offended");
              other->addEffect(BuffId("DEF_DEBUFF"), 100_visible);
            } else if (other->getBody().hasBrain(factory)) {
              other->verb("laugh", "laughs");
              other->addEffect(BuffId("HIGH_MORALE"), 100_visible);
            } else
              other->verb("don't", "doesn't", "laugh");
          }
      }
      break;
    case LastingEffect::BAD_BREATH:
      if (Random.roll(50)) {
        c->getPosition().globalMessage("The smell!");
        for (auto pos : c->getPosition().getRectangle(Rectangle::centered(7)))
          if (auto other = pos.getCreature())
            if (Random.roll(5)) {
              other->verb("cover your", "covers "_s + his(other->getAttributes().getGender()), "nose");
              other->addEffect(BuffId("DEF_DEBUFF"), 10_visible);
            }
      }
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
    case LastingEffect::SLEEP: return "sleep";
    case LastingEffect::IMMOBILE: return "immobility";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "stunning";
    case LastingEffect::INSANITY: return "insanity";
    case LastingEffect::PEACEFULNESS: return "love";
    case LastingEffect::CAPTURE_RESISTANCE: return "capture resistance";
    case LastingEffect::LIGHT_SOURCE: return "light";
    case LastingEffect::DARKNESS_SOURCE: return "darkness";
    case LastingEffect::NIGHT_VISION: return "night vision";
    case LastingEffect::ELF_VISION: return "elf vision";
    case LastingEffect::ARCHER_VISION: return "arrowslit vision";
    case LastingEffect::WARNING: return "warning";
    case LastingEffect::TELEPATHY: return "telepathy";
    case LastingEffect::SUNLIGHT_VULNERABLE: return "sunlight vulnerability";
    case LastingEffect::SATIATED: return "satiety";
    case LastingEffect::RESTED: return "wakefulness";
    case LastingEffect::SUMMONED: return "time to live";
    case LastingEffect::FAST_CRAFTING: return "fast crafting";
    case LastingEffect::FAST_TRAINING: return "fast training";
    case LastingEffect::SLOW_CRAFTING: return "slow crafting";
    case LastingEffect::SLOW_TRAINING: return "slow training";
    case LastingEffect::ENTERTAINER: return "entertainment";
    case LastingEffect::BAD_BREATH: return "smelly breath";
    case LastingEffect::ON_FIRE: return "combustion";
    case LastingEffect::FROZEN: return "freezing";
    case LastingEffect::MAGIC_CANCELLATION: return "magic cancellation";
    case LastingEffect::DISAPPEAR_DURING_DAY: return "night life";
    case LastingEffect::NO_CARRY_LIMIT: return "infinite carrying capacity";
    case LastingEffect::SPYING: return "spying";
    case LastingEffect::LIFE_SAVED: return "life saving";
    case LastingEffect::UNSTABLE: return "mental instability";
    case LastingEffect::OIL: return "oil";
    case LastingEffect::SWARMER: return "swarming";
    case LastingEffect::PSYCHIATRY: return "psychiatry";
    case LastingEffect::TURNED_OFF: return "power off";
    case LastingEffect::DRUNK: return "booze";
    case LastingEffect::NO_FRIENDLY_FIRE: return "no friendly fire";
    case LastingEffect::POLYMORPHED: return "polymorphed";
    case LastingEffect::AGGRAVATES: return "aggravation";
    case LastingEffect::CAN_DANCE: return "dancing";
    case LastingEffect::STEED: return "mounting";
    case LastingEffect::RIDER: return "riding";
    case LastingEffect::LOCKED_POSITION: return "disabled position swap";
  }
}

string LastingEffects::getDescription(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return "This is no dream! This is really happening!";
    case LastingEffect::SLOWED: return "Causes unnaturally slow movement.";
    case LastingEffect::SPEED: return "Grants an extra move every turn.";
    case LastingEffect::BLIND: return "Causes blindness";
    case LastingEffect::INVISIBLE: return "Makes you invisible to enemies.";
    case LastingEffect::PLAGUE: return "Decreases health every turn when it's above 50%. "
                                       "A small percent of creatures can die, others don't suffer from health loss.";
    case LastingEffect::PLAGUE_RESISTANT: return "Protects from plague infection.";
    case LastingEffect::POISON: return "Decreases health every turn by a little bit.";
    case LastingEffect::POISON_RESISTANT: return "Gives poison resistance.";
    case LastingEffect::FLYING: return "Causes levitation.";
    case LastingEffect::COLLAPSED: return "Moving across tiles takes three times longer.";
    case LastingEffect::PANIC: return "Increases defense and lowers damage.";
    case LastingEffect::RAGE: return "Increases damage and lowers defense.";
    case LastingEffect::HALLU: return "Causes hallucinations.";
    case LastingEffect::SLEEP_RESISTANT: return "Prevents being put to sleep.";
    case LastingEffect::SLEEP: return "Puts to sleep.";
    case LastingEffect::IMMOBILE: return "Creature does not move.";
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return "web";
    case LastingEffect::STUNNED: return "Allows enslaving as a prisoner, otherwise creature will die.";
    case LastingEffect::INSANITY: return "Makes the target hostile to every creature.";
    case LastingEffect::PEACEFULNESS: return "Makes the target friendly to every creature.";
    case LastingEffect::CAPTURE_RESISTANCE: return "Increases defense by 30% when capture order is placed.";
    case LastingEffect::DARKNESS_SOURCE: return "Causes the closest vicinity to become dark. Protects undead from sunlight.";
    case LastingEffect::LIGHT_SOURCE: return "Casts light on the closest surroundings.";
    case LastingEffect::NIGHT_VISION: return "Gives vision in the dark at full distance.";
    case LastingEffect::ELF_VISION: return "Allows to see and shoot through trees.";
    case LastingEffect::ARCHER_VISION: return "Allows to see and shoot through arrowslits.";
    case LastingEffect::WARNING: return "Warns about dangerous enemies and traps.";
    case LastingEffect::TELEPATHY: return "Allows you to detect other creatures with brains.";
    case LastingEffect::SUNLIGHT_VULNERABLE: return "Sunlight makes your body crumble to dust.";
    case LastingEffect::SATIATED: return "Increases morale and improves defense by +1.";
    case LastingEffect::RESTED: return "Increases morale and improves defense by +1.";
    case LastingEffect::SUMMONED: return "Will disappear after the given number of turns.";
    case LastingEffect::FAST_CRAFTING: return "Increases crafting speed.";
    case LastingEffect::FAST_TRAINING: return "Increases training and studying speed.";
    case LastingEffect::SLOW_CRAFTING: return "Decreases crafting speed.";
    case LastingEffect::SLOW_TRAINING: return "Decreases training and studying speed.";
    case LastingEffect::ENTERTAINER: return "Makes jokes, increasing morale of nearby creatures.";
    case LastingEffect::BAD_BREATH: return "Decreases morale of all nearby creatures.";
    case LastingEffect::ON_FIRE: return "The creature is burning alive.";
    case LastingEffect::FROZEN: return "The creature is frozen and cannot move.";
    case LastingEffect::MAGIC_CANCELLATION: return "Prevents from casting any spells.";
    case LastingEffect::DISAPPEAR_DURING_DAY: return "This creature is only active at night and disappears at dawn.";
    case LastingEffect::NO_CARRY_LIMIT: return "The creature can carry items without any weight limit.";
    case LastingEffect::SPYING: return "The creature can infiltrate enemy lines.";
    case LastingEffect::LIFE_SAVED: return "Prevents the death of the creature.";
    case LastingEffect::UNSTABLE: return "Creature may become insane after witnessing the death of an ally.";
    case LastingEffect::OIL: return "Creature is covered in oil and may be set on fire.";
    case LastingEffect::SWARMER: return "Grants damage and defense bonus for every other swarmer in vicinity.";
    case LastingEffect::PSYCHIATRY: return "Creature won't be attacked by insane creatures.";
    case LastingEffect::TURNED_OFF: return "Creature requires more automaton engines built.";
    case LastingEffect::DRUNK: return "Compromises fighting abilities.";
    case LastingEffect::NO_FRIENDLY_FIRE: return "Arrows and other projectiles bypass allies and only hit enemies.";
    case LastingEffect::POLYMORPHED: return "Creature will revert to original form.";
    case LastingEffect::AGGRAVATES: return "Makes enemies more aggressive and more likely to attack the base.";
    case LastingEffect::CAN_DANCE: return "Can dance all night long.";
    case LastingEffect::STEED: return "Can carry a rider.";
    case LastingEffect::RIDER: return "Can ride a steed.";
    case LastingEffect::LOCKED_POSITION: return "Creatures can't swap position with this automaton.";
  }
}

bool LastingEffects::canSee(const Creature* c1, const Creature* c2, GlobalTime time) {
  PROFILE_BLOCK("LastingEffects::canSee");
  return c1->getPosition().dist8(c2->getPosition()).value_or(5) < 5 &&
      c2->getBody().hasBrain(c1->getGame()->getContentFactory()) &&
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
    if (isAffected(c, *effect, time))
      return true;
  return result;
}

int LastingEffects::getPrice(LastingEffect e) {
  switch (e) {
    case LastingEffect::INSANITY:
    case LastingEffect::PEACEFULNESS:
    case LastingEffect::HALLU:
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
    case LastingEffect::FAST_CRAFTING:
    case LastingEffect::FAST_TRAINING:
      return 12;
    case LastingEffect::BLIND:
      return 16;
    case LastingEffect::SLOWED:
    case LastingEffect::POISON_RESISTANT:
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::POISON:
    case LastingEffect::TELEPATHY:
      return 20;
    case LastingEffect::LIFE_SAVED:
      return 400;
    case LastingEffect::INVISIBLE:
    case LastingEffect::CAPTURE_RESISTANCE:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::PREGNANT:
    case LastingEffect::FLYING:
    default:
      return 24;
  }
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
    case LastingEffect::SATIATED:
    case LastingEffect::RESTED:
    case LastingEffect::SUMMONED:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::PREGNANT:
    case LastingEffect::ON_FIRE:
    case LastingEffect::FROZEN:
    case LastingEffect::PLAGUE:
    case LastingEffect::PLAGUE_RESISTANT:
    case LastingEffect::SPYING:
    case LastingEffect::LIFE_SAVED:
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
    case LastingEffect::LIFE_SAVED:
      return FXVariantName::BUFF_WHITE;
    case LastingEffect::SPEED:
      return FXVariantName::BUFF_BLUE;
    case LastingEffect::SLOWED:
      return FXVariantName::DEBUFF_BLUE;

    case LastingEffect::CAPTURE_RESISTANCE:
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
    case LastingEffect::AGGRAVATES:
      return FXVariantName::DEBUFF_BLACK;
    default:
      return none;
  }
}

Color LastingEffects::getColor(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::OIL:
      return Color::BLACK;
    case LastingEffect::LIFE_SAVED:
      return Color::WHITE;
    case LastingEffect::SPEED:
      return Color::LIGHT_BLUE;
    case LastingEffect::DRUNK:
      return Color::YELLOW;
    case LastingEffect::AGGRAVATES:
      return Color::BLACK;
    case LastingEffect::INVISIBLE:
      return Color::WHITE;
    case LastingEffect::PLAGUE:
    case LastingEffect::POISON:
      return Color::GREEN;
    case LastingEffect::POISON_RESISTANT:
      return Color::YELLOW;
    case LastingEffect::INSANITY:
      return Color::PINK;
    case LastingEffect::CAPTURE_RESISTANCE:
      return Color::SKY_BLUE;
    case LastingEffect::MAGIC_CANCELLATION:
      return Color::BROWN;
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
    if (isAffected(c, *e))
      return false;
  return true;
}

bool LastingEffects::shouldAllyApplyInDanger(const Creature* victim, LastingEffect effect) {
  switch (effect) {
    case LastingEffect::LIFE_SAVED:
    case LastingEffect::INVISIBLE:
    case LastingEffect::SLEEP_RESISTANT:
    case LastingEffect::SPEED:
    case LastingEffect::FLYING:
    case LastingEffect::LIGHT_SOURCE:
    case LastingEffect::DARKNESS_SOURCE:
    case LastingEffect::CAPTURE_RESISTANCE:
    case LastingEffect::ELF_VISION:
    case LastingEffect::ARCHER_VISION:
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

bool LastingEffects::shouldEnemyApply(const Creature* victim, LastingEffect effect) {
  if (auto cancelled = getCancelledOneWay(effect))
    if (victim->isAffected(*cancelled) && !::getAdjective(*cancelled).bad)
      return true;
  return ::getAdjective(effect).bad;
}

TimeInterval LastingEffects::getDuration(LastingEffect e) {
  PROFILE;
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
    case LastingEffect::TELEPATHY:
    case LastingEffect::ENTANGLED:
    case LastingEffect::ON_FIRE:
      return 30_visible;
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
    case LastingEffect::BLIND:
      return 15_visible;
    case LastingEffect::INVISIBLE:
      return 15_visible;
    case LastingEffect::LIFE_SAVED:
    case LastingEffect::FROZEN:
    case LastingEffect::STUNNED:
    case LastingEffect::IMMOBILE:
      return 7_visible;
    case LastingEffect::SLEEP_RESISTANT:
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
      return 5_visible;
    case LastingEffect::INSANITY:
      return 20_visible;
    case LastingEffect::MAGIC_CANCELLATION:
    case LastingEffect::CAPTURE_RESISTANCE:
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
