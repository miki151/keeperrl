#include "stdafx.h"
#include "lasting_effect.h"
#include "creature.h"
#include "view_object.h"
#include "player_message.h"
#include "creature_attributes.h"
#include "body.h"

void LastingEffects::onAffected(WCreature c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::FLYING:
      if (msg) c->you(MsgType::ARE, "flying!");
      break;
    case LastingEffect::COLLAPSED:
      if (msg) c->you(MsgType::COLLAPSE);
      break;
    case LastingEffect::PREGNANT:
      if (msg) c->you(MsgType::ARE, "pregnant!");
      break;
    case LastingEffect::STUNNED:
      if (msg) c->you(MsgType::ARE, "stunned");
      break;
    case LastingEffect::PANIC:
      c->removeEffect(LastingEffect::RAGE, false);
      if (msg) c->you(MsgType::PANIC, "");
      break;
    case LastingEffect::RAGE:
      c->removeEffect(LastingEffect::PANIC, false);
      if (msg) c->you(MsgType::RAGE, "");
      break;
    case LastingEffect::HALLU:
      if (!c->isAffected(LastingEffect::BLIND) && msg)
        c->playerMessage("The world explodes into colors!");
      break;
    case LastingEffect::BLIND:
      if (msg) c->you(MsgType::ARE, "blind!");
      break;
    case LastingEffect::INVISIBLE:
      if (!c->isAffected(LastingEffect::BLIND) && msg)
        c->you(MsgType::TURN_INVISIBLE, "");
      c->modViewObject().setModifier(ViewObject::Modifier::INVISIBLE);
      break;
    case LastingEffect::POISON:
      if (msg) c->you(MsgType::ARE, "poisoned");
      break;
    case LastingEffect::DAM_BONUS: if (msg) c->you(MsgType::FEEL, "more dangerous"); break;
    case LastingEffect::DEF_BONUS: if (msg) c->you(MsgType::FEEL, "more protected"); break;
    case LastingEffect::SPEED:
      if (msg) c->you(MsgType::ARE, "moving faster");
      c->removeEffect(LastingEffect::SLOWED, false);
      break;
    case LastingEffect::SLOWED:
      if (msg) c->you(MsgType::ARE, "moving more slowly");
      c->removeEffect(LastingEffect::SPEED, false);
      break;
    case LastingEffect::TIED_UP: if (msg) c->you(MsgType::ARE, "tied up"); break;
    case LastingEffect::ENTANGLED: if (msg) c->you(MsgType::ARE, "entangled in a web"); break;
    case LastingEffect::SLEEP: if (msg) c->you(MsgType::FALL_ASLEEP, ""); break;
    case LastingEffect::POISON_RESISTANT:
      if (msg) c->you(MsgType::ARE, "now poison resistant");
      c->removeEffect(LastingEffect::POISON, true);
      break;
    case LastingEffect::FIRE_RESISTANT: if (msg) c->you(MsgType::ARE, "now fire resistant"); break;
    case LastingEffect::INSANITY: if (msg) c->you(MsgType::BECOME, "insane"); break;
    case LastingEffect::MAGIC_SHIELD: if (msg) c->you(MsgType::FEEL, "protected"); break;
    case LastingEffect::DARKNESS_SOURCE: break;
  }
}

bool LastingEffects::affects(WConstCreature c, LastingEffect effect) {
  switch (effect) {
    case LastingEffect::RAGE:
    case LastingEffect::PANIC:
      return !c->isAffected(LastingEffect::SLEEP);
    case LastingEffect::POISON:
      return !c->isAffected(LastingEffect::POISON_RESISTANT);
    case LastingEffect::TIED_UP:
    case LastingEffect::ENTANGLED:
      return c->getBody().canEntangle();
    default: return true;
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
    default: onTimedOut(c, effect, msg); break;
  }
}

void LastingEffects::onTimedOut(WCreature c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::STUNNED: break;
    case LastingEffect::SLOWED: if (msg) c->you(MsgType::ARE, "moving faster again"); break;
    case LastingEffect::SLEEP: if (msg) c->you(MsgType::WAKE_UP, ""); break;
    case LastingEffect::SPEED: if (msg) c->you(MsgType::ARE, "moving more slowly again"); break;
    case LastingEffect::DAM_BONUS: if (msg) c->you(MsgType::ARE, "less dangerous again"); break;
    case LastingEffect::DEF_BONUS: if (msg) c->you(MsgType::ARE, "less protected again"); break;
    case LastingEffect::PANIC:
    case LastingEffect::RAGE:
    case LastingEffect::HALLU: if (msg) c->playerMessage("Your mind is clear again"); break;
    case LastingEffect::ENTANGLED: if (msg) c->you(MsgType::BREAK_FREE, "the web"); break;
    case LastingEffect::TIED_UP: if (msg) c->you(MsgType::BREAK_FREE, ""); break;
    case LastingEffect::BLIND:
      if (msg)
        c->you("can see again");
      break;
    case LastingEffect::INVISIBLE:
      if (msg)
        c->you(MsgType::TURN_VISIBLE, "");
      c->modViewObject().removeModifier(ViewObject::Modifier::INVISIBLE);
      break;
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, "no longer poisoned");
      break;
    case LastingEffect::POISON_RESISTANT: if (msg) c->you(MsgType::ARE, "no longer poison resistant"); break;
    case LastingEffect::FIRE_RESISTANT: if (msg) c->you(MsgType::ARE, "no longer fire resistant"); break;
    case LastingEffect::FLYING:
      if (msg)
        c->you(MsgType::FALL, "on the " + c->getPosition().getName());
      break;
    case LastingEffect::COLLAPSED:
      c->you(MsgType::STAND_UP);
      break;
    case LastingEffect::INSANITY: if (msg) c->you(MsgType::BECOME, "sane again"); break;
    case LastingEffect::MAGIC_SHIELD: if (msg) c->you(MsgType::FEEL, "less protected"); break;
    case LastingEffect::PREGNANT: break;
    case LastingEffect::DARKNESS_SOURCE: break;
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
        if (c->isAffected(LastingEffect::MAGIC_SHIELD))
          value += 20;
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
    case LastingEffect::MAGIC_SHIELD: return "Magic shield"_good;
    case LastingEffect::DARKNESS_SOURCE: return "Source of darkness"_good;
    case LastingEffect::PREGNANT: return "Pregnant"_good;

    case LastingEffect::POISON: return "Poisoned"_bad;
    case LastingEffect::SLEEP: return "Sleeping"_bad;
    case LastingEffect::ENTANGLED: return "Entangled"_bad;
    case LastingEffect::TIED_UP: return "Tied up"_bad;
    case LastingEffect::SLOWED: return "Slowed"_bad;
    case LastingEffect::INSANITY: return "Insane"_bad;
    case LastingEffect::BLIND: return "Blind"_bad;
    case LastingEffect::STUNNED: return "Stunned"_bad;
    case LastingEffect::COLLAPSED: return "Collapsed"_bad;
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

void LastingEffects::onCreatureDamage(WCreature c, LastingEffect e) {
  switch (e) {
    case LastingEffect::SLEEP:
      c->removeEffect(e);
      break;
    case LastingEffect::MAGIC_SHIELD:
      c->getAttributes().shortenEffect(LastingEffect::MAGIC_SHIELD, 5);
      c->globalMessage("The magic shield absorbs the attack", "");
      break;
    default: break;
  }
}


