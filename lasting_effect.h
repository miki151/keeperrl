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
    MAGICAL_DISARMING_SKILL,
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
    SUMMONED
);

RICH_ENUM(CreatureCondition,
    SLEEPING,
    RESTRICTED_MOVEMENT
);

class LastingEffects {
  public:
  static void onAffected(WCreature, LastingEffect, bool msg);
  static bool affects(WConstCreature, LastingEffect);
  static optional<LastingEffect> getSuppressor(LastingEffect);
  static void onRemoved(WCreature, LastingEffect, bool msg);
  static void onTimedOut(WCreature, LastingEffect, bool msg);
  static void modifyAttr(WConstCreature, AttrType, double&);
  static void afterCreatureDamage(WCreature, LastingEffect);
  static bool tick(WCreature, LastingEffect);
  static const char* getGoodAdjective(LastingEffect);
  static const char* getBadAdjective(LastingEffect);
  static const vector<LastingEffect>& getCausingCondition(CreatureCondition);
  static double modifyCreatureDefense(LastingEffect, double damage, AttrType damageAttr);
  static const char* getName(LastingEffect);
  static const char* getDescription(LastingEffect);
  static bool canSee(WConstCreature, WConstCreature);
  static bool modifyIsEnemyResult(WConstCreature, WConstCreature, bool);
  static int getPrice(LastingEffect);
  static double getMoraleIncrease(WConstCreature);
};


