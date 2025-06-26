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
        c->verb(TStringId("YOU_ARE_FLYING"), TStringId("IS_FLYING"));
        break;
      case LastingEffect::COLLAPSED:
        c->you(MsgType::COLLAPSE);
        break;
      case LastingEffect::PREGNANT:
      case LastingEffect::BLIND:
      case LastingEffect::POISON:
      case LastingEffect::SLOWED:
      case LastingEffect::STUNNED:
      case LastingEffect::TIED_UP:
      case LastingEffect::SLEEP_RESISTANT:
      case LastingEffect::ENTANGLED:
      case LastingEffect::POISON_RESISTANT:
      case LastingEffect::TELEPATHY:
      case LastingEffect::CAPTURE_RESISTANCE:
      case LastingEffect::SUNLIGHT_VULNERABLE:
      case LastingEffect::PLAGUE:
      case LastingEffect::FROZEN:
      case LastingEffect::PLAGUE_RESISTANT:
      case LastingEffect::OIL:
      case LastingEffect::RESTED:
      case LastingEffect::DRUNK:
      case LastingEffect::SATIATED:
        c->you(MsgType::ARE, getAdjective(effect));
        break;
      case LastingEffect::PANIC:
        c->you(MsgType::PANIC);
        break;
      case LastingEffect::RAGE:
        c->you(MsgType::RAGE);
        break;
      case LastingEffect::HALLU:
        if (!c->isAffected(LastingEffect::BLIND))
          c->privateMessage(TStringId("THE_WORLD_EXPLODES_INTO_COLORS"));
        else
          c->privateMessage(TStringId("HALLU_WHILE_BLIND_MESSAGE"));
        break;
      case LastingEffect::INVISIBLE:
        if (!c->isAffected(LastingEffect::BLIND))
          c->you(MsgType::TURN_INVISIBLE);
        else
          c->privateMessage(TStringId("INVISIBLE_WHILE_BLIND_MESSAGE"));
        break;
      case LastingEffect::SPEED:
        c->you(MsgType::ARE, TStringId("FASTER"));
        break;
      case LastingEffect::SLEEP:
        c->you(MsgType::FALL_ASLEEP);
        break;
      case LastingEffect::INSANITY:
      case LastingEffect::PEACEFULNESS:
        c->you(MsgType::BECOME, getAdjective(effect));
        break;
      case LastingEffect::ELF_VISION:
        c->verb(TStringId("YOU_CAN_SEE_THRU_TREES"), TStringId("CAN_SEE_THRU_TREES"));
        break;
      case LastingEffect::ARCHER_VISION:
        c->verb(TStringId("YOU_CAN_SEE_THRU_ARROWSLITS"), TStringId("CAN_SEE_THRU_ARROWSLITS"));
        break;
      case LastingEffect::NIGHT_VISION:
        c->verb(TStringId("YOU_CAN_SEE_IN_THE_DARK"), TStringId("CAN_SEE_IN_THE_DARK"));
        break;
      case LastingEffect::WARNING:
        c->verb(TStringId("YOU_ARE_AWARE_OF_DANGER"), TStringId("IS_AWARE_OF_DANGER"));
        break;
      case LastingEffect::FAST_TRAINING:
        c->verb(TStringId("YOU_FEEL_LIKE_WORKING_OUT"), TStringId("FEELS_LIKE_WORKING_OUT"));
        break;
      case LastingEffect::FAST_CRAFTING:
        c->verb(TStringId("YOU_FEEL_LIKE_DOING_SOME_WORK"), TStringId("FEELS_LIKE_DOING_SOME_WORK"));
        break;
      case LastingEffect::SLOW_TRAINING:
      case LastingEffect::SLOW_CRAFTING:
        c->verb(TStringId("YOU_FEEL_LAZY"), TStringId("FEELS_LAZY"));
        break;
      case LastingEffect::ENTERTAINER:
        c->verb(TStringId("YOU_FEEL_LIKE_CRACKING_A_JOKE"), TStringId("FEELS_LIKE_CRACKING_A_JOKE"));
        break;
      case LastingEffect::BAD_BREATH:
        c->verb(TStringId("YOUR_BREATH_STINKS"), TStringId("HIS_BREATH_STINKS"));
        break;
      case LastingEffect::MAGIC_CANCELLATION:
        c->verb(TStringId("YOU_ARE_UNABLE_TO_CAST_SPELLS"), TStringId("IS_UNABLE_TO_CAST_SPELLS"));
        break;
      case LastingEffect::ON_FIRE:
        c->verb(TStringId("YOU_BURNING"), TStringId("IS_BURNING"));
        break;
      case LastingEffect::NO_CARRY_LIMIT:
        c->verb(TStringId("YOUR_CARRY_CAPACITY_INCREASES"), TStringId("HIS_CARRY_CAPACITY_INCREASES"));
        break;
      case LastingEffect::SPYING:
        c->verb(TStringId("YOU_PUT_ON_SUNGLASSES"), TStringId("PUTS_ON_SUNGLASSES"), TString(his(c->getAttributes().getGender())));
        break;
      case LastingEffect::IMMOBILE:
        c->verb(TStringId("YOU_CANT_MOVE_ANYMORE"), TStringId("CANT_MOVE_ANYMORE"));
        break;
      case LastingEffect::LIFE_SAVED:
        c->verb(TStringId("YOUR_LIFE_WILL_BE_SAVED"), TStringId("HIS_LIFE_WILL_BE_SAVED"));
        break;
      case LastingEffect::UNSTABLE:
        c->verb(TStringId("YOU_ARE_MENTALLY_UNSTABLE"), TStringId("IS_MENTALLY_UNSTABLE"));
        break;
      case LastingEffect::SWARMER:
        c->verb(TStringId("YOU_ARE_SWARMER"), TStringId("IS_SWARMER"));
        break;
      case LastingEffect::PSYCHIATRY:
        c->you(MsgType::BECOME, TStringId("MORE_UNDERSTANDING"));
        break;
      case LastingEffect::TURNED_OFF:
        c->verb(TStringId("YOU_ARE_TURNED_OFF"), TStringId("IS_TURNED_OFF"));
        break;
      case LastingEffect::NO_FRIENDLY_FIRE:
        c->verb(TStringId("YOUR_PROJECTILES_WONT_HIT_ALLIES"), TStringId("HIS_PROJECTILES_WONT_HIT_ALLIES"));
        break;
      case LastingEffect::AGGRAVATES:
        c->verb(TStringId("YOU_AGGRAVATE_ENEMIES"), TStringId("AGGRAVATES_ENEMIES"));
        break;
      case LastingEffect::CAN_DANCE:
        c->verb(TStringId("YOU_FEEL_LIKE_A_DANCING_FOOL"), TStringId("LOOKS_LIKE_A_DANCING_FOOL"));
        break;
      case LastingEffect::RIDER:
        c->verb(TStringId("YOU_SPIN_YOUR_SPURS"), TStringId("SPINS_HIS_SPURS"), TString(his(c->getAttributes().getGender())));
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
        c->you(MsgType::ARE, TStringId("CURED_FROM_PLAGUE"));
      break;
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, TStringId("CURED_FROM_POISONING"));
      break;
    case LastingEffect::SLEEP:
      c->getAttributes().increaseBaseAttr(AttrType("DEFENSE"), 10);
      if (msg)
        c->you(MsgType::WAKE_UP);
      break;
    case LastingEffect::STUNNED:
      break;
    case LastingEffect::ON_FIRE:
      if (msg)
        c->verb(TStringId("YOU_ARE_EXTINGUISHED"), TStringId("IS_EXTINGUISHED"));
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
      if (factory && c->getBody().burnsIntrinsically(factory)) {
        // add the message before death otherwise it won't appear.
        c->verb(TStringId("YOU_BURN_TO_DEATH"), TStringId("BURNS_TO_DEATH"));
        c->dieNoReason(Creature::DropType::ONLY_INVENTORY);
      }
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
        c->verb(TStringId("YOU_ARE_MOVING_FASTER_AGAIN"), TStringId("IS_MOVING_FASTER_AGAIN"));
        break;
      case LastingEffect::SLEEP:
        c->you(MsgType::WAKE_UP);
        break;
      case LastingEffect::SPEED:
        c->verb(TStringId("YOU_ARE_MOVING_SLOW_AGAIN"), TStringId("IS_MOVING_SLOW_AGAIN"));
        break;
      case LastingEffect::PANIC:
      case LastingEffect::RAGE:
      case LastingEffect::HALLU:
        c->verb(TStringId("YOU_ARE_NO_LONGER_HALLU"), TStringId("IS_NO_LONGER_HALLU"));
        break;
      case LastingEffect::ENTANGLED:
        c->you(MsgType::BREAK_FREE);
        break;
      case LastingEffect::TIED_UP:
        c->you(MsgType::BREAK_FREE);
        break;
      case LastingEffect::BLIND:
        c->verb(TStringId("YOU_ARE_NO_LONGER_BLIND"), TStringId("IS_NO_LONGER_BLIND"));
        break;
      case LastingEffect::INVISIBLE:
        c->you(MsgType::TURN_VISIBLE);
        break;
      case LastingEffect::SLEEP_RESISTANT:
        c->verb(TStringId("YOU_ARE_NO_LONGER_SLEEP_RESISTANT"), TStringId("IS_NO_LONGER_SLEEP_RESISTANT"));
        break;
      case LastingEffect::POISON:
        c->verb(TStringId("YOU_ARE_NO_LONGER_POISONED"), TStringId("IS_NO_LONGER_POISONED"));
        break;
      case LastingEffect::PLAGUE:
        c->verb(TStringId("YOUR_INFECTION_SUBSIDES"), TStringId("HIS_INFECTION_SUBSIDES"));
        break;
      case LastingEffect::PLAGUE_RESISTANT:
        c->verb(TStringId("YOU_ARE_NO_LONGER_PLAGUE_RESISTANT"), TStringId("IS_NO_LONGER_PLAGUE_RESISTANT"));
        break;
      case LastingEffect::POISON_RESISTANT:
        c->verb(TStringId("YOU_ARE_NO_LONGER_POISON_RESISTANT"), TStringId("IS_NO_LONGER_POISON_RESISTANT"));
        break;
      case LastingEffect::FLYING:
        c->verb(TStringId("YOU_FALL_ON_THE"), TStringId("FALLS_ON_THE"), c->getPosition().getName());
        break;
      case LastingEffect::COLLAPSED:
        c->you(MsgType::STAND_UP);
        break;
      case LastingEffect::INSANITY:
        c->verb(TStringId("YOU_REGAIN_SANITY"), TStringId("REGAINS_SANITY"));
        break;
      case LastingEffect::PEACEFULNESS:
        c->verb(TStringId("YOU_ARE_LESS_PEACEFUL"), TStringId("IS_LESS_PEACEFUL"));
        break;
      case LastingEffect::CAPTURE_RESISTANCE:
        c->verb(TStringId("YOU_ARE_LESS_RESISTANT_TO_CAPTURING"), TStringId("IS_LESS_RESISTANT_TO_CAPTURING"));
        break;
      case LastingEffect::ELF_VISION:
        c->verb(TStringId("YOU_CANT_SEE_THRU_TREES"), TStringId("CANT_SEE_THRU_TREES"));
        break;
      case LastingEffect::ARCHER_VISION:
        c->verb(TStringId("YOU_CANT_SEE_THRU_ARROWSLITS"), TStringId("CANT_SEE_THRU_ARROWSLITS"));
        break;
      case LastingEffect::NIGHT_VISION:
        c->verb(TStringId("YOU_CANT_SEE_IN_THE_DARK"), TStringId("CANT_SEE_IN_THE_DARK"));
        break;
      case LastingEffect::WARNING:
        c->verb(TStringId("YOU_ARE_LESS_AWARE_OF_DANGER"), TStringId("IS_LESS_AWARE_OF_DANGER"));
        break;
      case LastingEffect::TELEPATHY:
        c->verb(TStringId("YOU_ARE_NO_LONGER_TELEPATHIC"), TStringId("IS_NO_LONGER_TELEPATHIC"));
        break;
      case LastingEffect::SUNLIGHT_VULNERABLE:
        c->verb(TStringId("YOU_ARE_NO_LONGER_VULNERABLE_TO_SUNLIGHT"), TStringId("IS_NO_LONGER_VULNERABLE_TO_SUNLIGHT"));
        break;
      case LastingEffect::SATIATED:
        c->verb(TStringId("YOU_ARE_NO_LONGER_SATIATED"), TStringId("IS_NO_LONGER_SATIATED"));
        break;
      case LastingEffect::RESTED:
        c->verb(TStringId("YOU_ARE_NO_LONGER_RESTED"), TStringId("IS_NO_LONGER_RESTED"));
        break;
      case LastingEffect::ENTERTAINER:
        c->verb(TStringId("YOU_ARE_NO_LONGER_FUNNY"), TStringId("IS_NO_LONGER_FUNNY"));
        break;
      case LastingEffect::BAD_BREATH:
        c->verb(TStringId("YOUR_BREATH_IS_FRESH_AGAIN"), TStringId("HIS_BREATH_IS_FRESH_AGAIN"));
        break;
      case LastingEffect::MAGIC_CANCELLATION:
        c->verb(TStringId("YOU_CAN_CAST_SPELLS_AGAIN"), TStringId("CAN_CAST_SPELLS_AGAIN"));
        break;
      case LastingEffect::ON_FIRE:
        c->verb(TStringId("YOU_STOP_BURNING"), TStringId("STOPS_BURNING"));
        break;
      case LastingEffect::FROZEN:
        c->verb(TStringId("YOU_ARE_NO_LONGER_FROZEN"), TStringId("IS_NO_LONGER_FROZEN"));
        break;
      case LastingEffect::DISAPPEAR_DURING_DAY:
        break;
      case LastingEffect::NO_CARRY_LIMIT:
        c->verb(TStringId("YOUR_CARRY_CAPACITY_IS_LIMITED_AGAIN"), TStringId("HIS_CARRY_CAPACITY_IS_LIMITED_AGAIN"));
        break;
      case LastingEffect::SPYING:
        c->verb(TStringId("YOU_REMOVE_SUNGLASSES"), TStringId("REMOVES_SUNGLASSES"), TString(his(c->getAttributes().getGender())));
        break;
      case LastingEffect::LIFE_SAVED:
        c->verb(TStringId("YOUR_LIFE_WILL_NO_LONGER_BE_SAVED"), TStringId("HIS_LIFE_WILL_NO_LONGER_BE_SAVED"));
        break;
      case LastingEffect::UNSTABLE:
        c->verb(TStringId("YOU_ARE_MENTALLY_STABLE_AGAIN"), TStringId("IS_MENTALLY_STABLE_AGAIN"));
        break;
      case LastingEffect::OIL:
        c->verb(TStringId("YOU_ARE_NO_LONGER_COVERED_IN_OIL"), TStringId("IS_NO_LONGER_COVERED_IN_OIL"));
        break;
      case LastingEffect::SWARMER:
        c->verb(TStringId("YOU_ARE_NO_LONGER_SWARMER"), TStringId("IS_NO_LONGER_SWARMER"));
        break;
      case LastingEffect::PSYCHIATRY:
        c->you(MsgType::BECOME, TStringId("LESS_UNDERSTANDING"));
        break;
      case LastingEffect::TURNED_OFF:
        c->verb(TStringId("YOU_ARE_TURNED_ON"), TStringId("IS_TURNED_ON"));
        break;
      case LastingEffect::DRUNK:
        c->verb(TStringId("YOU_SOBER_UP"), TStringId("SOBERS_UP"));
        break;
      case LastingEffect::NO_FRIENDLY_FIRE:
        c->verb(TStringId("YOUR_PROJECTILES_WILL_HIT_ALLIES"), TStringId("HIS_PROJECTILES_WILL_HIT_ALLIES"));
        break;
      case LastingEffect::POLYMORPHED:
        c->verb(TStringId("YOU_RETURN_TO_YOUR_PREVIOUS_FORM"), TStringId("RETURNS_TO_YOUR_PREVIOUS_FORM"));
        c->addFX({FXName::SPAWN});
        break;
      case LastingEffect::AGGRAVATES:
        c->verb(TStringId("YOU_NO_LONGER_AGGRAVATE_ENEMIES"), TStringId("NO_LONGER_AGGRAVATES_ENEMIES"));
        break;
      case LastingEffect::CAN_DANCE:
//        c->you(MsgType::YOUR, "legs have the same length again");
        break;
      case LastingEffect::STEED:
//        c->verb("no longer feel", "no longer feels", "tacked up");
        break;
      case LastingEffect::RIDER:
        c->verb(TStringId("YOUR_SPURS_STOP_SPINNING"), TStringId("HIS_SPURS_STOP_SPINNING"));
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
    TString name;
    bool bad;
  };
  Adjective operator "" _bad(const char* n, size_t) {\
    return {TStringId(n), true};
  }

  Adjective operator "" _good(const char* n, size_t) {\
    return {TStringId(n), false};
  }
}

static Adjective getAdjective(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVISIBLE: return "BUFF_ADJECTIVE_INVISIBLE"_good;
    case LastingEffect::RAGE: return "BUFF_ADJECTIVE_RAGE"_good;
    case LastingEffect::HALLU: return "BUFF_ADJECTIVE_HALLU"_good;
    case LastingEffect::SLEEP_RESISTANT: return "BUFF_ADJECTIVE_SLEEP_RESISTANT"_good;
    case LastingEffect::SPEED: return "BUFF_ADJECTIVE_SPEED"_good;
    case LastingEffect::POISON_RESISTANT: return "BUFF_ADJECTIVE_POISON_RESISTANT"_good;
    case LastingEffect::PLAGUE_RESISTANT: return "BUFF_ADJECTIVE_PLAGUE_RESISTANT"_good;
    case LastingEffect::FLYING: return "BUFF_ADJECTIVE_FLYING"_good;
    case LastingEffect::LIGHT_SOURCE: return "BUFF_ADJECTIVE_LIGHT_SOURCE"_good;
    case LastingEffect::DARKNESS_SOURCE: return "BUFF_ADJECTIVE_DARKNESS_SOURCE"_good;
    case LastingEffect::PREGNANT: return "BUFF_ADJECTIVE_PREGNANT"_good;
    case LastingEffect::CAPTURE_RESISTANCE: return "BUFF_ADJECTIVE_CAPTURE_RESISTANCE"_good;
    case LastingEffect::ELF_VISION: return "BUFF_ADJECTIVE_ELF_VISION"_good;
    case LastingEffect::ARCHER_VISION: return "BUFF_ADJECTIVE_ARCHER_VISION"_good;
    case LastingEffect::NIGHT_VISION: return "BUFF_ADJECTIVE_NIGHT_VISION"_good;
    case LastingEffect::WARNING: return "BUFF_ADJECTIVE_WARNING"_good;
    case LastingEffect::TELEPATHY: return "BUFF_ADJECTIVE_TELEPATHY"_good;
    case LastingEffect::SATIATED: return "BUFF_ADJECTIVE_SATIATED"_good;
    case LastingEffect::RESTED: return "BUFF_ADJECTIVE_RESTED"_good;
    case LastingEffect::FAST_CRAFTING: return "BUFF_ADJECTIVE_FAST_CRAFTING"_good;
    case LastingEffect::FAST_TRAINING: return "BUFF_ADJECTIVE_FAST_TRAINING"_good;
    case LastingEffect::ENTERTAINER: return "BUFF_ADJECTIVE_ENTERTAINER"_good;
    case LastingEffect::NO_CARRY_LIMIT: return "BUFF_ADJECTIVE_NO_CARRY_LIMIT"_good;
    case LastingEffect::SPYING: return "BUFF_ADJECTIVE_SPYING"_good;
    case LastingEffect::LIFE_SAVED: return "BUFF_ADJECTIVE_LIFE_SAVED"_good;
    case LastingEffect::SWARMER: return "BUFF_ADJECTIVE_SWARMER"_good;
    case LastingEffect::PSYCHIATRY: return "BUFF_ADJECTIVE_PSYCHIATRY"_good;
    case LastingEffect::DRUNK: return "BUFF_ADJECTIVE_DRUNK"_good;
    case LastingEffect::NO_FRIENDLY_FIRE: return "BUFF_ADJECTIVE_NO_FRIENDLY_FIRE"_good;
    case LastingEffect::POLYMORPHED: return "BUFF_ADJECTIVE_POLYMORPHED"_good;
    case LastingEffect::CAN_DANCE: return "BUFF_ADJECTIVE_CAN_DANCE"_good;
    case LastingEffect::STEED: return "BUFF_ADJECTIVE_STEED"_good;
    case LastingEffect::RIDER: return "BUFF_ADJECTIVE_RIDER"_good;

    case LastingEffect::PANIC: return "BUFF_ADJECTIVE_PANIC"_bad;
    case LastingEffect::PEACEFULNESS: return "BUFF_ADJECTIVE_PEACEFULNESS"_bad;
    case LastingEffect::POISON: return "BUFF_ADJECTIVE_POISON"_bad;
    case LastingEffect::PLAGUE: return "BUFF_ADJECTIVE_PLAGUE"_bad;
    case LastingEffect::SLEEP: return "BUFF_ADJECTIVE_SLEEP"_bad;
    case LastingEffect::ENTANGLED: return "BUFF_ADJECTIVE_ENTANGLED"_bad;
    case LastingEffect::TIED_UP: return "BUFF_ADJECTIVE_TIED_UP"_bad;
    case LastingEffect::IMMOBILE: return "BUFF_ADJECTIVE_IMMOBILE"_bad;
    case LastingEffect::SLOWED: return "BUFF_ADJECTIVE_SLOWED"_bad;
    case LastingEffect::INSANITY: return "BUFF_ADJECTIVE_INSANITY"_bad;
    case LastingEffect::BLIND: return "BUFF_ADJECTIVE_BLIND"_bad;
    case LastingEffect::STUNNED: return "BUFF_ADJECTIVE_STUNNED"_bad;
    case LastingEffect::COLLAPSED: return "BUFF_ADJECTIVE_COLLAPSED"_bad;
    case LastingEffect::SUNLIGHT_VULNERABLE: return "BUFF_ADJECTIVE_SUNLIGHT_VULNERABLE"_bad;
    case LastingEffect::SUMMONED: return "BUFF_ADJECTIVE_SUMMONED"_bad;
    case LastingEffect::SLOW_CRAFTING: return "BUFF_ADJECTIVE_SLOW_CRAFTING"_bad;
    case LastingEffect::SLOW_TRAINING: return "BUFF_ADJECTIVE_SLOW_TRAINING"_bad;
    case LastingEffect::BAD_BREATH: return "BUFF_ADJECTIVE_BAD_BREATH"_bad;
    case LastingEffect::ON_FIRE: return "BUFF_ADJECTIVE_ON_FIRE"_bad;
    case LastingEffect::FROZEN: return "BUFF_ADJECTIVE_FROZEN"_bad;
    case LastingEffect::MAGIC_CANCELLATION: return "BUFF_ADJECTIVE_MAGIC_CANCELLATION"_bad;
    case LastingEffect::DISAPPEAR_DURING_DAY: return "BUFF_ADJECTIVE_DISAPPEAR_DURING_DAY"_bad;
    case LastingEffect::UNSTABLE: return "BUFF_ADJECTIVE_UNSTABLE"_bad;
    case LastingEffect::OIL: return "BUFF_ADJECTIVE_OIL"_bad;
    case LastingEffect::TURNED_OFF: return "BUFF_ADJECTIVE_TURNED_OFF"_bad;
    case LastingEffect::AGGRAVATES: return "BUFF_ADJECTIVE_AGGRAVATES"_bad;
    case LastingEffect::LOCKED_POSITION: return "BUFF_ADJECTIVE_LOCKED_POSITION"_bad;
  }
}

bool LastingEffects::isConsideredBad(LastingEffect effect) {
  return ::getAdjective(effect).bad;
}

TString LastingEffects::getAdjective(LastingEffect effect) {
  return ::getAdjective(effect).name;
}

optional<TString> LastingEffects::getGoodAdjective(LastingEffect effect) {
  auto adjective = ::getAdjective(effect);
  if (!adjective.bad)
    return adjective.name;
  return none;
}

optional<TString> LastingEffects::getBadAdjective(LastingEffect effect) {
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
    c->verb(TStringId("HAVE_GONE_BERSERK"), TStringId("HAS_GONE_BERSERK"));
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
        c->verb(TStringId("YOUR_IDENTITY_IS_UNCOVERED"), TStringId("HIS_IDENTITY_IS_UNCOVERED"));
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
      c->verb(TStringId("YOU_ARE_BURNING"), TStringId("IS_BURNING"));
      if (c->getBody().getHealth() <= 0) {
        c->verb(TStringId("YOU_BURN_TO_DEATH"), TStringId("BURNS_TO_DEATH"));
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
              c->verb(TStringId("YOU_SUFFER_PLAGUE"), TStringId("SUFFERS_PLAGUE"));
              if (c->getBody().getHealth() <= 0) {
                c->verb(TStringId("YOU_DIE_OF_PLAGUE"), TStringId("DIES_OF_PLAGUE"));
                c->dieWithReason(TStringId("PLAGUE"));
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
        c->verb(TStringId("YOU_SUFFER_POISONING"), TStringId("SUFFERS_POISONING"), MessagePriority::HIGH);
        if (c->getBody().getHealth() <= 0) {
          c->verb(TStringId("YOU_DIE_OF_POISONING"), TStringId("DIES_OF_POISONING"));
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
        c->privateMessage(PlayerMessage(TStringId("YOU_SENSE_BIG_DANGER"), MessagePriority::HIGH));
      else if (isDanger)
        c->privateMessage(PlayerMessage(TStringId("YOU_SENSE_DANGER"), MessagePriority::HIGH));
      break;
    }
    case LastingEffect::SUNLIGHT_VULNERABLE:
      if (c->getPosition().sunlightBurns() && !c->isAffected(LastingEffect::FROZEN)) {
        c->verb(TStringId("YOU_ARE_BURNT_BY_THE_SUN"), TStringId("IS_BURNT_BY_THE_SUN"));
        if (Random.roll(10)) {
          c->verb(TStringId("YOUR_BODY_CRUMBLES_TO_DUST"), TStringId("HIS_BODY_CRUMBLES_TO_DUST"));
          c->dieWithReason(TStringId("KILLED_BY_SUNLIGHT"), Creature::DropType::ONLY_INVENTORY);
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
          c->verb(TStringId("CRACK_A_JOKE_ABOUT"), TStringId("CRACKS_A_JOKE_ABOUT"),
              TString(*factory->buffs.at(*hatedGroup).hatedGroupName));
        else
          c->verb(TStringId("CRACK_A_JOKE"), TStringId("CRACKS_A_JOKE"));
        for (auto other : others)
          if (other != c && !other->isAffected(LastingEffect::SLEEP)) {
            if (hatedGroup && *hatedGroup == other->getAttributes().getHatedByEffect()) {
              other->verb(TStringId("ARE_OFFENDED"), TStringId("IS_OFFENDED"));
              other->addEffect(BuffId("DEF_DEBUFF"), 100_visible);
            } else if (other->getBody().hasBrain(factory)) {
              other->verb(TStringId("LAUGH"), TStringId("LAUGHS"));
              other->addEffect(BuffId("HIGH_MORALE"), 100_visible);
            } else
              other->verb(TStringId("DONT_LAUGH"), TStringId("DOESNT_LAUGH"));
          }
      }
      break;
    case LastingEffect::BAD_BREATH:
      if (Random.roll(50)) {
        c->getPosition().globalMessage(TStringId("SOMETHING_STINKS_MESSAGE"));
        for (auto pos : c->getPosition().getRectangle(Rectangle::centered(7)))
          if (auto other = pos.getCreature())
            if (Random.roll(5)) {
              other->verb(TStringId("YOU_COVER_YOUR_NOSE"), TStringId("COVERS_HIS_NOSE"), TString(his(other->getAttributes().getGender())));
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

TString LastingEffects::getName(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return TStringId("BUFF_PREGNANT");
    case LastingEffect::SLOWED: return TStringId("BUFF_SLOWED");
    case LastingEffect::SPEED: return TStringId("BUFF_SPEED");
    case LastingEffect::BLIND: return TStringId("BUFF_BLIND");
    case LastingEffect::INVISIBLE: return TStringId("BUFF_INVISIBLE");
    case LastingEffect::POISON: return TStringId("BUFF_POISON");
    case LastingEffect::PLAGUE: return TStringId("BUFF_PLAGUE");
    case LastingEffect::POISON_RESISTANT: return TStringId("BUFF_POISON_RESISTANT");
    case LastingEffect::PLAGUE_RESISTANT: return TStringId("BUFF_PLAGUE_RESISTANT");
    case LastingEffect::FLYING: return TStringId("BUFF_FLYING");
    case LastingEffect::COLLAPSED: return TStringId("BUFF_COLLAPSED");
    case LastingEffect::PANIC: return TStringId("BUFF_PANIC");
    case LastingEffect::RAGE: return TStringId("BUFF_RAGE");
    case LastingEffect::HALLU: return TStringId("BUFF_HALLU");
    case LastingEffect::SLEEP_RESISTANT: return TStringId("BUFF_SLEEP_RESISTANT");
    case LastingEffect::SLEEP: return TStringId("BUFF_SLEEP");
    case LastingEffect::IMMOBILE: return TStringId("BUFF_IMMOBILE");
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED: return TStringId("BUFF_ENTANGLED");
    case LastingEffect::STUNNED: return TStringId("BUFF_STUNNED");
    case LastingEffect::INSANITY: return TStringId("BUFF_INSANITY");
    case LastingEffect::PEACEFULNESS: return TStringId("BUFF_PEACEFULNESS");
    case LastingEffect::CAPTURE_RESISTANCE: return TStringId("BUFF_CAPTURE_RESISTANCE");
    case LastingEffect::LIGHT_SOURCE: return TStringId("BUFF_LIGHT_SOURCE");
    case LastingEffect::DARKNESS_SOURCE: return TStringId("BUFF_DARKNESS_SOURCE");
    case LastingEffect::NIGHT_VISION: return TStringId("BUFF_NIGHT_VISION");
    case LastingEffect::ELF_VISION: return TStringId("BUFF_ELF_VISION");
    case LastingEffect::ARCHER_VISION: return TStringId("BUFF_ARCHER_VISION");
    case LastingEffect::WARNING: return TStringId("BUFF_WARNING");
    case LastingEffect::TELEPATHY: return TStringId("BUFF_TELEPATHY");
    case LastingEffect::SUNLIGHT_VULNERABLE: return TStringId("BUFF_SUNLIGHT_VULNERABLE");
    case LastingEffect::SATIATED: return TStringId("BUFF_SATIATED");
    case LastingEffect::RESTED: return TStringId("BUFF_RESTED");
    case LastingEffect::SUMMONED: return TStringId("BUFF_SUMMONED");
    case LastingEffect::FAST_CRAFTING: return TStringId("BUFF_FAST_CRAFTING");
    case LastingEffect::FAST_TRAINING: return TStringId("BUFF_FAST_TRAINING");
    case LastingEffect::SLOW_CRAFTING: return TStringId("BUFF_SLOW_CRAFTING");
    case LastingEffect::SLOW_TRAINING: return TStringId("BUFF_SLOW_TRAINING");
    case LastingEffect::ENTERTAINER: return TStringId("BUFF_ENTERTAINER");
    case LastingEffect::BAD_BREATH: return TStringId("BUFF_BAD_BREATH");
    case LastingEffect::ON_FIRE: return TStringId("BUFF_ON_FIRE");
    case LastingEffect::FROZEN: return TStringId("BUFF_FROZEN");
    case LastingEffect::MAGIC_CANCELLATION: return TStringId("BUFF_MAGIC_CANCELLATION");
    case LastingEffect::DISAPPEAR_DURING_DAY: return TStringId("BUFF_DISAPPEAR_DURING_DAY");
    case LastingEffect::NO_CARRY_LIMIT: return TStringId("BUFF_NO_CARRY_LIMIT");
    case LastingEffect::SPYING: return TStringId("BUFF_SPYING");
    case LastingEffect::LIFE_SAVED: return TStringId("BUFF_LIFE_SAVED");
    case LastingEffect::UNSTABLE: return TStringId("BUFF_UNSTABLE");
    case LastingEffect::OIL: return TStringId("BUFF_OIL");
    case LastingEffect::SWARMER: return TStringId("BUFF_SWARMER");
    case LastingEffect::PSYCHIATRY: return TStringId("BUFF_PSYCHIATRY");
    case LastingEffect::TURNED_OFF: return TStringId("BUFF_TURNED_OFF");
    case LastingEffect::DRUNK: return TStringId("BUFF_DRUNK");
    case LastingEffect::NO_FRIENDLY_FIRE: return TStringId("BUFF_NO_FRIENDLY_FIRE");
    case LastingEffect::POLYMORPHED: return TStringId("BUFF_POLYMORPHED");
    case LastingEffect::AGGRAVATES: return TStringId("BUFF_AGGRAVATES");
    case LastingEffect::CAN_DANCE: return TStringId("BUFF_CAN_DANCE");
    case LastingEffect::STEED: return TStringId("BUFF_STEED");
    case LastingEffect::RIDER: return TStringId("BUFF_RIDER");
    case LastingEffect::LOCKED_POSITION: return TStringId("BUFF_LOCKED_POSITION");
  }
}

TString LastingEffects::getDescription(LastingEffect type) {
  switch (type) {
    case LastingEffect::PREGNANT: return TStringId("BUFF_DESC_PREGNANT");
    case LastingEffect::SLOWED: return TStringId("BUFF_DESC_SLOWED");
    case LastingEffect::SPEED: return TStringId("BUFF_DESC_SPEED");
    case LastingEffect::BLIND: return TStringId("BUFF_DESC_BLIND");
    case LastingEffect::INVISIBLE: return TStringId("BUFF_DESC_INVISIBLE");
    case LastingEffect::PLAGUE: return TStringId("BUFF_DESC_PLAGUE");
    case LastingEffect::PLAGUE_RESISTANT: return TStringId("BUFF_DESC_PLAGUE_RESISTANT");
    case LastingEffect::POISON: return TStringId("BUFF_DESC_POISON");
    case LastingEffect::POISON_RESISTANT: return TStringId("BUFF_DESC_POISON_RESISTANT");
    case LastingEffect::FLYING: return TStringId("BUFF_DESC_FLYING");
    case LastingEffect::COLLAPSED: return TStringId("BUFF_DESC_COLLAPSED");
    case LastingEffect::PANIC: return TStringId("BUFF_DESC_PANIC");
    case LastingEffect::RAGE: return TStringId("BUFF_DESC_RAGE");
    case LastingEffect::HALLU: return TStringId("BUFF_DESC_HALLU");
    case LastingEffect::SLEEP_RESISTANT: return TStringId("BUFF_DESC_SLEEP_RESISTANT");
    case LastingEffect::SLEEP: return TStringId("BUFF_DESC_SLEEP");
    case LastingEffect::IMMOBILE: return TStringId("BUFF_DESC_IMMOBILE");
    case LastingEffect::TIED_UP: return TStringId("BUFF_DESC_TIED_UP");
    case LastingEffect::ENTANGLED: return TStringId("BUFF_DESC_ENTANGLED");
    case LastingEffect::STUNNED: return TStringId("BUFF_DESC_STUNNED");
    case LastingEffect::INSANITY: return TStringId("BUFF_DESC_INSANITY");
    case LastingEffect::PEACEFULNESS: return TStringId("BUFF_DESC_PEACEFULNESS");
    case LastingEffect::CAPTURE_RESISTANCE: return TStringId("BUFF_DESC_CAPTURE_RESISTANCE");
    case LastingEffect::DARKNESS_SOURCE: return TStringId("BUFF_DESC_DARKNESS_SOURCE");
    case LastingEffect::LIGHT_SOURCE: return TStringId("BUFF_DESC_LIGHT_SOURCE");
    case LastingEffect::NIGHT_VISION: return TStringId("BUFF_DESC_NIGHT_VISION");
    case LastingEffect::ELF_VISION: return TStringId("BUFF_DESC_ELF_VISION");
    case LastingEffect::ARCHER_VISION: return TStringId("BUFF_DESC_ARCHER_VISION");
    case LastingEffect::WARNING: return TStringId("BUFF_DESC_WARNING");
    case LastingEffect::TELEPATHY: return TStringId("BUFF_DESC_TELEPATHY");
    case LastingEffect::SUNLIGHT_VULNERABLE: return TStringId("BUFF_DESC_SUNLIGHT_VULNERABLE");
    case LastingEffect::SATIATED: return TStringId("BUFF_DESC_SATIATED");
    case LastingEffect::RESTED: return TStringId("BUFF_DESC_RESTED");
    case LastingEffect::SUMMONED: return TStringId("BUFF_DESC_SUMMONED");
    case LastingEffect::FAST_CRAFTING: return TStringId("BUFF_DESC_FAST_CRAFTING");
    case LastingEffect::FAST_TRAINING: return TStringId("BUFF_DESC_FAST_TRAINING");
    case LastingEffect::SLOW_CRAFTING: return TStringId("BUFF_DESC_SLOW_CRAFTING");
    case LastingEffect::SLOW_TRAINING: return TStringId("BUFF_DESC_SLOW_TRAINING");
    case LastingEffect::ENTERTAINER: return TStringId("BUFF_DESC_ENTERTAINER");
    case LastingEffect::BAD_BREATH: return TStringId("BUFF_DESC_BAD_BREATH");
    case LastingEffect::ON_FIRE: return TStringId("BUFF_DESC_ON_FIRE");
    case LastingEffect::FROZEN: return TStringId("BUFF_DESC_FROZEN");
    case LastingEffect::MAGIC_CANCELLATION: return TStringId("BUFF_DESC_MAGIC_CANCELLATION");
    case LastingEffect::DISAPPEAR_DURING_DAY: return TStringId("BUFF_DESC_DISAPPEAR_DURING_DAY");
    case LastingEffect::NO_CARRY_LIMIT: return TStringId("BUFF_DESC_NO_CARRY_LIMIT");
    case LastingEffect::SPYING: return TStringId("BUFF_DESC_SPYING");
    case LastingEffect::LIFE_SAVED: return TStringId("BUFF_DESC_LIFE_SAVED");
    case LastingEffect::UNSTABLE: return TStringId("BUFF_DESC_UNSTABLE");
    case LastingEffect::OIL: return TStringId("BUFF_DESC_OIL");
    case LastingEffect::SWARMER: return TStringId("BUFF_DESC_SWARMER");
    case LastingEffect::PSYCHIATRY: return TStringId("BUFF_DESC_PSYCHIATRY");
    case LastingEffect::TURNED_OFF: return TStringId("BUFF_DESC_TURNED_OFF");
    case LastingEffect::DRUNK: return TStringId("BUFF_DESC_DRUNK");
    case LastingEffect::NO_FRIENDLY_FIRE: return TStringId("BUFF_DESC_NO_FRIENDLY_FIRE");
    case LastingEffect::POLYMORPHED: return TStringId("BUFF_DESC_POLYMORPHED");
    case LastingEffect::AGGRAVATES: return TStringId("BUFF_DESC_AGGRAVATES");
    case LastingEffect::CAN_DANCE: return TStringId("BUFF_DESC_CAN_DANCE");
    case LastingEffect::STEED: return TStringId("BUFF_DESC_STEED");
    case LastingEffect::RIDER: return TStringId("BUFF_DESC_RIDER");
   case LastingEffect::LOCKED_POSITION: return TStringId("BUFF_DESC_LOCKED_POSITION");
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
