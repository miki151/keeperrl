#include "stdafx.h"
#include "lasting_effect.h"
#include "creature.h"
#include "view_object.h"
#include "player_message.h"
#include "creature_attributes.h"
#include "body.h"
#include "furniture.h"
#include "level.h"
#include "fx_name.h"

static optional<LastingEffect> getCancelledOneWay(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::POISON_RESISTANT:
      return LastingEffect::POISON;
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

void LastingEffects::onAffected(WCreature c, LastingEffect effect, bool msg) {
  PROFILE;
  if (auto e = getCancelled(effect))
    c->removeEffect(*e, true);
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
        c->you(MsgType::ARE, "knocked out"); break;
      case LastingEffect::PANIC:
        c->you(MsgType::PANIC, ""); break;
      case LastingEffect::RAGE:
        c->you(MsgType::RAGE, ""); break;
      case LastingEffect::HALLU:
        if (!c->isAffected(LastingEffect::BLIND))
          c->privateMessage("The world explodes into colors!");
        else
          c->privateMessage("You feel as if a party has started without you.");
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
      case LastingEffect::SLEEP_RESISTANT:
        c->you(MsgType::ARE, "now sleep resistant"); break;
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
      case LastingEffect::PEACEFULNESS:
        c->you(MsgType::BECOME, "peaceful"); break;
      case LastingEffect::LIGHT_SOURCE:
        c->getPosition().addCreatureLight(false);
        break;
      case LastingEffect::DARKNESS_SOURCE:
        c->getPosition().addCreatureLight(true);
        break;
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
      case LastingEffect::REGENERATION:
        c->you(MsgType::ARE, "regenerating"); break;
      case LastingEffect::WARNING:
        c->you(MsgType::FEEL, "more aware of danger"); break;
      case LastingEffect::TELEPATHY:
        c->you(MsgType::ARE, "telepathic"); break;
      case LastingEffect::SUNLIGHT_VULNERABLE:
        c->you(MsgType::ARE, "vulnerable to sunlight"); break;
      case LastingEffect::SATIATED:
        c->you(MsgType::ARE, "satiated"); break;
      case LastingEffect::RESTED:
        c->you(MsgType::ARE, "well rested"); break;
      case LastingEffect::SUMMONED:
        c->you(MsgType::YOUR, "days are numbered"); break;
    }
}

bool LastingEffects::affects(WConstCreature c, LastingEffect effect) {
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
    case LastingEffect::SLEEP:
      c->you(MsgType::WAKE_UP, "");
      break;
    case LastingEffect::STUNNED:
      c->you(MsgType::ARE, "no longer unconscious");
      break;
    default:
      onTimedOut(c, effect, msg); break;
  }
}

void LastingEffects::onTimedOut(WCreature c, LastingEffect effect, bool msg) {
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
      default: break;
    }
}

static const int attrBonus = 3;

int LastingEffects::getAttrBonus(WConstCreature c, AttrType type) {
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
  }
}

const char* LastingEffects::getGoodAdjective(LastingEffect effect) {
  if (auto adjective = getAdjective(effect))
    if (!adjective->bad)
      return adjective->name;
  return nullptr;
}

const char* LastingEffects::getBadAdjective(LastingEffect effect) {
  if (auto adjective = getAdjective(effect))
    if (adjective->bad)
      return adjective->name;
  return nullptr;
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
    case LastingEffect::REGENERATION:
      c->getBody().heal(c, 0.03);
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
            if (v.dist8(position) <= 1)
              isBigDanger = true;
            else
              isDanger = true;
          }
        if (WCreature enemy = v.getCreature()) {
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
    default:
      break;
  }
  return false;
}

const char* LastingEffects::getName(LastingEffect type) {
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
  }
}

const char* LastingEffects::getDescription(LastingEffect type) {
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
  }
}

bool LastingEffects::canSee(WConstCreature c1, WConstCreature c2) {
  PROFILE;
  return c1->getPosition().dist8(c2->getPosition()) < 5 && c2->getBody().hasBrain() &&
      c1->isAffected(LastingEffect::TELEPATHY);
}

bool LastingEffects::modifyIsEnemyResult(WConstCreature c, WConstCreature other, bool result) {
  PROFILE;
  if (c->isAffected(LastingEffect::INSANITY))
    return true;
  if (c->isAffected(LastingEffect::PEACEFULNESS))
    return false;
  return result;
}

int LastingEffects::getPrice(LastingEffect e) {
  switch (e) {
    case LastingEffect::INSANITY:
    case LastingEffect::PEACEFULNESS:
    case LastingEffect::HALLU:
    case LastingEffect::BLEEDING:
    case LastingEffect::SUNLIGHT_VULNERABLE:
    case LastingEffect::SATIATED:
    case LastingEffect::RESTED:
    case LastingEffect::SUMMONED:
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
      return 24;
  }
}

double LastingEffects::getMoraleIncrease(WConstCreature c) {
  PROFILE;
  double ret = 0;
  if (c->isAffected(LastingEffect::RESTED))
    ret += 0.2;
  if (c->isAffected(LastingEffect::SATIATED))
    ret += 0.2;
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
      return false;
    default:
      return true;
  }
}

optional<FXName> LastingEffects::getFXName(LastingEffect effect) {
  switch (effect) {
    case LastingEffect::PEACEFULNESS:
      return FXName::PEACEFULNESS;
    case LastingEffect::SLEEP:
      return FXName::SLEEP;
    case LastingEffect::BLIND:
      return FXName::BLIND;
    case LastingEffect::INSANITY:
      return FXName::INSANITY;
    case LastingEffect::SPEED:
      return FXName::SPEED;
    case LastingEffect::SLOWED:
      return FXName::SLOW;
    case LastingEffect::FLYING:
      return FXName::FLYING;
    default:
      return none;
  }
}
