#pragma once

#include "util.h"
#include "game_time.h"

RICH_ENUM(
  LastingEffect,
  SLEEP,
  PANIC,
  RAGE,
  SLOWED,
  SPEED,
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
  CAPTURE_RESISTANCE,
  MAGIC_CANCELLATION,
  ELF_VISION,
  ARCHER_VISION,
  NIGHT_VISION,
  REGENERATION,
  WARNING,
  TELEPATHY,
  SUNLIGHT_VULNERABLE,
  SATIATED,
  RESTED,
  SUMMONED,
  FAST_CRAFTING,
  FAST_TRAINING,
  SLOW_CRAFTING,
  SLOW_TRAINING,
  ENTERTAINER,
  BAD_BREATH,
  ON_FIRE,
  FROZEN,
  DISAPPEAR_DURING_DAY,
  NO_CARRY_LIMIT,
  SPYING,
  LIFE_SAVED,
  UNSTABLE,
  OIL,
  SWARMER,
  PSYCHIATRY,
  TURNED_OFF,
  DRUNK,
  NO_FRIENDLY_FIRE,
  POLYMORPHED,
  AGGRAVATES,
  CAN_DANCE,
  STEED,
  RIDER
);

struct Color;
class ContentFactory;

class LastingEffects {
  public:
  static void onAffected(Creature*, LastingEffect, bool msg);
  static bool affects(const Creature*, LastingEffect, const ContentFactory*);
  static optional<LastingEffect> getSuppressor(LastingEffect);
  static void onRemoved(Creature*, LastingEffect, bool msg);
  static void onTimedOut(Creature*, LastingEffect, bool msg);
  static int getAttrBonus(const Creature*, AttrType);
  static void afterCreatureDamage(Creature*, LastingEffect);
  static bool tick(Creature*, LastingEffect);
  static optional<string> getGoodAdjective(LastingEffect);
  static optional<string> getBadAdjective(LastingEffect);
  static bool isConsideredBad(LastingEffect);
  static string getAdjective(LastingEffect);
  static double modifyCreatureDefense(const Creature*, LastingEffect, double damage, AttrType damageAttr);
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
  static bool shouldEnemyApply(const Creature* victim, LastingEffect);
  static bool shouldAllyApplyInDanger(const Creature* victim, LastingEffect);
  static TimeInterval getDuration(const Creature* c, LastingEffect);
  static void runTests();
  static Color getColor(LastingEffect);
  static bool losesControl(const Creature*);
  static bool doesntMove(const Creature*);
  static bool restrictedMovement(const Creature*);
  static bool canSwapPosition(const Creature*);
  static bool inheritsFromSteed(LastingEffect);
};
