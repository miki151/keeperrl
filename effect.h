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

#include "util.h"
#include "position.h"

class Level;
class Creature;
class Item;
class Tribe;
class CreatureGroup;

class EffectType;
class ContentFactory;
struct Color;

class Effect {
  public:

  Effect(const EffectType&) noexcept;
  Effect(const Effect&) noexcept;
  Effect(Effect&&) noexcept;
  Effect() noexcept;
  Effect& operator = (const Effect&);
  Effect& operator = (Effect&&);
  ~Effect();

  template <class Archive>
  void serialize(Archive&, const unsigned int);

  bool apply(Position, Creature* attacker = nullptr) const;
  bool applyToCreature(Creature*, Creature* attacker = nullptr) const;
  string getName(const ContentFactory*) const;
  string getDescription(const ContentFactory*) const;
  optional<MinionEquipmentType> getMinionEquipmentType() const;
  bool canAutoAssignMinionEquipment() const;
  optional<FXInfo> getProjectileFX() const;
  optional<ViewId> getProjectile() const;
  Color getColor(const ContentFactory*) const;
  int getPrice(const ContentFactory*) const;

  EffectAIIntent shouldAIApply(const Creature* caster, Position) const;

  static vector<Creature*> summon(Creature*, CreatureId, int num, optional<TimeInterval> ttl, TimeInterval delay = 0_visible,
      optional<Position> = none);
  static vector<Creature*> summon(Position, CreatureGroup&, int num, optional<TimeInterval> ttl, TimeInterval delay = 0_visible);
  static vector<Creature*> summonCreatures(Position, vector<PCreature>, TimeInterval delay = 0_visible);
  static vector<Effect> getWishedForEffects(const ContentFactory*);
  static void enhanceWeapon(Creature*, int mod, const string& msg);
  static void enhanceArmor(Creature*, int mod, const string& msg);

  HeapAllocated<EffectType> SERIAL(effect);
};

static_assert(std::is_nothrow_move_constructible<Effect>::value, "T should be noexcept MoveConstructible");
