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
#include "spell_school.h"

class Creature;
class MonsterAIFactory;
class Tribe;
class ItemType;
class CreatureAttributes;
class ControllerFactory;
class NameGenerator;
class GameConfig;
class CreatureInventory;
class SpellMap;

class CreatureFactory {
  public:
  PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&);
  PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&, const vector<ItemType>& inventory);
  PCreature fromId(CreatureId, TribeId);
  static PController getShopkeeper(Rectangle shopArea, Creature*);
  static PCreature getRollingBoulder(TribeId, Vec2 direction);
  static PCreature getHumanForTests();
  PCreature getGhost(Creature*);
  static PCreature getIllusion(Creature*);

  static CreatureAttributes getKrakenAttributes(ViewId, const char* name);
  ViewId getViewId(CreatureId);
  const Gender& getGender(CreatureId);
  vector<CreatureId> getAllCreatures() const;

  NameGenerator* getNameGenerator();

  const map<string, SpellSchool> getSpellSchools() const;
  const vector<Spell>& getSpells() const;
  const Spell* getSpell(const string&) const;
  const string& getSpellName(const Spell*) const;
  CreatureFactory(NameGenerator, map<CreatureId, CreatureAttributes>, map<CreatureId, CreatureInventory>, map<string, SpellSchool>,
      vector<Spell>);
  ~CreatureFactory();
  CreatureFactory(const CreatureFactory&) = delete;
  CreatureFactory(CreatureFactory&&);
  CreatureFactory& operator = (CreatureFactory&&);

  void merge(CreatureFactory);

  SERIALIZATION_DECL(CreatureFactory)

  private:
  void initSplash(TribeId);
  static PCreature getSokobanBoulder(TribeId);
  PCreature getSpecial(TribeId, bool humanoid, bool large, bool living, bool wings, const ControllerFactory&);
  PCreature get(CreatureId, TribeId, MonsterAIFactory);
  static PCreature get(CreatureAttributes, TribeId, const ControllerFactory&, SpellMap);
  CreatureAttributes getAttributesFromId(CreatureId);
  CreatureAttributes getAttributes(CreatureId);
  map<CreatureId, ViewId> idMap;
  HeapAllocated<NameGenerator> SERIAL(nameGenerator);
  map<CreatureId, CreatureAttributes> SERIAL(attributes);
  map<CreatureId, CreatureInventory> SERIAL(inventory);
  vector<ItemType> getDefaultInventory(CreatureId) const;
  map<string, SpellSchool> SERIAL(spellSchools);
  vector<Spell> SERIAL(spells);
  static void addInventory(Creature*, const vector<ItemType>& items);
};
