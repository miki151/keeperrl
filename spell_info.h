#ifndef _SPELL_INFO_H
#define _SPELL_INFO_H

#include "enums.h"
#include "util.h"

enum class SpellId {
  HEALING,
  SUMMON_INSECTS,
  DECEPTION,
  SPEED_SELF,
  STR_BONUS,
  DEX_BONUS,
  FIRE_SPHERE_PET,
  TELEPORT,
  INVISIBILITY,
  WORD_OF_POWER,
  SUMMON_SPIRIT,
  PORTAL,
  CURE_POISON,
  METEOR_SHOWER,
  MAGIC_SHIELD
};

struct SpellInfo {
  SpellId id;
  string name;
  EffectType type;
  double ready;
  int difficulty;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);
};



#endif
