#pragma once

#include "enum_variant.h"
#include "lasting_effect.h"

enum class EffectId { 
    TELEPORT,
    HEAL,
    ROLLING_BOULDER,
    FIRE,
    PORTAL,
    DESTROY_EQUIPMENT,
    ENHANCE_ARMOR,
    ENHANCE_WEAPON,
    EMIT_POISON_GAS,
    WORD_OF_POWER,
    AIR_BLAST,
    DECEPTION,
    SUMMON,
    ACID,
    ALARM,
    TELE_ENEMIES,
    LEAVE_BODY,
    SILVER_DAMAGE,
    CURE_POISON,
    METEOR_SHOWER,
    LASTING,
    PLACE_FURNITURE,
};

class EffectType : public EnumVariant<EffectId, TYPES(LastingEffect, CreatureId, FurnitureType),
        ASSIGN(LastingEffect, EffectId::LASTING),
        ASSIGN(CreatureId, EffectId::SUMMON),
        ASSIGN(FurnitureType, EffectId::PLACE_FURNITURE)
      > {
  using EnumVariant::EnumVariant;
};

enum class DirEffectId {
  BLAST,
  CREATURE_EFFECT,
};

class DirEffectType : public EnumVariant<DirEffectId, TYPES(EffectType),
        ASSIGN(EffectType, DirEffectId::CREATURE_EFFECT)> {
  using EnumVariant::EnumVariant;
};

