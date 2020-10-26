#pragma once

#include "util.h"
#include "effect_ai_intent.h"
#include "game_time.h"

RICH_ENUM(
  LastingEffect,
  SLEEP,
  PANIC,
  RAGE,
  SLOWED,
  SPEED,
  DAM_BONUS,
  DEF_BONUS,
  BLEEDING,
  HALLU,
  BLIND,
  INVISIBLE,
  POISON,
  ENTANGLED,
  TIED_UP,
  IMMOBILE,
  STUNNED,
  POISON_RESISTANT,
  FIRE_RESISTANT,
  COLD_RESISTANT,
  ACID_RESISTANT,
  FLYING,
  COLLAPSED,
  INSANITY,
  PEACEFULNESS,
  LIGHT_SOURCE,
  DARKNESS_SOURCE,
  PREGNANT,
  PLAGUE,
  PLAGUE_RESISTANT,
  SLEEP_RESISTANT,
  MAGIC_RESISTANCE,
  MELEE_RESISTANCE,
  RANGED_RESISTANCE,
  MAGIC_VULNERABILITY,
  MELEE_VULNERABILITY,
  RANGED_VULNERABILITY,
  MAGIC_CANCELLATION,
  SPELL_DAMAGE,
  ELF_VISION,
  NIGHT_VISION,
  REGENERATION,
  WARNING,
  TELEPATHY,
  SUNLIGHT_VULNERABLE,
  SATIATED,
  RESTED,
  SUMMONED,
  HATE_DWARVES,
  HATE_UNDEAD,
  HATE_HUMANS,
  HATE_GREENSKINS,
  HATE_ELVES,
  HATE_DRAGONS,
  FAST_CRAFTING,
  FAST_TRAINING,
  SLOW_CRAFTING,
  SLOW_TRAINING,
  ENTERTAINER,
  BAD_BREATH,
  ON_FIRE,
  FROZEN,
  AMBUSH_SKILL,
  SWIMMING_SKILL,
  DISARM_TRAPS_SKILL,
  CONSUMPTION_SKILL,
  COPULATION_SKILL,
  CROPS_SKILL,
  SPIDER_SKILL,
  EXPLORE_SKILL,
  EXPLORE_NOCTURNAL_SKILL,
  EXPLORE_CAVES_SKILL,
  BRIDGE_BUILDING_SKILL,
  DISTILLATION_SKILL,
  NAVIGATION_DIGGING_SKILL,
  DISAPPEAR_DURING_DAY,
  NO_CARRY_LIMIT,
  SPYING,
  LIFE_SAVED,
  UNSTABLE,
  OIL,
  SWARMER,
  PSYCHIATRY,
  INVULNERABLE,
  TURNED_OFF,
  DRUNK,
  NO_FRIENDLY_FIRE,
  POLYMORPHED
);

struct Color;

class LastingEffects {
  public:
  static void onAffected(Creature*, LastingEffect, bool msg);
  static bool affects(const Creature*, LastingEffect);
  static optional<LastingEffect> getSuppressor(LastingEffect);
  static void onRemoved(Creature*, LastingEffect, bool msg);
  static void onTimedOut(Creature*, LastingEffect, bool msg);
  static int getAttrBonus(const Creature*, AttrType);
  static void afterCreatureDamage(Creature*, LastingEffect);
  static bool tick(Creature*, LastingEffect);
  static optional<string> getGoodAdjective(LastingEffect);
  static optional<string> getBadAdjective(LastingEffect);
  static double modifyCreatureDefense(LastingEffect, double damage, AttrType damageAttr);
  static void onAllyKilled(Creature*);
  static string getName(LastingEffect);
  static string getDescription(LastingEffect);
  static bool canSee(const Creature*, const Creature*, GlobalTime);
  static bool modifyIsEnemyResult(const Creature*, const Creature*, GlobalTime, bool);
  static int getPrice(LastingEffect);
  static double getMoraleIncrease(const Creature*, optional<GlobalTime>);
  static double getCraftingSpeed(const Creature*);
  static double getTrainingSpeed(const Creature*);
  static bool canConsume(LastingEffect);
  static bool canWishFor(LastingEffect);
  static optional<FXVariantName> getFX(LastingEffect);
  static optional<FXInfo> getApplicationFX(LastingEffect);
  static bool canProlong(LastingEffect);
  static bool obeysFormation(const Creature*, const Creature* against);
  static EffectAIIntent shouldAIApply(const Creature* victim, LastingEffect, bool isEnemy);
  static AttrType modifyMeleeDamageAttr(const Creature* attacker, AttrType);
  static TimeInterval getDuration(const Creature* c, LastingEffect);
  static void runTests();
  static const char* getHatedGroupName(LastingEffect);
  static Color getColor(LastingEffect);
  static bool losesControl(const Creature*);
  static bool doesntMove(const Creature*);
  static bool restrictedMovement(const Creature*);
  static bool canSwapPosition(const Creature*);
};
