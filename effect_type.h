#ifndef _EFFECT_TYPE_H
#define _EFFECT_TYPE_H

#include "enum_variant.h"

enum class EffectId { 
    TELEPORT,
    HEAL,
    IDENTIFY,
    ROLLING_BOULDER,
    FIRE,
    PORTAL,
    DESTROY_EQUIPMENT,
    ENHANCE_ARMOR,
    ENHANCE_WEAPON,
    FIRE_SPHERE_PET,
    GUARDING_BOULDER,
    EMIT_POISON_GAS,
    WORD_OF_POWER,
    DECEPTION,
    SUMMON_INSECTS,
    ACID,
    ALARM,
    TELE_ENEMIES,
    SUMMON_SPIRIT,
    LEAVE_BODY,
    SILVER_DAMAGE,
    CURE_POISON,
    METEOR_SHOWER,
    LASTING,
};

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
    STUNNED,
    POISON_RESISTANT,
    FIRE_RESISTANT,
    FLYING,
    INSANITY,
    MAGIC_SHIELD
);


typedef EnumVariant<EffectId, TYPES(LastingEffect),
        ASSIGN(LastingEffect, EffectId::LASTING)> EffectType;

enum class DirEffectId {
  BLAST,
  CREATURE_EFFECT,
};

typedef EnumVariant<DirEffectId, TYPES(EffectType),
        ASSIGN(EffectType, DirEffectId::CREATURE_EFFECT)> DirEffectType;

#endif
