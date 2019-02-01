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
#include "tribe.h"
#include "experience_type.h"

class Creature;
class MonsterAIFactory;
class Tribe;
class ItemType;
class CreatureAttributes;
class ControllerFactory;
class NameGenerator;
class GameConfig;

class CreatureFactory {
  public:
  PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&) const;
  PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&, const vector<ItemType>& inventory) const;
  PCreature fromId(CreatureId, TribeId) const;
  static PController getShopkeeper(Rectangle shopArea, Creature*);
  static PCreature getRollingBoulder(TribeId, Vec2 direction);
  static PCreature getHumanForTests();
  PCreature getGhost(Creature*) const;
  static PCreature getIllusion(Creature*);

  static void addInventory(Creature*, const vector<ItemType>& items);
  static CreatureAttributes getKrakenAttributes(ViewId, const char* name);
  ViewId getViewId(CreatureId) const;
  const Gender& getGender(CreatureId);

  CreatureFactory(NameGenerator*, const GameConfig*);
  ~CreatureFactory();

  private:
  void initSplash(TribeId);
  static PCreature getSokobanBoulder(TribeId);
  PCreature getSpecial(TribeId, bool humanoid, bool large, bool living, bool wings, const ControllerFactory&) const;
  PCreature get(CreatureId, TribeId, MonsterAIFactory) const;
  static PCreature get(CreatureAttributes, TribeId, const ControllerFactory&);
  CreatureAttributes getAttributesFromId(CreatureId) const;
  CreatureAttributes getAttributes(CreatureId) const;
  mutable map<CreatureId, ViewId> idMap;
  NameGenerator* nameGenerator;
  map<CreatureId, CreatureAttributes> attributes;
};
