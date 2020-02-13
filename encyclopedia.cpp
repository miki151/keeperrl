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
#include "encyclopedia.h"
#include "view.h"
#include "player_control.h"
#include "collective.h"
#include "technology.h"
#include "creature_factory.h"
#include "creature.h"
#include "spell.h"
#include "skill.h"
#include "build_info.h"
#include "dungeon_level.h"
#include "spell_school.h"
#include "content_factory.h"
#include "creature_factory.h"
#include "game_info.h"
#include "creature_name.h"

template<class T>
string combine(const vector<T*>& v) {
  return combine(
      v.transform([](const T* e) -> string { return e->getName(); }));
}

template<class T>
string combine(const vector<T>& v) {
  return combine(
      v.transform([](const T& e) -> string { return e.name; }));
}

string combine(const vector<TechId>& v) {
  return combine(v.transform([](TechId id) -> string { return id.data(); }));
}

void skill(View* view, const Skill* skill) {
  view->presentText(capitalFirst(skill->getName()), skill->getHelpText());
}

void skills(View* view, int lastInd = 0) {
  vector<ListElem> options;
  vector<Skill*> s = Skill::getAll();
  for (Skill* skill : s)
    options.push_back(skill->getName());
  auto index = view->chooseFromList("Skills", options, lastInd);
  if (!index)
    return;
  skill(view, s[*index]);
  skills(view, *index);
}

void showSpells(View* view, const pair<SpellSchoolId, SpellSchool>& school) {
  vector<ListElem> options;
  options.emplace_back("Spell:", "Level:", ListElem::ElemMod::TITLE);
  auto& spells = school.second.spells;
  /*sort(spells.begin(), spells.end(), [](const Spell& s1, const Spell& s2) {
    return std::forward_as_tuple(s1.getExpLevel(), s1.getName()) <
           std::forward_as_tuple(s2.getExpLevel(), s2.getName());});*/
  for (auto& spell : spells) {
    options.emplace_back(spell.first.data(), spell.second > 0 ? toString(spell.second) : "none"_s, ListElem::ElemMod::TEXT);
  }
  view->presentList("List of spells and the spellcaster levels at which they are acquired.", options);
}

void villainPoints(View* view) {
  vector<ListElem> options;
  options.emplace_back("Villain type:", "Points:", ListElem::ElemMod::TITLE);
  for (auto type : ENUM_ALL(VillainType))
    if (type != VillainType::PLAYER) {
    auto points = int(100 * DungeonLevel::getProgress(type));
    options.emplace_back(getName(type), toString(points), ListElem::ElemMod::TEXT);
  }
  view->presentList("Experience points awarded for conquering each villain type.", options);
}

static vector<PlayerInfo> getBestiary(ContentFactory* f) {
  vector<PlayerInfo> ret;
  for (auto& id : f->getCreatures().getAllCreatures()) {
    auto c = f->getCreatures().fromId(id, TribeId::getMonster());
    ret.push_back(PlayerInfo(c.get(), f));
    ret.back().name = c->getName().groupOf(1);
  }
  return ret;
}

Encyclopedia::Encyclopedia(ContentFactory* f)
    : schools(f->getCreatures().getSpellSchools()), spells(f->getCreatures().getSpells()), bestiary(getBestiary(f)) {
}

Encyclopedia::~Encyclopedia() {
}


