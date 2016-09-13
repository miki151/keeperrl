#include "stdafx.h"
#include "lasting_effect.h"
#include "creature.h"
#include "view_object.h"
#include "player_message.h"
#include "creature_attributes.h"
#include "body.h"

void LastingEffects::onAffected(Creature* c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::FLYING:
      if (msg) c->you(MsgType::ARE, "flying!");
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
      if (!c->isBlind() && msg)
        c->playerMessage("The world explodes into colors!");
      break;
    case LastingEffect::BLIND:
      if (msg) c->you(MsgType::ARE, "blind!");
      c->modViewObject().setModifier(ViewObject::Modifier::BLIND);
      break;
    case LastingEffect::INVISIBLE:
      if (!c->isBlind() && msg)
        c->you(MsgType::TURN_INVISIBLE, "");
      c->modViewObject().setModifier(ViewObject::Modifier::INVISIBLE);
      break;
    case LastingEffect::POISON:
      if (msg) c->you(MsgType::ARE, "poisoned");
      c->modViewObject().setModifier(ViewObject::Modifier::POISONED);
      break;
    case LastingEffect::STR_BONUS: if (msg) c->you(MsgType::FEEL, "stronger"); break;
    case LastingEffect::DEX_BONUS: if (msg) c->you(MsgType::FEEL, "more agile"); break;
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

bool LastingEffects::affects(const Creature* c, LastingEffect effect) {
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

void LastingEffects::onRemoved(Creature* c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, "cured from poisoning");
      c->modViewObject().removeModifier(ViewObject::Modifier::POISONED);
      break;
    default: onTimedOut(c, effect, msg); break;
  }
}

void LastingEffects::onTimedOut(Creature* c, LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::STUNNED: break;
    case LastingEffect::SLOWED: if (msg) c->you(MsgType::ARE, "moving faster again"); break;
    case LastingEffect::SLEEP: if (msg) c->you(MsgType::WAKE_UP, ""); break;
    case LastingEffect::SPEED: if (msg) c->you(MsgType::ARE, "moving more slowly again"); break;
    case LastingEffect::STR_BONUS: if (msg) c->you(MsgType::ARE, "weaker again"); break;
    case LastingEffect::DEX_BONUS: if (msg) c->you(MsgType::ARE, "less agile again"); break;
    case LastingEffect::PANIC:
    case LastingEffect::RAGE:
    case LastingEffect::HALLU: if (msg) c->playerMessage("Your mind is clear again"); break;
    case LastingEffect::ENTANGLED: if (msg) c->you(MsgType::BREAK_FREE, "the web"); break;
    case LastingEffect::TIED_UP: if (msg) c->you(MsgType::BREAK_FREE, ""); break;
    case LastingEffect::BLIND:
      if (msg) 
        c->you("can see again");
      c->modViewObject().removeModifier(ViewObject::Modifier::BLIND);
      break;
    case LastingEffect::INVISIBLE:
      if (msg)
        c->you(MsgType::TURN_VISIBLE, "");
      c->modViewObject().removeModifier(ViewObject::Modifier::INVISIBLE);
      break;
    case LastingEffect::POISON:
      if (msg)
        c->you(MsgType::ARE, "no longer poisoned");
      c->modViewObject().removeModifier(ViewObject::Modifier::POISONED);
      break;
    case LastingEffect::POISON_RESISTANT: if (msg) c->you(MsgType::ARE, "no longer poison resistant"); break;
    case LastingEffect::FIRE_RESISTANT: if (msg) c->you(MsgType::ARE, "no longer fire resistant"); break;
    case LastingEffect::FLYING:
      if (msg)
        c->you(MsgType::FALL, "on the " + c->getPosition().getName());
      break;
    case LastingEffect::INSANITY: if (msg) c->you(MsgType::BECOME, "sane again"); break;
    case LastingEffect::MAGIC_SHIELD: if (msg) c->you(MsgType::FEEL, "less protected"); break;
    case LastingEffect::PREGNANT: break;
    case LastingEffect::DARKNESS_SOURCE: break;
  } 
}

int attrBonus = 3;

void LastingEffects::modifyAttr(const Creature* c, AttrType attr, int& value) {
  switch (attr) {
    case AttrType::STRENGTH:
      if (c->isAffected(LastingEffect::STR_BONUS))
        value += attrBonus;
      break;
    case AttrType::DEXTERITY:
      if (c->isAffected(LastingEffect::DEX_BONUS))
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

void LastingEffects::modifyMod(const Creature* c, ModifierType type, int& value) {
  switch (type) {
    case ModifierType::FIRED_DAMAGE: 
    case ModifierType::THROWN_DAMAGE: 
        if (c->isAffected(LastingEffect::PANIC))
          value -= attrBonus;
        if (c->isAffected(LastingEffect::RAGE))
          value += attrBonus;
        break;
    case ModifierType::DAMAGE: 
        if (c->isAffected(LastingEffect::PANIC))
          value -= attrBonus;
        if (c->isAffected(LastingEffect::RAGE))
          value += attrBonus;
        break;
    case ModifierType::DEFENSE: 
        if (c->isAffected(LastingEffect::PANIC))
          value += attrBonus;
        if (c->isAffected(LastingEffect::RAGE))
          value -= attrBonus;
        if (c->isAffected(LastingEffect::SLEEP))
          value *= 0.66;
        if (c->isAffected(LastingEffect::MAGIC_SHIELD))
          value += 20;
        break;
    case ModifierType::ACCURACY: 
        if (c->isAffected(LastingEffect::SLEEP))
          value = 0;
        break;
    default: break;
  }
}

const char* LastingEffects::getGoodAdjective(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::INVISIBLE: return "Invisible";
    case LastingEffect::PANIC: return "Panic";
    case LastingEffect::RAGE: return "Enraged";
    case LastingEffect::HALLU: return "Hallucinating";
    case LastingEffect::STR_BONUS: return "Strength bonus";
    case LastingEffect::DEX_BONUS: return "Dexterity bonus";
    case LastingEffect::SPEED: return "Speed bonus";
    case LastingEffect::POISON_RESISTANT: return "Poison resistant";
    case LastingEffect::FIRE_RESISTANT: return "Fire resistant";
    case LastingEffect::FLYING: return "Flying";
    case LastingEffect::MAGIC_SHIELD: return "Magic shield";
    case LastingEffect::DARKNESS_SOURCE: return "Source of darkness";
    case LastingEffect::PREGNANT: return "Pregnant";
    default: return nullptr;
  }
}

const char* LastingEffects::getBadAdjective(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::POISON: return "Poisoned";
    case LastingEffect::SLEEP: return "Sleeping";
    case LastingEffect::ENTANGLED: return "Entangled";
    case LastingEffect::TIED_UP: return "Tied up";
    case LastingEffect::SLOWED: return "Slowed";
    case LastingEffect::INSANITY: return "Insane";
    default: return nullptr;
  }
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

void LastingEffects::onCreatureDamage(Creature* c, LastingEffect e) {
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


