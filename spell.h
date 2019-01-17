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
#include "singleton.h"
#include "spell_id.h"

enum class CastMessageType {
  STANDARD,
  AIR_BLAST,
  BREATHE_FIRE
};

class Effect;
class DirEffectType;

class Spell : public Singleton<Spell, SpellId> {
  public:
  const string& getName() const;
  bool isDirected() const;
  bool hasEffect(Effect) const;
  bool hasEffect(DirEffectType) const;
  Effect getEffect() const;
  DirEffectType getDirEffectType() const;
  int getDifficulty() const;
  string getDescription() const;
  void addMessage(WCreature);
  SoundId getSound() const;
  optional<int> getLearningExpLevel() const;

  static void init();

  // TODO: why keep const members private? there is no danger of misuse
  // Also gettters wouldn't be necessary
  private:
  Spell(const string&, Effect, int difficulty, SoundId, CastMessageType = CastMessageType::STANDARD);
  Spell(const string&, DirEffectType, int difficulty, SoundId, CastMessageType = CastMessageType::STANDARD);

  const string name;
  const HeapAllocated<variant<Effect, DirEffectType>> effect;
  const int difficulty;
  const CastMessageType castMessageType;
  const SoundId sound;
};

