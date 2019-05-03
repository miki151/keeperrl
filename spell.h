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
class Position;

class Spell {
  public:
  const string& getSymbol() const;
  const Effect& getEffect() const;
  int getCooldown() const;
  string getDescription() const;
  void addMessage(Creature*) const;
  optional<SoundId> getSound() const;
  bool canTargetSelf() const;
  void apply(Creature*, Position target) const;
  int getRange() const;
  const string& getId() const;
  const optional<string>& getUpgrade() const;

  SERIALIZATION_DECL(Spell)

  private:
  string SERIAL(id);
  optional<string> SERIAL(upgrade);
  string SERIAL(symbol);
  HeapAllocated<Effect> SERIAL(effect);
  int SERIAL(cooldown);
  CastMessageType SERIAL(castMessageType) = CastMessageType::STANDARD;
  optional<SoundId> SERIAL(sound);
  int SERIAL(range) = 0;
  optional<FXName> SERIAL(fx);
  bool SERIAL(endOnly) = false;
  bool SERIAL(targetSelf) = false;
};

