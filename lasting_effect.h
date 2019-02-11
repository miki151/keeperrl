#pragma once

#include "util.h"

RICH_ENUM(LastingEffect,
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
    STUNNED,
    POISON_RESISTANT,
    FIRE_RESISTANT,
    FLYING,
    COLLAPSED,
    INSANITY,
    PEACEFULNESS,
    LIGHT_SOURCE,
    DARKNESS_SOURCE,
    PREGNANT,
    SLEEP_RESISTANT,
    MAGIC_RESISTANCE,
    MELEE_RESISTANCE,
    RANGED_RESISTANCE,
    MAGIC_VULNERABILITY,
    MELEE_VULNERABILITY,
    RANGED_VULNERABILITY,
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
    FAST_CRAFTING,
    FAST_TRAINING,
    SLOW_CRAFTING,
    SLOW_TRAINING,
    ENTERTAINER,
    BAD_BREATH,
    ON_FIRE,
    AMBUSH_SKILL,
    STEALING_SKILL,
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
    NAVIGATION_DIGGING_SKILL,
    DISAPPEAR_DURING_DAY,
    NO_CARRY_LIMIT
);

RICH_ENUM(CreatureCondition,
    SLEEPING,
    RESTRICTED_MOVEMENT
);

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
  static const vector<LastingEffect>& getCausingCondition(CreatureCondition);
  static double modifyCreatureDefense(LastingEffect, double damage, AttrType damageAttr);
  static string getName(LastingEffect);
  static string getDescription(LastingEffect);
  static bool canSee(const Creature*, const Creature*);
  static bool modifyIsEnemyResult(const Creature*, const Creature*, bool);
  static int getPrice(LastingEffect);
  static double getMoraleIncrease(const Creature*);
  static double getCraftingSpeed(const Creature*);
  static double getTrainingSpeed(const Creature*);
  static bool canConsume(LastingEffect);
  static optional<FXVariantName> getFX(LastingEffect);
  static optional<FXInfo> getApplicationFX(LastingEffect);
  static bool canProlong(LastingEffect);
  static bool obeysFormation(const Creature*, const Creature* against);
};
