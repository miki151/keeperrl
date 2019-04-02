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

#pragma once

#include "enums.h"
#include "util.h"

RICH_ENUM(
  CastMessageType,
  STANDARD,
  AIR_BLAST,
  BREATHE_FIRE
);

class Effect;
class DirEffectType;

class Spell {
  public:
  const string& getSymbol() const;
  bool isDirected() const;
  bool hasEffect(const Effect&) const;
  bool hasEffect(const DirEffectType&) const;
  const Effect& getEffect() const;
  const DirEffectType& getDirEffectType() const;
  MAKE_VARIANT(SpellVariant, Effect, DirEffectType);
  const SpellVariant& getVariant() const;
  int getCooldown() const;
  string getDescription() const;
  void addMessage(Creature*) const;
  optional<SoundId> getSound() const;

  SERIALIZATION_DECL(Spell)

  private:
  string SERIAL(symbol);
  HeapAllocated<SpellVariant> SERIAL(effect);
  int SERIAL(cooldown);
  CastMessageType SERIAL(castMessageType) = CastMessageType::STANDARD;
  optional<SoundId> SERIAL(sound);
};

