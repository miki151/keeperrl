/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _SPELL_H
#define _SPELL_H

#include "enums.h"
#include "util.h"
#include "singleton.h"
#include "effect_type.h"

RICH_ENUM(SpellId,
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
  AIR_BLAST,
  SUMMON_SPIRIT,
  PORTAL,
  CURE_POISON,
  METEOR_SHOWER,
  MAGIC_SHIELD,
  BLAST,
  STUN_RAY
);

enum class CastMessageType {
  STANDARD,
  AIR_BLAST
};

class Spell : public Singleton<Spell, SpellId> {
  public:
  const string& getName() const;
  bool isDirected() const;
  bool hasEffect(EffectType) const;
  bool hasEffect(DirEffectType) const;
  EffectType getEffectType() const;
  DirEffectType getDirEffectType() const;
  int getDifficulty() const;
  string getDescription() const;
  void addMessage(Creature*);

  static void init();

  private:
  Spell(const string&, EffectType, int difficulty, CastMessageType = CastMessageType::STANDARD);
  Spell(const string&, DirEffectType, int difficulty, CastMessageType = CastMessageType::STANDARD);

  const string name;
  const variant<EffectType, DirEffectType> effect;
  const int difficulty;
  const CastMessageType castMessageType;
};

#endif
