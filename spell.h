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
#include "spell_id.h"

RICH_ENUM(
  CastMessageType,
  STANDARD,
  AIR_BLAST,
  BREATHE_FIRE,
  ABILITY
);

class Effect;
class Position;
class MoveInfo;
class ContentFactory;

class Spell {
  public:
  STRUCT_DECLARATIONS(Spell)
  const string& getSymbol() const;
  const Effect& getEffect() const;
  int getCooldown() const;
  string getDescription(const ContentFactory*) const;
  void addMessage(Creature*) const;
  optional<SoundId> getSound() const;
  bool canTargetSelf() const;
  void apply(Creature*, Position target) const;
  int getRange() const;
  bool isEndOnly() const;
  SpellId getId() const;
  const char* getName() const;
  optional<SpellId> getUpgrade() const;
  MoveInfo getAIMove(const Creature*) const;

  SERIALIZATION_DECL(Spell)

  private:
  PrimaryId<SpellId> SERIAL(id);
  optional<SpellId> SERIAL(upgrade);
  string SERIAL(symbol);
  HeapAllocated<Effect> SERIAL(effect);
  int SERIAL(cooldown);
  CastMessageType SERIAL(castMessageType) = CastMessageType::STANDARD;
  optional<SoundId> SERIAL(sound);
  int SERIAL(range) = 0;
  optional<FXName> SERIAL(fx);
  bool SERIAL(endOnly) = false;
  bool SERIAL(targetSelf) = false;
  bool checkTrajectory(const Creature* caster, Position to) const;
};

