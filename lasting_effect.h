#pragma once

#include "util.h"

RICH_ENUM(LastingEffect,
    SLEEP,
    PANIC,
    RAGE,
    SLOWED,
    SPEED,
    STR_BONUS,
    DEX_BONUS,
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
    INSANITY,
    MAGIC_SHIELD,
    DARKNESS_SOURCE,
    PREGNANT
);

RICH_ENUM(CreatureCondition,
    SLEEPING,
    RESTRICTED_MOVEMENT
);

class LastingEffects {
  public:
  static void onAffected(WCreature, LastingEffect, bool msg);
  static bool affects(WConstCreature, LastingEffect);
  static void onRemoved(WCreature, LastingEffect, bool msg);
  static void onTimedOut(WCreature, LastingEffect, bool msg);
  static void modifyAttr(WConstCreature, AttrType, int&);
  static void modifyMod(WConstCreature, ModifierType, int&);
  static void onCreatureDamage(WCreature, LastingEffect);
  static const char* getGoodAdjective(LastingEffect);
  static const char* getBadAdjective(LastingEffect);
  static const vector<LastingEffect>& getCausingCondition(CreatureCondition);
};


