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
#include "pantheon.h"
#include "technology.h"
#include "creature_factory.h"
#include "creature.h"

void bestiary(View* view, int lastInd) {
}

void advance(View* view, const Technology* tech) {
  string text;
  const vector<Technology*>& prerequisites = tech->getPrerequisites();
  const vector<Technology*>& allowed = tech->getAllowed();
  if (!prerequisites.empty())
    text += "Requires: " + combine(prerequisites) + "\n";
  if (!allowed.empty())
    text += "Allows research: " + combine(allowed) + "\n";
  const vector<SpellInfo>& spells = PlayerControl::getSpellLearning(tech);
  if (!spells.empty())
    text += "Teaches spells: " + combine(spells) + "\n";
  const vector<PlayerControl::RoomInfo>& rooms = filter(PlayerControl::getRoomInfo(), 
      [tech] (const PlayerControl::RoomInfo& info) { return info.techId && Technology::get(*info.techId) == tech;});
  if (!rooms.empty())
    text += "Unlocks rooms: " + combine(rooms) + "\n";
  vector<string> minions = transform2<string>(PlayerControl::getSpawnInfo(tech),
      [](CreatureId id) { return CreatureFactory::fromId(id, Tribe::get(TribeId::MONSTER))->getSpeciesName();});
  if (!minions.empty())
    text += "Unlocks minions: " + combine(minions) + "\n";
  if (!tech->canResearch())
    text += " \nCan only be acquired by special means.";
  view->presentText(capitalFirst(tech->getName()), text);
}

void advances(View* view, int lastInd = 0) {
  vector<View::ListElem> options;
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
  vector<View::ListElem> options;
  vector<Skill*> s = Skill::getAll();
  for (Skill* skill : s)
    options.push_back(skill->getName());
  auto index = view->chooseFromList("Skills", options, lastInd);
  if (!index)
    return;
  skill(view, s[*index]);
  skills(view, *index);
}

void room(View* view, PlayerControl::RoomInfo& info) {
  string text = info.description;
  if (info.techId)
    text += "\n \nRequires: " + Technology::get(*info.techId)->getName();
  view->presentText(info.name, text);
}

void rooms(View* view, int lastInd = 0) {
  vector<View::ListElem> options;
  vector<PlayerControl::RoomInfo> roomList = PlayerControl::getRoomInfo();
  for (auto& elem : roomList)
    options.push_back(elem.name);
  auto index = view->chooseFromList("Rooms", options, lastInd);
  if (!index)
    return;
  room(view, roomList[*index]);
  rooms(view, *index);
}

void workshop(View* view, int lastInd = 0) {
  vector<View::ListElem> options;
  vector<PlayerControl::RoomInfo> roomList = PlayerControl::getWorkshopInfo();
  for (auto& elem : roomList)
    options.push_back(elem.name);
  auto index = view->chooseFromList("Workshop", options, lastInd);
  if (!index)
    return;
  room(view, roomList[*index]);
  workshop(view, *index);
}

void tribes(View* view, int lastInd = 0) {
}

void deity(View* view, const Deity* deity) {
  view->presentText(deity->getName(),
      "Lives in " + deity->getHabitatString() + " and is the " + deity->getGender().god() + " of "
      + deity->getEpithets() + ".");
}

void deities(View* view, int lastInd = 0) {
  vector<View::ListElem> options;
  for (Deity* deity : Deity::getDeities())
    options.push_back(deity->getName());
  auto index = view->chooseFromList("Deities", options, lastInd);
  if (!index)
    return;
  deity(view, Deity::getDeities()[*index]);
  deities(view, *index);
}

void Encyclopedia::present(View* view, int lastInd) {
  auto index = view->chooseFromList("Choose topic:",
      {"Advances", "Workshop", "Deities", "Skills"}, lastInd);
  if (!index)
    return;
  switch (*index) {
    case 0: advances(view); break;
    case 1: workshop(view); break;
    case 2: deities(view); break;
    case 3: skills(view); break;
    default: FAIL << "wfepok";
  }
  present(view, *index);
}


