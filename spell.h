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
#include "view_id.h"
#include "fx_info.h"
#include "keybinding.h"
#include "t_string.h"

class Effect;
class Position;
class MoveInfo;
class ContentFactory;

RICH_ENUM(
  SpellType,
  SPELL,
  ABILITY
);

class Spell {
  public:
  STRUCT_DECLARATIONS(Spell)
  const string& getSymbol() const;
  const Effect& getEffect() const;
  Range getCooldown() const;
  vector<TString> getDescription(const Creature*, const ContentFactory*) const;
  void addMessage(Creature*) const;
  optional<SoundId> getSound() const;
  bool canTargetSelf() const;
  void apply(Creature*, Position target) const;
  int getRange() const;
  bool isEndOnly() const;
  void setSpellId(SpellId);
  SpellId getId() const;
  TString getName(const ContentFactory*) const;
  optional<SpellId> getUpgrade() const;
  void getAIMove(const Creature*, MoveInfo&) const;
  bool isBlockedBy(const Creature* c, Position pos) const;
  SpellType getType() const;
  bool isFriendlyFire(const Creature* caster, Position to) const;

  SERIALIZATION_DECL(Spell)
  template <class Archive>
  void serializeImpl(Archive& ar1, const unsigned int);

  private:
  PrimaryId<SpellId> SERIAL(id);
  optional<SpellId> SERIAL(upgrade);
  string SERIAL(symbol);
  HeapAllocated<Effect> SERIAL(effect);
  Range SERIAL(cooldown);
  pair<TString, TString> SERIAL(message) = {TStringId("YOU_CAST_SPELL"), TStringId("CASTS_SPELL")};
  optional<SoundId> SERIAL(sound);
  int SERIAL(range) = 0;
  optional<FXInfo> SERIAL(fx);
  optional<ViewId> SERIAL(projectileViewId);
  bool SERIAL(endOnly) = false;
  bool SERIAL(targetSelf) = false;
  bool SERIAL(blockedByWall) = true;
  optional<int> SERIAL(maxHits);
  SpellType SERIAL(type) = SpellType::SPELL;
  int checkTrajectory(const Creature* caster, Position to) const;
};

