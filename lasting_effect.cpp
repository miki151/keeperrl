#include "stdafx.h"
#include "lasting_effect.h"
#include "creature.h"
#include "view_object.h"
#include "player_message.h"
#include "creature_attributes.h"
#include "body.h"

static optional<LastingEffect> getCancelled(LastingEffect effect) {
  auto getCancelledOneWay = [] (LastingEffect effect) -> optional<LastingEffect> {
    switch (effect) {
      case LastingEffect::POISON_RESISTANT:
        return LastingEffect::POISON;
      default:
        return none;
    }
  };
  auto getCancelledBothWays = [] (LastingEffect effect) -> optional<LastingEffect> {
    switch (effect) {
      case LastingEffect::PANIC:
        return LastingEffect::RAGE;
      case LastingEffect::SPEED:
        return LastingEffect::SLOWED;
      default:
        return none;
    }
  };
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

static optional<ViewObject::Modifier> getViewObjectModifier(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVISIBLE:
      return ViewObjectModifier::INVISIBLE;
    default:
      return none;
  }
}

void LastingEffects::onAffected(WCreature c, LastingEffect effect, bool msg) {
  if (auto e = getCancelled(effect))
    c->removeEffect(*e, false);
  if (auto mod = getViewObjectModifier(effect))
    c->modViewObject().setModifier(*mod);
  if (msg)
    switch (effect) {
      case LastingEffect::FLYING:
        c->you(MsgType::ARE, "flying!"); break;
      case LastingEffect::BLEEDING:
        c->secondPerson("You start bleeding");
        c->thirdPerson(c->getName().the() + " starts bleeding");
        break;
      case LastingEffect::COLLAPSED:
        c->you(MsgType::COLLAPSE); break;
      case LastingEffect::PREGNANT:
        c->you(MsgType::ARE, "pregnant!"); break;
      case LastingEffect::STUNNED:
        c->you(MsgType::ARE, "stunned"); break;
      case LastingEffect::PANIC:
        c->you(MsgType::PANIC, ""); break;
      case LastingEffect::RAGE:
        c->you(MsgType::RAGE, ""); break;
      case LastingEffect::HALLU:
        if (!c->isAffected(LastingEffect::BLIND))
          c->privateMessage("The world explodes into colors!");
        else
          c->privateMessage("You feel as if a party had started without you.");
        break;
      case LastingEffect::BLIND:
        c->you(MsgType::ARE, "blind!"); break;
      case LastingEffect::INVISIBLE:
        if (!c->isAffected(LastingEffect::BLIND))
          c->you(MsgType::TURN_INVISIBLE, "");
        else
          c->privateMessage("You feel like a child again.");
        break;
      case LastingEffect::POISON:
        c->you(MsgType::ARE, "poisoned"); break;
      case LastingEffect::DAM_BONUS:
        c->you(MsgType::FEEL, "more dangerous"); break;
      case LastingEffect::DEF_BONUS:
        c->you(MsgType::FEEL, "more protected"); break;
      case LastingEffect::SPEED:
        c->you(MsgType::ARE, "moving faster"); break;
      case LastingEffect::SLOWED:
        c->you(MsgType::ARE, "moving more slowly"); break;
      case LastingEffect::TIED_UP:
        c->you(MsgType::ARE, "tied up"); break;
      case LastingEffect::ENTANGLED:
        c->you(MsgType::ARE, "entangled in a web"); break;
      case LastingEffect::SLEEP:
        c->you(MsgType::FALL_ASLEEP, ""); break;
      case LastingEffect::POISON_RESISTANT:
        c->you(MsgType::ARE, "now poison resistant"); break;
      case LastingEffect::FIRE_RESISTANT:
        c->you(MsgType::ARE, "now fire resistant"); break;
      case LastingEffect::INSANITY:
        c->you(MsgType::BECOME, "insane"); break;
      case LastingEffect::DARKNESS_SOURCE: break;
      case LastingEffect::MAGIC_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to magical attacks"); break;
      case LastingEffect::MELEE_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to melee attacks"); break;
      case LastingEffect::RANGED_RESISTANCE:
        c->you(MsgType::ARE, "now resistant to ranged attacks"); break;
      case LastingEffect::MAGIC_VULNERABILITY:
        c->you(MsgType::ARE, "now vulnerable to magical attacks"); break;
      case LastingEffect::MELEE_VULNERABILITY:
        c->you(MsgType::ARE, "now vulnerable to melee attacks"); break;
      case LastingEffect::RANGED_VULNERABILITY:
        c->you(MsgType::ARE, "now vulnerable to ranged attacks"); break;
      case LastingEffect::ELF_VISION:
        c->you("can see through trees"); break;
      case LastingEffect::NIGHT_VISION:
        c->you("can see in the dark"); break;
    }
}

bool LastingEffects::affects(WConstCreature c, LastingEffect effect) {
  switch (effect) {
    case LastingEffect::RAGE:
    case LastingEffect::PANIC:
      return !c->isAffected(LastingEffect::SLEEP);
    case LastingEffect::POISON:
      return !c->isAffected(LastingEffect::POISON_RESISTANT);
    default:
      return true;
  }
}

optional<LastingEffect> LastingEffects::getSuppressor(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::COLLAPSED:
      return LastingEffect::FLYING;
    default:
      return none;
  }
}

void LastingEffects::onRemoved(WCreature c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, "cured from poisoning");
      break;
    default:
      onTimedOut(c, effect, msg); break;
  }
}

void LastingEffects::onTimedOut(WCreature c, LastingEffect effect, bool msg) {
  if (auto mod = getViewObjectModifier(effect))
    c->modViewObject().removeModifier(*mod);
  if (msg)
    switch (effect) {
      case LastingEffect::SLOWED:
        c->you(MsgType::ARE, "moving faster again"); break;
      case LastingEffect::SLEEP:
        c->you(MsgType::WAKE_UP, ""); break;
      case LastingEffect::SPEED:
        c->you(MsgType::ARE, "moving more slowly again"); break;
      case LastingEffect::DAM_BONUS:
        c->you(MsgType::ARE, "less dangerous again"); break;
      case LastingEffect::DEF_BONUS:
        c->you(MsgType::ARE, "less protected again"); break;
      case LastingEffect::PANIC:
      case LastingEffect::RAGE:
      case LastingEffect::HALLU:
        c->you(MsgType::YOUR, "mind is clear again"); break;
      case LastingEffect::ENTANGLED:
        c->you(MsgType::BREAK_FREE, "the web"); break;
      case LastingEffect::TIED_UP:
        c->you(MsgType::BREAK_FREE, ""); break;
      case LastingEffect::BLEEDING:
        c->you(MsgType::YOUR, "bleeding stops"); break;
      case LastingEffect::BLIND:
        c->you("can see again"); break;
      case LastingEffect::INVISIBLE:
        c->you(MsgType::TURN_VISIBLE, ""); break;
      case LastingEffect::POISON:
        c->you(MsgType::ARE, "no longer poisoned"); break;
      case LastingEffect::POISON_RESISTANT:
        c->you(MsgType::ARE, "no longer poison resistant"); break;
      case LastingEffect::FIRE_RESISTANT:
        c->you(MsgType::ARE, "no longer fire resistant"); break;
      case LastingEffect::FLYING:
        c->you(MsgType::FALL, "on the " + c->getPosition().getName()); break;
      case LastingEffect::COLLAPSED:
        c->you(MsgType::STAND_UP); break;
      case LastingEffect::INSANITY:
        c->you(MsgType::BECOME, "sane again"); break;
      case LastingEffect::MAGIC_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to magical attacks"); break;
      case LastingEffect::MELEE_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to melee attacks"); break;
      case LastingEffect::RANGED_RESISTANCE:
        c->you(MsgType::FEEL, "less resistant to ranged attacks"); break;
      case LastingEffect::MAGIC_VULNERABILITY:
        c->you(MsgType::FEEL, "less vulnerable to magical attacks"); break;
      case LastingEffect::MELEE_VULNERABILITY:
        c->you(MsgType::FEEL, "less vulnerable to melee attacks"); break;
      case LastingEffect::RANGED_VULNERABILITY:
        c->you(MsgType::FEEL, "less vulnerable to ranged attacks"); break;
      case LastingEffect::ELF_VISION:
        c->you("can't see through trees anymore"); break;
      case LastingEffect::NIGHT_VISION:
        c->you("can't see in the dark anymore"); break;
      default: break;
    }
}

static const int attrBonus = 3;

void LastingEffects::modifyAttr(WConstCreature c, AttrType type, double& value) {
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
          value *= 0.66;
        if (c->isAffected(LastingEffect::DEF_BONUS))
          value += attrBonus;
      break;
    case AttrType::SPEED:
      if (c->isAffected(LastingEffect::SLOWED))
        value /= 1.5;
      if (c->isAffected(LastingEffect::SPEED))
        value *= 1.5;
      break;
    default: break;
  }
}

namespace {
  struct Adjective {
    const char* name;
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
    case LastingEffect::PANIC: return "Panic"_good;
    case LastingEffect::RAGE: return "Enraged"_good;
    case LastingEffect::HALLU: return "Hallucinating"_good;
    case LastingEffect::DAM_BONUS: return "Damage bonus"_good;
    case LastingEffect::DEF_BONUS: return "Defense bonus"_good;
    case LastingEffect::SPEED: return "Speed bonus"_good;
    case LastingEffect::POISON_RESISTANT: return "Poison resistant"_good;
    case LastingEffect::FIRE_RESISTANT: return "Fire resistant"_good;
    case LastingEffect::FLYING: return "Flying"_good;
    case LastingEffect::DARKNESS_SOURCE: return "Source of darkness"_good;
    case LastingEffect::PREGNANT: return "Pregnant"_good;
    case LastingEffect::MAGIC_RESISTANCE: return "Resistant to magical attacks"_good;
    case LastingEffect::MELEE_RESISTANCE: return "Resistant to melee attacks"_good;
    case LastingEffect::RANGED_RESISTANCE: return "Resistant to ranged attacks"_good;
    case LastingEffect::ELF_VISION: return "Can see through trees"_good;
    case LastingEffect::NIGHT_VISION: return "Can see in the dark"_good;

    case LastingEffect::POISON: return "Poisoned"_bad;
    case LastingEffect::BLEEDING: return "Bleeding"_bad;
    case LastingEffect::SLEEP: return "Sleeping"_bad;
    case LastingEffect::ENTANGLED: return "Entangled"_bad;
    case LastingEffect::TIED_UP: return "Tied up"_bad;
    case LastingEffect::SLOWED: return "Slowed"_bad;
    case LastingEffect::INSANITY: return "Insane"_bad;
    case LastingEffect::BLIND: return "Blind"_bad;
    case LastingEffect::STUNNED: return "Stunned"_bad;
    case LastingEffect::COLLAPSED: return "Collapsed"_bad;
    case LastingEffect::MAGIC_VULNERABILITY: return "Vulnerable to magical attacks"_bad;
    case LastingEffect::MELEE_VULNERABILITY: return "Vulnerable to melee attacks"_bad;
    case LastingEffect::RANGED_VULNERABILITY: return "Vulnerable to ranged attacks"_bad;
  }
}

const char* LastingEffects::getGoodAdjective(LastingEffect effect) {
  auto adjective = getAdjective(effect);
  if (!adjective.bad)
    return adjective.name;
  else
    return nullptr;
}

const char* LastingEffects::getBadAdjective(LastingEffect effect) {
  auto adjective = getAdjective(effect);
  if (adjective.bad)
    return adjective.name;
  else
    return nullptr;
}

const vector<LastingEffect>& LastingEffects::getCausingCondition(CreatureCondition condition) {
  switch (condition) {
    case CreatureCondition::RESTRICTED_MOVEMENT: {
      static vector<LastingEffect> ret {LastingEffect::ENTANGLED, LastingEffect::TIED_UP,
          LastingEffect::SLEEP, LastingEffect::STUNNED};
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

void LastingEffects::afterCreatureDamage(WCreature c, LastingEffect e) {
  switch (e) {
    case LastingEffect::SLEEP:
      c->removeEffect(e);
      break;
    default: break;
  }
}

bool LastingEffects::tick(WCreature c, LastingEffect effect) {
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
    default:
      break;
  }
  return false;
}


