#pragma once

#include "util.h"
#include "position.h"

RICH_ENUM(SoundId,
  BEAST_DEATH,
  HUMANOID_DEATH,
  BLUNT_DAMAGE,
  BLADE_DAMAGE,
  BLUNT_NO_DAMAGE,
  BLADE_NO_DAMAGE,
  MISSED_ATTACK,
  DIG_MARK,
  DIG_UNMARK,
  ADD_CONSTRUCTION,
  REMOVE_CONSTRUCTION,
  DIGGING,
  CONSTRUCTING,
  TREE_CUTTING,
  TRAP_ARMING,
  BANG_DOOR,
  SHOOT_BOW,
  WHIP,
  CREATE_IMP,
  DYING_PIG,
  DYING_DONKEY,
  SPELL_HEALING,
  SPELL_SUMMON_INSECTS,
  SPELL_DECEPTION,
  SPELL_SPEED_SELF,
  SPELL_STR_BONUS,
  SPELL_DEX_BONUS,
  SPELL_FIRE_SPHERE_PET,
  SPELL_TELEPORT,
  SPELL_INVISIBILITY,
  SPELL_WORD_OF_POWER,
  SPELL_AIR_BLAST,
  SPELL_SUMMON_SPIRIT,
  SPELL_PORTAL,
  SPELL_CURE_POISON,
  SPELL_METEOR_SHOWER,
  SPELL_MAGIC_SHIELD,
  SPELL_BLAST,
  SPELL_STUN_RAY
);

class Sound {
  public:
  Sound(SoundId);
  Sound& setPosition(const Position&);
  Sound& setPitch(double);
  const optional<Position>& getPosition() const;
  SoundId getId() const;
  double getPitch() const;

  SERIALIZATION_DECL(Sound)

  private:
  SoundId SERIAL(id);
  optional<Position> position;
  double SERIAL(pitch) = 1;
};

