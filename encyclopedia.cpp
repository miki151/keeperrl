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

namespace {
void bestiary(View* view, int lastInd) {
}

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

void advance(View* view, const Technology* tech) {
  string text;
  const vector<Technology*>& prerequisites = tech->getPrerequisites();
  const vector<Technology*>& allowed = tech->getAllowed();
  if (!prerequisites.empty())
    text += "Requires: " + combine(prerequisites) + "\n";
  if (!allowed.empty())
    text += "Allows research: " + combine(allowed) + "\n";
  const vector<BuildInfo::RoomInfo>& rooms = BuildInfo::getRoomInfo().filter(
      [tech] (const BuildInfo::RoomInfo& info) {
          for (auto& req : info.requirements)
            if (req.getId() == BuildInfo::RequirementId::TECHNOLOGY && req.get<TechId>() == tech->getId())
              return true;
          return false;});
  if (!rooms.empty())
    text += "Unlocks rooms: " + combine(rooms) + "\n";
  if (!tech->canResearch())
    text += " \nCan only be acquired by special means.";
  view->presentText(capitalFirst(tech->getName()), text);
}

void advances(View* view, int lastInd = 0) {
  vector<ListElem> options;
  vector<Technology*> techs = Technology::getSorted();
  for (Technology* tech : techs)
    options.push_back(tech->getName());
  auto index = view->chooseFromList("Advances", options, lastInd);
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

void room(View* view, BuildInfo::RoomInfo& info) {
  string text = info.description;
  for (auto& req : info.requirements)
    text += "\n \nRequires: " + BuildInfo::getRequirementText(req) + ".";
  view->presentText(info.name, text);
}

void rooms(View* view, int lastInd = 0) {
  vector<ListElem> options;
  vector<BuildInfo::RoomInfo> roomList = BuildInfo::getRoomInfo();
  for (auto& elem : roomList)
    options.push_back(elem.name);
  auto index = view->chooseFromList("Rooms", options, lastInd);
  if (!index)
    return;
  room(view, roomList[*index]);
  rooms(view, *index);
}

void spells(View* view) {
  vector<ListElem> options;
  options.emplace_back("Spell:", "Level:", ListElem::ElemMod::TITLE);
  vector<Spell*> spells = Spell::getAll();
  sort(spells.begin(), spells.end(), [](const Spell* s1, const Spell* s2) {
      auto l1 = s1->getLearningExpLevel();
      auto l2 = s2->getLearningExpLevel();
      return (l1 && !l2) || (l2 && l1 && *l1 < *l2); });
  for (auto spell : spells) {
    auto level = spell->getLearningExpLevel();
    options.emplace_back(spell->getName(), level ? toString(*level) : "none"_s, ListElem::ElemMod::TEXT);
  }
  view->presentList("List of spells and the spellcaster levels at which they are acquired.", options);
}

}

void Encyclopedia::present(View* view, int lastInd) {
  auto index = view->chooseFromList("Choose topic:", {"Technology", "Skills", "Spells"}, lastInd);
  if (!index)
    return;
  switch (*index) {
    case 0: advances(view); break;
//    case 1: workshop(view); break;
    case 1: skills(view); break;
    case 2: spells(view); break;
    default: FATAL << "wfepok";
  }
  present(view, *index);
}


