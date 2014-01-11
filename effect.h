#ifndef _EFFECT_H
#define _EFFECT_H

#include "util.h"
#include "enums.h"

class Level;
class Creature;
class Item;

enum class EffectStrength { WEAK, NORMAL, STRONG };
enum class EffectType { 
    TELEPORT,
    HEAL,
    SLEEP,
    IDENTIFY,
    PANIC,
    RAGE,
    ROLLING_BOULDER,
    FIRE,
    SLOW,
    SPEED,
    HALLU,
    STR_BONUS,
    DEX_BONUS,
    BLINDNESS,
    INVISIBLE,
    PORTAL,
    GIVE_ITEM,
    DESTROY_EQUIPMENT,
    ENHANCE_ARMOR,
    ENHANCE_WEAPON,
    FIRE_SPHERE_PET,
    GUARDING_BOULDER,
    EMIT_POISON_GAS,
    POISON,
};

class Effect {
  public:
  virtual ~Effect() {}
  static PEffect getEffect(EffectType type);
  static PEffect giveItemEffect(ItemId, int num = 1);
  virtual void applyToCreature(Creature*, EffectStrength) {}
  EffectType getType();

  protected:
  Effect(EffectType);

  private:
  EffectType type;
};

class PoisonEffect : public Effect {
  public:
  PoisonEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class EmitPoisonGasEffect : public Effect {
  public:
  EmitPoisonGasEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class GuardingBuilderEffect : public Effect {
  public:
  GuardingBuilderEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class FireSpherePetEffect : public Effect {
  public:
  FireSpherePetEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class EnhanceArmorEffect : public Effect {
  public:
  EnhanceArmorEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class EnhanceWeaponEffect : public Effect {
  public:
  EnhanceWeaponEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class DestroyEquipmemntEffect : public Effect {
  public:
  DestroyEquipmemntEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class GiveItemEffect : public Effect {
  public:
  GiveItemEffect(ItemId, int num);
  virtual void applyToCreature(Creature*, EffectStrength);

  private:
  ItemId id;
  int num;
};

class TeleportEffect : public Effect {
  public:
  TeleportEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class PortalEffect : public Effect {
  public:
  PortalEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class HealingEffect : public Effect {
  public:
  HealingEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class SleepEffect : public Effect {
  public:
  SleepEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class PanicEffect : public Effect {
  public:
  PanicEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class RageEffect : public Effect {
  public:
  RageEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class HalluEffect : public Effect {
  public:
  HalluEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class StrBonusEffect : public Effect {
  public:
  StrBonusEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class DexBonusEffect : public Effect {
  public:
  DexBonusEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class IdentifyEffect : public Effect {
  public:
  IdentifyEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class RollingBoulderEffect : public Effect {
  public:
  RollingBoulderEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class FireEffect : public Effect {
  public:
  FireEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class SlowEffect : public Effect {
  public:
  SlowEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class SpeedEffect : public Effect {
  public:
  SpeedEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class BlindnessEffect : public Effect {
  public:
  BlindnessEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

class InvisibleEffect : public Effect {
  public:
  InvisibleEffect();
  virtual void applyToCreature(Creature*, EffectStrength);
};

#endif
