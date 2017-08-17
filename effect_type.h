#pragma once

#include "enum_variant.h"
#include "lasting_effect.h"

enum class EffectId { 
  TELEPORT,
  HEAL,
  ROLLING_BOULDER,
  FIRE,
  DESTROY_EQUIPMENT,
  ENHANCE_ARMOR,
  ENHANCE_WEAPON,
  EMIT_POISON_GAS,
  CIRCULAR_BLAST,
  DECEPTION,
  SUMMON,
  SUMMON_ELEMENT,
  ACID,
  ALARM,
  TELE_ENEMIES,
  SILVER_DAMAGE,
  CURE_POISON,
  LASTING,
  PLACE_FURNITURE,
  DAMAGE,
  INJURE_BODY_PART,
  LOOSE_BODY_PART,
  REGROW_BODY_PART,
};

struct DamageInfo {
  AttrType SERIAL(attr);
  AttackType SERIAL(attackType);
  COMPARE_ALL(attr, attackType)
  SERIALIZE_ALL(attr, attackType)
};

class EffectType : public EnumVariant<EffectId, TYPES(LastingEffect, CreatureId, FurnitureType, DamageInfo, BodyPart),
        ASSIGN(LastingEffect, EffectId::LASTING),
        ASSIGN(CreatureId, EffectId::SUMMON),
        ASSIGN(FurnitureType, EffectId::PLACE_FURNITURE),
        ASSIGN(DamageInfo, EffectId::DAMAGE),
        ASSIGN(BodyPart, EffectId::INJURE_BODY_PART, EffectId::LOOSE_BODY_PART)
      > {
  using EnumVariant::EnumVariant;
};

enum class DirEffectId {
  BLAST,
  CREATURE_EFFECT,
};

class DirEffectType : public EnumVariant<DirEffectId, TYPES(EffectType),
        ASSIGN(EffectType, DirEffectId::CREATURE_EFFECT)> {
  public:
  template <typename ...Args>
  DirEffectType(int r, Args&&...args) : EnumVariant(std::forward<Args>(args)...), range(r) {}

  int getRange() const {
    return range;
  }

  private:
  int range;
};

