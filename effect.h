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
#include "enums.h"
#include "position.h"

class Level;
class Creature;
class Item;
class Tribe;
class CreatureFactory;
class EffectType;
class DirEffectType;

class Effect {
  public:
  static void applyToCreature(WCreature, const EffectType&, EffectStrength);
  static void applyToPosition(Position, const EffectType&, EffectStrength);
  static void applyDirected(WCreature, Vec2 direction, const DirEffectType&, EffectStrength);

  static vector<WCreature> summon(WCreature, CreatureId, int num, int ttl, double delay = 0);
  static vector<WCreature> summon(Position, CreatureFactory&, int num, int ttl, double delay = 0);
  static vector<WCreature> summonCreatures(Position, int radius, vector<PCreature>, double delay = 0);
  static vector<WCreature> summonCreatures(WCreature, int radius, vector<PCreature>, double delay = 0);
  static string getName(const EffectType&);
  static const char* getName(LastingEffect);
  static string getDescription(const EffectType&);
  static string getDescription(const DirEffectType&);
  static const char* getDescription(LastingEffect);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);
};

enum class EffectStrength { WEAK, NORMAL, STRONG };


