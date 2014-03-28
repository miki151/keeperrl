#include "stdafx.h"
#include "encyclopedia.h"
#include "view.h"
#include "collective.h"
#include "pantheon.h"
#include "technology.h"
#include "creature_factory.h"

void Encyclopedia::present(View* view, int lastInd) {
  auto index = view->chooseFromList("Choose topic:",
      {"Advances", "Workshop", "Deities"}, lastInd);
  if (!index)
    return;
  switch (*index) {
    case 0: advances(view); break;
    case 1: workshop(view); break;
    case 2: deities(view); break;
    default: FAIL << "wfepok";
  }
  present(view, *index);
}

void Encyclopedia::bestiary(View* view, int lastInd) {
}

void Encyclopedia::advance(View* view, const Technology* tech) {
  string text;
  const vector<Technology*>& prerequisites = tech->getPrerequisites();
  const vector<Technology*>& allowed = tech->getAllowed();
  if (!prerequisites.empty())
    text += "Requires: " + combine(prerequisites) + "\n";
  if (!allowed.empty())
    text += "Allows research: " + combine(allowed) + "\n";
  const vector<SpellInfo>& spells = Collective::getSpellLearning(tech);
  if (!spells.empty())
    text += "Teaches spells: " + combine(spells) + "\n";
  const vector<Collective::RoomInfo>& rooms = filter(Collective::getRoomInfo(), 
      [tech] (const Collective::RoomInfo& info) { return info.techId && Technology::get(*info.techId) == tech;});
  if (!rooms.empty())
    text += "Unlocks rooms: " + combine(rooms) + "\n";
  vector<string> minions = transform2<string>(Collective::getSpawnInfo(tech),
      [](CreatureId id) { return CreatureFactory::fromId(id, Tribes::get(TribeId::MONSTER))->getSpeciesName();});
  if (!minions.empty())
    text += "Unlocks minions: " + combine(minions) + "\n";
  if (!tech->canResearch())
    text += " \nCan only be acquired by special means.";
  view->presentText(capitalFirst(tech->getName()), text);
}

void Encyclopedia::advances(View* view, int lastInd) {
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

void Encyclopedia::room(View* view, Collective::RoomInfo& info) {
  string text = info.description;
  if (info.techId)
    text += "\n \nRequires: " + Technology::get(*info.techId)->getName();
  view->presentText(info.name, text);
}

void Encyclopedia::rooms(View* view, int lastInd) {
  vector<View::ListElem> options;
  vector<Collective::RoomInfo> roomList = Collective::getRoomInfo();
  for (auto& elem : roomList)
    options.push_back(elem.name);
  auto index = view->chooseFromList("Rooms", options, lastInd);
  if (!index)
    return;
  room(view, roomList[*index]);
  rooms(view, *index);
}

void Encyclopedia::workshop(View* view, int lastInd) {
  vector<View::ListElem> options;
  vector<Collective::RoomInfo> roomList = Collective::getWorkshopInfo();
  for (auto& elem : roomList)
    options.push_back(elem.name);
  auto index = view->chooseFromList("Workshop", options, lastInd);
  if (!index)
    return;
  room(view, roomList[*index]);
  workshop(view, *index);
}

void Encyclopedia::tribes(View* view, int lastInd) {
}

void Encyclopedia::deity(View* view, const Deity* deity) {
  view->presentText(deity->getName(),
      "Lives in " + deity->getHabitatString() + " and is the " + deity->getGender().god() + " of "
      + deity->getEpithets() + ".");
}

void Encyclopedia::deities(View* view, int lastInd) {
  vector<View::ListElem> options;
  for (Deity* deity : Deity::getDeities())
    options.push_back(deity->getName());
  auto index = view->chooseFromList("Deities", options, lastInd);
  if (!index)
    return;
  deity(view, Deity::getDeities()[*index]);
  deities(view, *index);
}


