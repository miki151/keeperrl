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

#include "stdafx.h"

#include "pantheon.h"
#include "creature.h"
#include "level.h"
#include "name_generator.h"

template <class Archive> 
void Deity::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(name)
    & SVAR(gender)
    & SVAR(epithets)
    & SVAR(habitat);
}

SERIALIZABLE(Deity);

SERIALIZATION_CONSTRUCTOR_IMPL(Deity);

static map<DeityHabitat, vector<EpithetId>> epithetsMap {
  { DeityHabitat::FIRE,   // fire elemental
      { EpithetId::WAR, EpithetId::DEATH, EpithetId::DESTRUCTION, EpithetId::WEALTH,
        EpithetId::FEAR, EpithetId::CRAFTS, EpithetId::LIGHT, EpithetId::DARKNESS }},
  { DeityHabitat::EARTH, // earth elemental
      { EpithetId::HEALTH, EpithetId::NATURE, EpithetId::LOVE,
        EpithetId::WEALTH, EpithetId::MIND, EpithetId::CHANGE, EpithetId::DEFENSE, EpithetId::DARKNESS }},
  { DeityHabitat::TREES, // ent?
      { EpithetId::HEALTH, EpithetId::NATURE, EpithetId::LOVE, EpithetId::LIGHT,
        EpithetId::CHANGE, EpithetId::CRAFTS, EpithetId::FORTUNE, EpithetId::SECRETS,  }},
  { DeityHabitat::WATER, // water elemental
      { EpithetId::NATURE, EpithetId::WISDOM, EpithetId::WEALTH, EpithetId::MIND, EpithetId::DESTRUCTION,
        EpithetId::CHANGE, EpithetId::DEFENSE, EpithetId::FEAR, EpithetId::COURAGE, EpithetId::HEALTH,  }},
  { DeityHabitat::AIR, // air elemental
      { EpithetId::MIND, EpithetId::LIGHTNING, EpithetId::LIGHT, EpithetId::CHANGE,
        EpithetId::FORTUNE, EpithetId::FEAR, EpithetId::COURAGE,  }},
  { DeityHabitat::STARS, // ?
      { EpithetId::DEATH, EpithetId::WAR, EpithetId::WISDOM, EpithetId::LOVE,
        EpithetId::DARKNESS, EpithetId::FORTUNE, EpithetId::SECRETS,  }}

};

/*
    CHANGE earth, water, trees, earth
    COURAGE air, water
    CRAFTS trees, fire
    DARKNESS stars, earth, fire
    DEATH stars, fire
    DEFENSE water, earth
    DESTRUCTION water, fire
    FEAR air, water, fire
    FORTUNE stars, air, trees
    HEALTH water, trees, earth
    LIGHT air, trees, fire
    LIGHTNING air
    LOVE stars, air, trees
    MIND air, water, earth
    NATURE water, trees, earth
    SECRETS stars, trees
    WAR stars, fire
    WEALTH water, earth, fire
    WISDOM stars, water
*/

Epithet::Epithet(const string& n, const string& d) : name(n), description(d) {
}

void Epithet::init() {
  set(EpithetId::CHANGE, new Epithet("change", ""));
  set(EpithetId::COURAGE, new Epithet("courage", "Increases the morale of your minions.")); // ?
  set(EpithetId::CRAFTS, new Epithet("crafts", "Your minions work more effectively in the workshops."));
  set(EpithetId::DARKNESS, new Epithet("darkness", "Makes the night last longer."));
  set(EpithetId::DEATH, new Epithet("death",
    "Decreases the cost of raising undead, and sometimes turns the Keeper or other minions into zombies."));
  set(EpithetId::DEFENSE, new Epithet("defense", "Sends servants when the Keeper is in danger."));
  set(EpithetId::DESTRUCTION, new Epithet("destruction", ""));
  set(EpithetId::FEAR, new Epithet("fear", "Decreases the morale of your minions."));
  set(EpithetId::FORTUNE, new Epithet("fortune", "")); // ?
  set(EpithetId::HEALTH, new Epithet("health", "Heals the Keeper when he is in dire need."));
  set(EpithetId::LIGHT, new Epithet("light", "Makes the day last longer.")); // ?
  set(EpithetId::LIGHTNING, new Epithet("lightning", "Protects the Keeper by striking lightings at attackers."));
  set(EpithetId::LOVE, new Epithet("love", "Makes your enemies less inclined to attack you.")); // potion of love
  set(EpithetId::MIND, new Epithet("mind", ""));
  set(EpithetId::NATURE, new Epithet("nature", "Decreases the cost of taming beasts."));
  set(EpithetId::SECRETS, new Epithet("secrets", "Has a chance of revealing a piece land."));
  set(EpithetId::WAR, new Epithet("war", "Makes your enemies hate you more."));
  set(EpithetId::WEALTH, new Epithet("wealth", ""));
  set(EpithetId::WISDOM, new Epithet("wisdom", "Decreases the cost of research."));
}

const string& Epithet::getName() const {
  return name;
}

const string& Epithet::getDescription() const {
  return description;
}

const string& Deity::getName() const {
  return name;
}

Gender Deity::getGender() const {
  return gender;
}

vector<EpithetId> Deity::getEpithets() const {
  return epithets;
}

string Deity::getEpithetsString() const {
  vector<string> v;
  for (EpithetId e : epithets)
    v.push_back(Epithet::get(e)->getName());
  return combine(v);
}

static string toString(DeityHabitat h) {
  switch (h) {
    case DeityHabitat::EARTH: return "the earth";
    case DeityHabitat::FIRE: return "fire";
    case DeityHabitat::AIR: return "the air";
    case DeityHabitat::TREES: return "the trees";
    case DeityHabitat::WATER: return "water";
    case DeityHabitat::STARS: return "the stars";
  }
  return "";
}

DeityHabitat Deity::getHabitat() const {
  return habitat;
}

string Deity::getHabitatString() const {
  return toString(habitat);
}

vector<Deity*> generateDeities() {
  set<EpithetId> used;
  vector<Deity*> ret;
  for (auto elem : epithetsMap) {
    string deity = NameGenerator::get(NameGeneratorId::DEITY)->getNext();
    Gender gend = Gender::male;
    if ((deity.back() == 'a' || deity.back() == 'i') && !Random.roll(4))
      gend = Gender::female;
    vector<EpithetId> ep;
    for (int i : Range(Random.get(1, 4))) {
      EpithetId epithet;
      int cnt = 100;
      do {
        epithet = Random.choose(elem.second);
      } while (used.count(epithet) && --cnt > 0);
      if (cnt == 0)
        break;
      used.insert(epithet);
      ep.push_back(epithet);
    }
    if (ep.empty())
      return generateDeities();
    ret.push_back(new Deity(deity, gend, ep, elem.first)); 
  }
  for (Deity* deity : ret)
    Debug() << deity->getName() + " lives in " + deity->getHabitatString() + ". " 
      << deity->getGender().he() << " is the " << deity->getGender().god() 
          << " of " << deity->getEpithetsString();
  return ret;
}

vector<Deity*> Deity::getDeities() {
  if (deities.empty()) {
    deities = generateDeities();
    for (Deity* d : deities)
      for (EpithetId id : d->getEpithets())
        byEpithet[id] = d;
  }
  return deities;
}

Deity* Deity::getDeity(EpithetId id) {
  return byEpithet[id];
}

Deity* Deity::getDeity(DeityHabitat h) {
  for (Deity* d : getDeities())
    if (d->getHabitat() == h)
      return d;
  FAIL << "Didn't find deity for habitat " << (int)h;
  return getDeities()[0];
}
 
vector<Deity*> Deity::deities;
EnumMap<EpithetId, Deity*> Deity::byEpithet;

Deity::Deity(const string& n, Gender g, const vector<EpithetId>& e, DeityHabitat h) :
    name(n), gender(g), epithets(e), habitat(h) {
}

CreatureFactory Deity::getServant(Tribe* tribe) const {
  CreatureId id = CreatureId(0);
  switch (habitat) {
    case DeityHabitat::AIR: id = CreatureId::AIR_ELEMENTAL; break;
    case DeityHabitat::FIRE: id = CreatureId::FIRE_ELEMENTAL; break;
    case DeityHabitat::WATER: id = CreatureId::WATER_ELEMENTAL; break;
    case DeityHabitat::EARTH: id = CreatureId::EARTH_ELEMENTAL; break;
    case DeityHabitat::TREES: id = CreatureId::ENT; break;
    case DeityHabitat::STARS: id = CreatureId::ANGEL; break;
  }
  return CreatureFactory::singleType(tribe, id);
}
