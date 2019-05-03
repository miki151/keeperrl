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

void Encyclopedia::advance(View* view, TechId tech) const {
  string text;
  const vector<TechId>& prerequisites = technology.techs.at(tech).prerequisites;
  vector<TechId> allowed = technology.getAllowed(tech);
  if (!prerequisites.empty())
    text += "Requires: " + combine(prerequisites) + "\n";
  if (!allowed.empty())
    text += "Allows research: " + combine(allowed) + "\n";
  const vector<BuildInfo>& rooms = buildInfo.filter(
      [tech] (const BuildInfo& info) {
          for (auto& req : info.requirements)
            if (auto techReq = req.getReferenceMaybe<TechId>())
              if (*techReq == tech)
                return true;
          return false;});
  if (!rooms.empty())
    text += "Unlocks rooms: " + combine(rooms) + "\n";
  view->presentText(capitalFirst(tech), text);
}

void Encyclopedia::advances(View* view, int lastInd) const {
  auto techs = technology.getSorted();
  auto index = view->chooseFromList("Advances", ListElem::convert(techs), lastInd);
  if (!index)
    return;
  advance(view, techs[*index]);
  advances(view, *index);
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

void showSpells(View* view, const pair<string, SpellSchool>& school) {
  vector<ListElem> options;
  options.emplace_back("Spell:", "Level:", ListElem::ElemMod::TITLE);
  auto& spells = school.second.spells;
  /*sort(spells.begin(), spells.end(), [](const Spell& s1, const Spell& s2) {
    return std::forward_as_tuple(s1.getExpLevel(), s1.getName()) <
           std::forward_as_tuple(s2.getExpLevel(), s2.getName());});*/
  for (auto& spell : spells) {
    options.emplace_back(spell.first, spell.second > 0 ? toString(spell.second) : "none"_s, ListElem::ElemMod::TEXT);
  }
  view->presentList("List of spells and the spellcaster levels at which they are acquired.", options);
}

void Encyclopedia::spellSchools(View* view, int lastInd) const {
  vector<ListElem> options;
  vector<pair<string, SpellSchool>> pairs;
  for (auto& school : schools) {
    options.push_back(school.first);
    pairs.push_back(school);
  }
  auto index = view->chooseFromList("Spell schools", options, lastInd);
  if (!index)
    return;
  showSpells(view, pairs[*index]);
  spellSchools(view, *index);
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

Encyclopedia::Encyclopedia(vector<BuildInfo> buildInfo, map<string, SpellSchool> spellSchools,
    vector<Spell> spells, const Technology& technology)
    : buildInfo(std::move(buildInfo)), schools(spellSchools), spells(spells), technology(technology) {
}

void Encyclopedia::present(View* view, int lastInd) {
  auto index = view->chooseFromList("Choose topic:", {"Technology", "Skills", "Spells", "Level increases"}, lastInd);
  if (!index)
    return;
  switch (*index) {
    case 0: advances(view); break;
    case 1: skills(view); break;
    case 2: spellSchools(view); break;
    case 3: villainPoints(view); break;
    default: FATAL << "wfepok";
  }
  present(view, *index);
}


