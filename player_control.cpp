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

#include "player_control.h"
#include "level.h"
#include "task.h"
#include "player.h"
#include "message_buffer.h"
#include "model.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "music.h"
#include "village_control.h"
#include "pantheon.h"
#include "item.h"
#include "item_factory.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "collective.h"
#include "effect.h"

template <class Archive> 
void PlayerControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CreatureView)
    & SUBCLASS(CollectiveControl)
    & SVAR(memory)
    & SVAR(gatheringTeam)
    & SVAR(team)
    & SVAR(possessed)
    & SVAR(model)
    & SVAR(showWelcomeMsg)
    & SVAR(lastControlKeeperQuestion)
    & SVAR(startImpNum)
    & SVAR(retired)
    & SVAR(payoutWarning)
    & SVAR(surprises);
  CHECK_SERIAL;
}

SERIALIZABLE(PlayerControl);

SERIALIZATION_CONSTRUCTOR_IMPL(PlayerControl);

typedef Collective::CostInfo CostInfo;
typedef Collective::ResourceId ResourceId;

PlayerControl::BuildInfo::BuildInfo(SquareInfo info, Optional<TechId> id, const string& h, char key, string group)
    : squareInfo(info), buildType(SQUARE), techId(id), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(TrapInfo info, Optional<TechId> id, const string& h, char key, string group)
    : trapInfo(info), buildType(TRAP), techId(id), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(DeityHabitat habitat, CostInfo cost, const string& group, const string& h,
    char key) : squareInfo({SquareType(SquareId::ALTAR, habitat), cost, "To " + Deity::getDeity(habitat)->getName(), false}),
    buildType(SQUARE), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(const Creature* c, CostInfo cost, const string& group, const string& h, char key)
    : squareInfo({SquareType(SquareId::CREATURE_ALTAR, c), cost, "To " + c->getName(), false}),
    buildType(SQUARE), help(h), hotkey(key), groupName(group) {}

PlayerControl::BuildInfo::BuildInfo(BuildType type, const string& h, char key, string group)
    : buildType(type), help(h), hotkey(key), groupName(group) {
  CHECK(contains({DIG, IMP, GUARD_POST, DESTROY, FETCH, DISPATCH, CLAIM_TILE}, type));
}

PlayerControl::BuildInfo::BuildInfo(MinionInfo info, Optional<TechId> tech, const string& group, const string& h)
    : minionInfo(info), buildType(MINION), techId(tech), help(h), hotkey(0), groupName(group) {}

vector<PlayerControl::BuildInfo> PlayerControl::getBuildInfo() const {
  return getBuildInfo(getLevel(), getTribe());
}

vector<PlayerControl::BuildInfo> PlayerControl::getBuildInfo(const Level* level, const Tribe* tribe) {
  const CostInfo altarCost {ResourceId::STONE, 30};
  vector<BuildInfo> buildInfo {
    BuildInfo(BuildInfo::DIG, "", 'd'),
    BuildInfo({SquareId::STOCKPILE, {ResourceId::GOLD, 0}, "Everything", true}, Nothing(), "", 's', "Storage"),
    BuildInfo({SquareId::STOCKPILE_EQUIP, {ResourceId::GOLD, 0}, "Equipment", true}, Nothing(), "", 0, "Storage"),
    BuildInfo({SquareId::STOCKPILE_RES, {ResourceId::GOLD, 0}, "Resources", true}, Nothing(), "", 0, "Storage"),
    BuildInfo({SquareId::TREASURE_CHEST, {ResourceId::WOOD, 5}, "Treasure room"}, Nothing(), ""),
    BuildInfo({SquareId::DORM, {ResourceId::WOOD, 10}, "Dormitory"}, Nothing(), "", 'm'),
    BuildInfo({SquareId::TRAINING_ROOM, {ResourceId::IRON, 20}, "Training room"}, Nothing(), "", 't'),
    BuildInfo({SquareId::LIBRARY, {ResourceId::WOOD, 20}, "Library"}, Nothing(), "", 'y'),
    BuildInfo({SquareId::LABORATORY, {ResourceId::STONE, 15}, "Laboratory"}, TechId::ALCHEMY, "", 'r', "Workshops"),
    BuildInfo({SquareId::WORKSHOP, {ResourceId::IRON, 15}, "Forge"}, TechId::CRAFTING, "", 'f', "Workshops"),
    BuildInfo({SquareId::RITUAL_ROOM, {ResourceId::MANA, 15}, "Ritual room"}, Nothing(), ""),
    BuildInfo({SquareId::BEAST_LAIR, {ResourceId::WOOD, 12}, "Beast lair"}, Nothing(), ""),
    BuildInfo({SquareId::CEMETERY, {ResourceId::STONE, 20}, "Graveyard"}, Nothing(), "", 'v'),
    BuildInfo({SquareId::PRISON, {ResourceId::IRON, 20}, "Prison"}, Nothing(), "", 0),
    BuildInfo({SquareId::TORTURE_TABLE, {ResourceId::IRON, 20}, "Torture room"}, Nothing(), "", 'u')};
  for (Deity* deity : Deity::getDeities())
    buildInfo.push_back(BuildInfo(deity->getHabitat(), altarCost, "Shrines",
          deity->getGender().god() + " of " + deity->getEpithetsString(), 0));
  if (level)
    for (const Creature* c : level->getAllCreatures())
      if (c->isWorshipped())
        buildInfo.push_back(BuildInfo(c, altarCost, "Shrines", c->getSpeciesName(), 0));
  append(buildInfo, {
    BuildInfo(BuildInfo::GUARD_POST, "Place it anywhere to send a minion.", 'p', "Orders"),
    BuildInfo(BuildInfo::CLAIM_TILE, "Claim a tile. Building anything has the same effect.", 0, "Orders"),
    BuildInfo(BuildInfo::FETCH, "Order imps to fetch items from outside the dungeon.", 0, "Orders"),
    BuildInfo(BuildInfo::DISPATCH, "Click on an existing task to have imps perform it faster.", 'a', "Orders"),
    BuildInfo(BuildInfo::DESTROY, "", 'e', "Orders"),
    BuildInfo({{SquareId::TRIBE_DOOR, tribe}, {ResourceId::WOOD, 5}, "Door"}, TechId::CRAFTING,
        "Click on a built door to lock it.", 'o', "Installations"),
    BuildInfo({SquareId::BRIDGE, {ResourceId::WOOD, 20}, "Bridge"}, Nothing(), "", 0, "Installations"),
    BuildInfo({{SquareId::BARRICADE, tribe}, {ResourceId::WOOD, 20}, "Barricade"}, TechId::CRAFTING, "", 0,
      "Installations"),
    BuildInfo({SquareId::TORCH, {ResourceId::WOOD, 1}, "Torch"}, Nothing(), "", 'c', "Installations"),
    BuildInfo({SquareId::EYEBALL, {ResourceId::MANA, 10}, "Eyeball"}, Nothing(), "", 0, "Installations"),
    BuildInfo({SquareId::IMPALED_HEAD, {ResourceId::PRISONER_HEAD, 1}, "Prisoner head", false, true}, Nothing(),
        "Impaled head of an executed prisoner. Aggravates enemies.", 0, "Installations"),
    BuildInfo({TrapType::TERROR, "Terror trap", ViewId::TERROR_TRAP}, TechId::TRAPS,
        "Causes the trespasser to panic.", 0, "Traps"),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}, TechId::TRAPS,
        "Releases a cloud of poisonous gas.", 0, "Traps"),
    BuildInfo({TrapType::ALARM, "Alarm trap", ViewId::ALARM_TRAP}, TechId::TRAPS,
        "Summons all minions", 0, "Traps"),
    BuildInfo({TrapType::WEB, "Web trap", ViewId::WEB_TRAP}, TechId::TRAPS,
        "Immobilises the trespasser for some time.", 0, "Traps"),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}, TechId::TRAPS,
        "Causes a huge boulder to roll towards the enemy.", 0, "Traps"),
    BuildInfo({TrapType::SURPRISE, "Surprise trap", ViewId::SURPRISE_TRAP}, TechId::TRAPS,
        "Teleports nearby minions to deal with the trespasser.", 0, "Traps"),
  });
  return buildInfo;
}

vector<PlayerControl::BuildInfo> PlayerControl::workshopInfo {
};

vector<PlayerControl::BuildInfo> PlayerControl::libraryInfo {
  BuildInfo(BuildInfo::IMP, "", 'i'),
  BuildInfo({CreatureId::KRAKEN, {}, {ResourceId::MANA, 200}}, TechId::KRAKEN, "",
      "Place the kraken in an underground lake, in reach of passing enemies."),
};

vector<PlayerControl::BuildInfo> PlayerControl::minionsInfo {
};

vector<PlayerControl::RoomInfo> PlayerControl::getRoomInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : getBuildInfo(nullptr, nullptr))
    if (bInfo.buildType == BuildInfo::SQUARE)
      ret.push_back({bInfo.squareInfo.name, bInfo.help, bInfo.techId});
  return ret;
}

vector<PlayerControl::RoomInfo> PlayerControl::getWorkshopInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : workshopInfo)
    if (bInfo.buildType == BuildInfo::TRAP) {
      BuildInfo::TrapInfo info = bInfo.trapInfo;
      ret.push_back({info.name, bInfo.help, bInfo.techId});
    }
  return ret;
}

typedef Collective::Warning Warning;

string PlayerControl::getWarningText(Collective::Warning w) {
  switch (w) {
    case Warning::DIGGING: return "Start digging into the mountain to build a dungeon.";
    case Warning::STORAGE: return "You need to build a storage room.";
    case Warning::WOOD: return "Cut down some trees for wood.";
    case Warning::IRON: return "You need to mine more iron.";
    case Warning::STONE: return "You need to mine more stone.";
    case Warning::GOLD: return "You need to mine more gold.";
    case Warning::LIBRARY: return "Build a library to start research.";
    case Warning::BEDS: return "You need to build beds for your minions.";
    case Warning::TRAINING: return "Build a training room for your minions.";
    case Warning::WORKSHOP: return "Build a workshop to produce equipment and traps.";
    case Warning::LABORATORY: return "Build a laboratory to produce potions.";
    case Warning::NO_WEAPONS: return "You need weapons for your minions.";
    case Warning::GRAVES: return "You need a graveyard to collect corpses";
    case Warning::CHESTS: return "You need to build a treasure room.";
    case Warning::NO_PRISON: return "You need to build a prison.";
    case Warning::LARGER_PRISON: return "You need a larger prison.";
    case Warning::TORTURE_ROOM: return "You need to build a torture room.";
    case Warning::ALTAR: return "You need to build a shrine to sacrifice.";
    case Warning::MORE_CHESTS: return "You need a larger treasure room.";
    case Warning::MANA: return "Kill or torture some innocent beings for more mana.";
    case Warning::MORE_LIGHTS: return "Place more torches to light up your dungeon.";
  }
  return "";
}

static bool seeEverything = false;

PlayerControl::PlayerControl(Collective* col, Model* m, Level* level) : CollectiveControl(col), model(m) {
  bool hotkeys[128] = {0};
  for (BuildInfo info : concat(getBuildInfo(level, nullptr), workshopInfo)) {
    if (info.hotkey) {
      CHECK(!hotkeys[int(info.hotkey)]);
      hotkeys[int(info.hotkey)] = true;
    }
  }
  for (TechInfo info : getTechInfo()) {
    if (info.button.hotkey) {
      CHECK(!hotkeys[int(info.button.hotkey)]);
      hotkeys[int(info.button.hotkey)] = true;
    }
  }
  memory.reset(new map<UniqueEntity<Level>::Id, MapMemory>);
  for (Vec2 v : level->getBounds())
    if (contains({"gold ore", "iron ore", "stone"}, level->getSquare(v)->getName()))
      getMemory(level).addObject(v, level->getSquare(v)->getViewObject());
  for(const Location* loc : level->getAllLocations())
    if (loc->isMarkedAsSurprise())
      surprises.insert(loc->getBounds().middle());
  Options::addTrigger(OptionId::SHOW_MAP, [&] (bool val) { seeEverything = val; });
}

const int basicImpCost = 20;
const int minionLimit = 40;

void PlayerControl::unpossess() {
  if (possessed == getKeeper())
    lastControlKeeperQuestion = getCollective()->getTime();
  CHECK(possessed);
  if (possessed->isPlayer())
    possessed->popController();
  ViewObject::setHallu(false);
  possessed = nullptr;
 /* team.clear();
  gatheringTeam = false;*/
}

void PlayerControl::render(View* view) {
  if (retired)
    return;
  if (possessed && (!possessed->isPlayer() || possessed->isDead())) {
 /*   if (contains(team, possessed))
      removeElement(team, possessed);*/
    if ((possessed->isDead() || possessed->isAffected(LastingEffect::SLEEP)) && !team.empty()) {
      possess(team.front(), view);
    } else {
      view->setTimeMilli(possessed->getTime() * 300);
      view->clearMessages();
      if (possessed->getLevel() != getLevel())
        view->resetCenter();
      unpossess();
    }
  }
  if (!possessed) {
    view->updateView(this);
  } else
    view->stopClock();
  if (showWelcomeMsg && Options::getValue(OptionId::HINTS)) {
    view->updateView(this);
    showWelcomeMsg = false;
    view->presentText("Welcome", "In short: you are a warlock who has been banished from the lawful world for practicing black magic. You are going to claim the land of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext() + " and make it your domain. The best way to achieve this is to kill everyone.\n \n"
"Use the mouse to dig into the mountain. You can select rectangular areas using the shift key. You will need access to trees, iron and gold ore. Build rooms and traps and prepare for war. You can control a minion at any time by clicking on them in the minions tab or on the map.\n \n You can turn these messages off in the options (press F2).");
  }
}

bool PlayerControl::isTurnBased() {
  return retired || (possessed != nullptr && possessed->isPlayer());
}

void PlayerControl::retire() {
  if (possessed)
    unpossess();
  retired = true;
}

ViewObject PlayerControl::getResourceViewObject(ResourceId id) const {
  switch (id) {
    case ResourceId::MANA: return ViewObject::mana();
    default: return ItemFactory::fromId(Collective::resourceInfo.at(id).itemId)->getViewObject();
  }
}

static vector<ItemType> marketItems {
  {ItemId::POTION, EffectId::HEAL},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLEEP)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::BLIND)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::FLYING)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SLOWED)},
  {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::SPEED)},
  ItemId::WARNING_AMULET,
  ItemId::HEALING_AMULET,
  ItemId::DEFENSE_AMULET,
  ItemId::FRIENDLY_ANIMALS_AMULET,
  {ItemId::RING, LastingEffect::FIRE_RESISTANT},
  {ItemId::RING, LastingEffect::POISON_RESISTANT},
};

enum class PlayerControl::MinionOption { POSSESS, EQUIPMENT, INFO, WAKE_UP, PRISON, TORTURE, SACRIFICE, EXECUTE,
  LABOR, TRAINING, WORKSHOP, LAB, STUDY, WORSHIP, COPULATE, CONSUME };

struct TaskOption {
  MinionTask task;
  PlayerControl::MinionOption option;
  string description;
};

vector<TaskOption> taskOptions { 
  {MinionTask::TRAIN, PlayerControl::MinionOption::TRAINING, "Training"},
  {MinionTask::WORKSHOP, PlayerControl::MinionOption::WORKSHOP, "Forge"},
  {MinionTask::LABORATORY, PlayerControl::MinionOption::LAB, "Lab"},
  {MinionTask::STUDY, PlayerControl::MinionOption::STUDY, "Study"},
  {MinionTask::WORSHIP, PlayerControl::MinionOption::WORSHIP, "Worship"},
  {MinionTask::COPULATE, PlayerControl::MinionOption::COPULATE, "Copulate"},
  {MinionTask::CONSUME, PlayerControl::MinionOption::CONSUME, "Consume"},
};

static string getMoraleString(double morale) {
  return convertToString(morale);
 /* if (morale < -0.33)
    return "low";
  if (morale > 0.33)
    return "high";
  return "normal";*/
}

void PlayerControl::getMinionOptions(Creature* c, vector<MinionOption>& mOpt, vector<View::ListElem>& lOpt) {
  if (getCollective()->hasTrait(c, MinionTrait::PRISONER)) {
    mOpt = {MinionOption::PRISON, MinionOption::TORTURE, MinionOption::SACRIFICE, MinionOption::EXECUTE,
      MinionOption::LABOR};
    lOpt = {"Send to prison", "Torture", "Sacrifice", "Execute", "Labor"};
    return;
  }
  mOpt = {MinionOption::POSSESS, MinionOption::INFO };
  lOpt = {"Possess", "Description" };
  if (!getCollective()->hasTrait(c, MinionTrait::NO_EQUIPMENT)) {
    mOpt.push_back(MinionOption::EQUIPMENT);
    lOpt.push_back("Equipment");
  }
  lOpt.emplace_back("Order task:", View::TITLE);
  for (auto elem : taskOptions)
    if (c->getMinionTasks()[elem.task] > 0) {
      lOpt.push_back(elem.description);
      mOpt.push_back(elem.option);
    }
  if (c->isAffected(LastingEffect::SLEEP)) {
    mOpt.push_back(MinionOption::WAKE_UP);
    lOpt.push_back("Wake up");
  }
  lOpt.emplace_back("", View::INACTIVE);
  vector<Skill*> skills = c->getSkills();
  if (!skills.empty()) {
    lOpt.emplace_back("Skills:", View::TITLE);
    for (Skill* skill : skills)
      lOpt.emplace_back(skill->getName(), View::INACTIVE);
    lOpt.emplace_back("", View::INACTIVE);
  }
  vector<SpellInfo> spells = c->getSpells();
  if (!spells.empty()) {
    lOpt.emplace_back("Spells:", View::TITLE);
    for (SpellInfo elem : spells)
      lOpt.emplace_back(elem.name, View::INACTIVE);
    lOpt.emplace_back("", View::INACTIVE);
  }
  for (string s : c->getAdjectives())
    lOpt.emplace_back(s, View::INACTIVE);
}

Creature* PlayerControl::getConsumptionTarget(View* view, Creature* consumer) {
  vector<Creature*> res;
  vector<View::ListElem> opt;
  for (Creature* c : getCollective()->getConsumptionTargets(consumer)) {
    res.push_back(c);
    opt.emplace_back(c->getName() + ", level " + convertToString(c->getExpLevel()));
  }
  if (auto index = view->chooseFromList("Choose minion to consume:", opt))
    return res[*index];
  return nullptr;
}

void PlayerControl::minionView(View* view, Creature* creature, int prevIndex) {
  vector<MinionOption> mOpt;
  vector<View::ListElem> lOpt;
  getMinionOptions(creature, mOpt, lOpt);
  auto index = view->chooseFromList(capitalFirst(creature->getNameAndTitle()) 
      + ", level: " + convertToString(creature->getExpLevel()) + ", kills: "
      + convertToString(creature->getKills().size())
      + ", morale: " + getMoraleString(creature->getMorale()),
      lOpt, prevIndex, View::MINION_MENU);
  if (!index)
    return;
  switch (mOpt[*index]) {
    case MinionOption::POSSESS: possess(creature, view); return;
    case MinionOption::EQUIPMENT: handleEquipment(view, creature); break;
    case MinionOption::INFO: messageBuffer.addMessage(MessageBuffer::important(creature->getDescription())); break;
    case MinionOption::WAKE_UP: creature->removeEffect(LastingEffect::SLEEP); return;
    case MinionOption::PRISON:
      getCollective()->setMinionTask(creature, MinionTask::PRISON);
      getCollective()->setTrait(creature, MinionTrait::PRISONER);
      return;
    case MinionOption::TORTURE:
      getCollective()->setTrait(creature, MinionTrait::PRISONER);
      getCollective()->orderTorture(creature);
      return;
    case MinionOption::SACRIFICE:
      getCollective()->setTrait(creature, MinionTrait::PRISONER);
      getCollective()->orderSacrifice(creature);
      return;
    case MinionOption::EXECUTE:
      getCollective()->setTrait(creature, MinionTrait::PRISONER);
      getCollective()->orderExecution(creature);
      return;
    case MinionOption::LABOR:
      getCollective()->setTrait(creature, MinionTrait::WORKER);
      break;
    case MinionOption::CONSUME:
      if (Creature* target = getConsumptionTarget(view, creature))
        getCollective()->orderConsumption(creature, target);
      return;
    default:
      for (auto elem : taskOptions)
        if (mOpt[*index] == elem.option) {
          getCollective()->setMinionTask(creature, elem.task);
          creature->removeEffect(LastingEffect::SLEEP);
          return;
        }
      FAIL << "Unknown option " << int(mOpt[*index]);
      break;
  }
  minionView(view, creature, *index);
}

void PlayerControl::handleEquipment(View* view, Creature* creature, int prevItem) {
  if (!creature->isHumanoid()) {
    view->presentText("", creature->getTheName() + " can't use any equipment.");
    return;
  }
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  vector<View::ListElem> list;
  vector<Item*> ownedItems = getCollective()->getAllItems([this, creature](const Item* it) {
      return getCollective()->getMinionEquipment().getOwner(it) == creature; });
  vector<Item*> slotItems;
  vector<EquipmentSlot> slotIndex;
  for (auto slot : slots) {
    list.emplace_back(Equipment::slotTitles.at(slot), View::TITLE);
    vector<Item*> items;
    for (Item* it : ownedItems)
      if (it->canEquip() && it->getEquipmentSlot() == slot)
        items.push_back(it);
    for (int i = creature->getEquipment().getMaxItems(slot); i < items.size(); ++i)
      // a rare occurence that minion owns too many items of the same slot,
      //should happen only when an item leaves the fortress and then is braught back
      getCollective()->getMinionEquipment().discard(items[i]);
    append(slotItems, items);
    append(slotIndex, vector<EquipmentSlot>(items.size(), slot));
    for (Item* item : items) {
      removeElement(ownedItems, item);
      list.push_back(item->getNameAndModifiers() + (creature->getEquipment().isEquiped(item) 
            ? " (equiped)" : " (pending)"));
    }
    if (creature->getEquipment().getMaxItems(slot) > items.size()) {
      list.push_back("[Equip item]");
      slotIndex.push_back(slot);
      slotItems.push_back(nullptr);
    }
  }
  list.emplace_back(View::ListElem("Consumables", View::TITLE));
  vector<pair<string, vector<Item*>>> consumables = Item::stackItems(ownedItems,
      [&](const Item* it) { if (!creature->getEquipment().hasItem(it)) return " (pending)"; else return ""; } );
  for (auto elem : consumables)
    list.push_back(elem.first);
  list.push_back("[Add item]");
  Optional<int> newIndex = model->getView()->chooseFromList(creature->getName() + "'s equipment", list, prevItem);
  if (!newIndex)
    return;
  int index = *newIndex;
  if (index == slotItems.size() + consumables.size()) { // [Add item]
    int itIndex = 0;
    int scrollPos = 0;
    while (1) {
      const Item* chosenItem = chooseEquipmentItem(view, {}, [&](const Item* it) {
          return getCollective()->getMinionEquipment().getOwner(it) != creature && !it->canEquip()
              && getCollective()->getMinionEquipment().needs(creature, it, true); }, &itIndex, &scrollPos);
      if (chosenItem)
        getCollective()->getMinionEquipment().own(creature, chosenItem);
      else
        break;
    }
  } else
  if (index >= slotItems.size()) {  // discard a consumable item
    getCollective()->getMinionEquipment().discard(consumables[index - slotItems.size()].second[0]);
  } else
  if (Item* item = slotItems[index])  // discard equipment
    getCollective()->getMinionEquipment().discard(item);
  else { // add new equipment
    vector<Item*> currentItems = creature->getEquipment().getItem(slotIndex[index]);
    const Item* chosenItem = chooseEquipmentItem(view, currentItems, [&](const Item* it) {
        return getCollective()->getMinionEquipment().getOwner(it) != creature
            && creature->canEquipIfEmptySlot(it, nullptr) && it->getEquipmentSlot() == slotIndex[index]; });
    if (chosenItem) {
      if (Creature* c = const_cast<Creature*>(getCollective()->getMinionEquipment().getOwner(chosenItem)))
        c->removeEffect(LastingEffect::SLEEP);
      if (chosenItem->getEquipmentSlot() != EquipmentSlot::WEAPON
          || chosenItem->getMinStrength() <= creature->getAttr(AttrType::STRENGTH)
          || view->yesOrNoPrompt(chosenItem->getTheName() + " is too heavy for " + creature->getTheName() 
            + ", and will incur an accuracy penaulty.\n Do you want to continue?"))
        getCollective()->getMinionEquipment().own(creature, chosenItem);
    }
  }
  handleEquipment(view, creature, index);
}

Item* PlayerControl::chooseEquipmentItem(View* view, vector<Item*> currentItems, ItemPredicate predicate,
    int* prevIndex, int* scrollPos) const {
  vector<View::ListElem> options;
  vector<Item*> currentItemVec;
  if (!currentItems.empty())
    options.push_back(View::ListElem("Currently equiped: ", View::TITLE));
  for (Item* it : currentItems) {
    currentItemVec.push_back(it);
    options.push_back(it->getNameAndModifiers());
  }
  options.emplace_back("Free:", View::TITLE);
  vector<Item*> availableItems;
  vector<Item*> usedItems;
  for (Item* item : getCollective()->getAllItems(predicate))
    if (!contains(currentItems, item)) {
      if (getCollective()->getMinionEquipment().getOwner(item))
        usedItems.push_back(item);
      else
        availableItems.push_back(item);
    }
  if (currentItems.empty() && availableItems.empty() && usedItems.empty())
    return nullptr;
  vector<pair<string, vector<Item*>>> usedStacks = Item::stackItems(usedItems,
      [&](const Item* it) {
        const Creature* c = NOTNULL(getCollective()->getMinionEquipment().getOwner(it));
        return " owned by " + c->getAName() + " (level " + convertToString(c->getExpLevel()) + ")";});
  vector<Item*> allStacked;
  for (auto elem : concat(Item::stackItems(availableItems), usedStacks)) {
    if (!usedStacks.empty() && elem == usedStacks.front())
      options.emplace_back("Used:", View::TITLE);
    options.emplace_back(elem.first);
    allStacked.push_back(elem.second.front());
  }
  auto index = view->chooseFromList("Choose an item to equip: ", options, prevIndex ? *prevIndex : 0,
      View::NORMAL_MENU, scrollPos);
  if (!index)
    return nullptr;
  if (prevIndex)
    *prevIndex = *index;
  return concat(currentItemVec, allStacked)[*index];
}

void PlayerControl::handleMarket(View* view, int prevItem) {
  vector<Vec2> storage = getCollective()->getAllSquares(getCollective()->getEquipmentStorageSquares());
  if (storage.empty()) {
    view->presentText("Information", "You need a storage room to use the market.");
    return;
  }
  vector<View::ListElem> options;
  vector<PItem> items;
  for (ItemType id : marketItems) {
    items.push_back(ItemFactory::fromId(id));
    options.emplace_back(items.back()->getName() + "    $" + convertToString(items.back()->getPrice()),
        items.back()->getPrice() > getCollective()->numResource(ResourceId::GOLD) ? View::INACTIVE : View::NORMAL);
  }
  auto index = view->chooseFromList("Buy items", options, prevItem);
  if (!index)
    return;
  Vec2 dest = chooseRandom(storage);
  getCollective()->takeResource({ResourceId::GOLD, items[*index]->getPrice()});
  getLevel()->getSquare(dest)->dropItem(std::move(items[*index]));
  view->updateView(this);
  handleMarket(view, *index);
}

static string requires(TechId id) {
  return " (requires: " + Technology::get(id)->getName() + ")";
}

void PlayerControl::handlePersonalSpells(View* view) {
  vector<View::ListElem> options {
      View::ListElem("The Keeper can learn spells for use in combat and other situations. ", View::TITLE),
      View::ListElem("You can cast them with 's' when you are in control of the Keeper.", View::TITLE)};
  vector<SpellId> knownSpells = getCollective()->getAvailableSpells();
  for (auto elem : getCollective()->getAllSpells()) {
    SpellInfo spell = Creature::getSpell(elem);
    View::ElemMod mod = View::NORMAL;
    string suff;
    if (!contains(knownSpells, spell.id)) {
      mod = View::INACTIVE;
      suff = requires(getCollective()->getNeededTech(spell.id));
    }
    options.emplace_back(spell.name + suff, mod);
  }
  view->presentList("Sorcery", options);
}

int PlayerControl::getNumMinions() const {
  return getCollective()->getCreatures(MinionTrait::FIGHTER).size();
}

int PlayerControl::getMinLibrarySize() const {
  return 2 * getCollective()->getTechnologies().size();
}

void PlayerControl::handleLibrary(View* view) {
  if (getCollective()->getSquares(SquareId::LIBRARY).empty()) {
    view->presentText("", "You need to build a library to start research.");
    return;
  }
  vector<View::ListElem> options;
  bool allInactive = false;
  if (getCollective()->getSquares(SquareId::LIBRARY).size() <= getMinLibrarySize()) {
    allInactive = true;
    options.emplace_back("You need a larger library to continue research.", View::TITLE);
  }
  options.push_back(View::ListElem("You have " 
        + convertToString(int(getCollective()->numResource(ResourceId::MANA))) + " mana. ", View::TITLE));
  vector<Technology*> techs = filter(Technology::getNextTechs(getCollective()->getTechnologies()),
      [](const Technology* tech) { return tech->canResearch(); });
  for (Technology* tech : techs) {
    int cost = getCollective()->getTechCost(tech);
    string text = tech->getName() + " (" + convertToString(cost) + " mana)";
    options.emplace_back(text, cost <= int(getCollective()->numResource(ResourceId::MANA))
        && !allInactive ? View::NORMAL : View::INACTIVE);
  }
  options.emplace_back("Researched:", View::TITLE);
  for (Technology* tech : getCollective()->getTechnologies())
    options.emplace_back(tech->getName(), View::INACTIVE);
  auto index = view->chooseFromList("Library", options);
  if (!index)
    return;
  Technology* tech = techs[*index];
  getCollective()->takeResource({ResourceId::MANA, int(getCollective()->getTechCost(tech))});
  getCollective()->acquireTech(tech);
  view->updateView(this);
  handleLibrary(view);
}

typedef GameInfo::BandInfo::Button Button;

Optional<pair<ViewObject, int>> PlayerControl::getCostObj(CostInfo cost) const {
  if (cost.value() > 0 && !Collective::resourceInfo.at(cost.id()).dontDisplay)
    return make_pair(getResourceViewObject(cost.id()), cost.value());
  else
    return Nothing();
}

string PlayerControl::getMinionName(CreatureId id) const {
  static map<CreatureId, string> names;
  if (!names.count(id))
    names[id] = CreatureFactory::fromId(id, nullptr)->getName();
  return names.at(id);
}

ViewObject PlayerControl::getMinionViewObject(CreatureId id) const {
  static map<CreatureId, ViewObject> objs;
  if (!objs.count(id))
    objs[id] = CreatureFactory::fromId(id, nullptr)->getViewObject();
  return objs.at(id);
}

vector<Button> PlayerControl::fillButtons(const vector<BuildInfo>& buildInfo) const {
  vector<Button> buttons;
  for (BuildInfo button : buildInfo) {
    bool isTech = (!button.techId || getCollective()->hasTech(*button.techId));
    switch (button.buildType) {
      case BuildInfo::SQUARE: {
           BuildInfo::SquareInfo& elem = button.squareInfo;
           ViewObject viewObj(SquareFactory::get(elem.type)->getViewObject());
           string description;
           if (elem.cost.value() > 0)
             description = "[" + convertToString(getCollective()->getSquares(elem.type).size()) + "]";
           int availableNow = !elem.cost.value() ? 1 : getCollective()->numResource(elem.cost.id()) / elem.cost.value();
           if (Collective::resourceInfo.at(elem.cost.id()).dontDisplay && availableNow)
             description += " (" + convertToString(availableNow) + " available)";
           if (Collective::getSecondarySquare(elem.type))
             viewObj = SquareFactory::get(*Collective::getSecondarySquare(elem.type))->getViewObject();
           buttons.push_back({
               viewObj,
               elem.name,
               getCostObj(elem.cost),
               description,
               (elem.noCredit && !availableNow) ? "inactive" : isTech ? "" : "Requires " + Technology::get(*button.techId)->getName() });
           }
           break;
      case BuildInfo::DIG: {
             buttons.push_back({
                 ViewObject(ViewId::DIG_ICON, ViewLayer::LARGE_ITEM, ""),
                 "Dig or cut tree", Nothing(), "", ""});
           }
           break;
      case BuildInfo::FETCH: {
             buttons.push_back({
                 ViewObject(ViewId::FETCH_ICON, ViewLayer::LARGE_ITEM, ""),
                 "Fetch items", Nothing(), "", ""});
           }
           break;
      case BuildInfo::CLAIM_TILE: {
             buttons.push_back({
                 ViewObject(ViewId::KEEPER_FLOOR, ViewLayer::LARGE_ITEM, ""),
                 "Claim tile", Nothing(), "", ""});
           }
           break;
      case BuildInfo::DISPATCH: {
             buttons.push_back({
                 ViewObject(ViewId::IMP, ViewLayer::LARGE_ITEM, ""),
                 "Dispatch imp", Nothing(), "", ""});
           }
           break;
      case BuildInfo::TRAP: {
             BuildInfo::TrapInfo& elem = button.trapInfo;
             int numTraps = getCollective()->getTrapItems(elem.type).size();
             buttons.push_back({
                 ViewObject(elem.viewId, ViewLayer::LARGE_ITEM, ""),
                 elem.name,
                 Nothing(),
                 "(" + convertToString(numTraps) + " ready)",
                 isTech ? "" : "Requires " + Technology::get(*button.techId)->getName() });
           }
           break;
      case BuildInfo::IMP: {
           pair<ViewObject, int> cost = {ViewObject::mana(), getImpCost()};
           buttons.push_back({
               ViewObject(ViewId::IMP, ViewLayer::CREATURE, ""),
               "Summon imp",
               cost,
               "[" + convertToString(
                   getCollective()->getCreatures({MinionTrait::WORKER}, {MinionTrait::PRISONER}).size()) + "]",
               getImpCost() <= getCollective()->numResource(ResourceId::MANA) ? "" : "inactive"});
           break; }
      case BuildInfo::MINION: {
           BuildInfo::MinionInfo& elem = button.minionInfo;
           buttons.push_back({
               getMinionViewObject(elem.id),
               "Summon " + getMinionName(elem.id),
               getCostObj(elem.cost),
               "",
               !getCollective()->hasResource(elem.cost) ? "inactive" : isTech ? "" 
                  : "Requires " + Technology::get(*button.techId)->getName()});
           break; }
      case BuildInfo::DESTROY:
           buttons.push_back({
               ViewObject(ViewId::DESTROY_BUTTON, ViewLayer::CREATURE, ""), "Remove construction", Nothing(), "",
                   ""});
           break;
      case BuildInfo::GUARD_POST:
           buttons.push_back({
               ViewObject(ViewId::GUARD_POST, ViewLayer::CREATURE, ""), "Guard post", Nothing(), "", ""});
           break;
    }
    if (!isTech)
      buttons.back().help = "Requires " + Technology::get(*button.techId)->getName();
    if (buttons.back().help.empty())
      buttons.back().help = button.help;
    buttons.back().hotkey = button.hotkey;
    buttons.back().groupName = button.groupName;
  }
  return buttons;
}

vector<PlayerControl::TechInfo> PlayerControl::getTechInfo() const {
  vector<TechInfo> ret;
  ret.push_back({{ViewId::MANA, "Sorcery"}, [this](PlayerControl* c, View* view) {c->handlePersonalSpells(view);}});
  ret.push_back({{ViewId::EMPTY, ""}, [](PlayerControl* c, View*) {}});
  ret.push_back({{ViewId::LIBRARY, "Library", 'l'},
      [this](PlayerControl* c, View* view) { c->handleLibrary(view); }});
  ret.push_back({{ViewId::GOLD, "Black market"},
      [this](PlayerControl* c, View* view) { c->handleMarket(view); }});
  ret.push_back({{ViewId::EMPTY, ""}, [](PlayerControl* c, View*) {}});
  ret.push_back({{ViewId::BOOK, "Keeperopedia"},
      [this](PlayerControl* c, View* view) { model->keeperopedia.present(view); }});
  return ret;
}

void PlayerControl::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.bandInfo.deities.clear();
  for (Deity* deity : Deity::getDeities())
    gameInfo.bandInfo.deities.push_back({deity->getName(), getCollective()->getStanding(deity)});
  gameInfo.villageInfo.villages.clear();
  bool attacking = false;
  for (VillageControl* c : model->getVillageControls())
    if (!c->isAnonymous()) {
      gameInfo.villageInfo.villages.push_back(c->getVillageInfo());
      if (c->currentlyAttacking())
        attacking = true;
    }
  if (attacking)
    model->getView()->getJukebox()->setCurrent(Jukebox::BATTLE);
  Model::SunlightInfo sunlightInfo = model->getSunlightInfo();
  gameInfo.sunlightInfo = { sunlightInfo.getText(), (int)sunlightInfo.timeRemaining };
  gameInfo.infoType = GameInfo::InfoType::BAND;
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.buildings = fillButtons(getBuildInfo());
  info.workshop = fillButtons(workshopInfo);
  info.libraryButtons = fillButtons(libraryInfo);
  info.tasks = getCollective()->getMinionTaskStrings();
  info.creatures.clear();
  info.payoutTimeRemaining = getCollective()->getNextPayoutTime() - getCollective()->getTime();
  info.nextPayout = getCollective()->getNextSalaries();
  for (Creature* c : getCollective()->getCreaturesAnyOf(
        {MinionTrait::LEADER, MinionTrait::FIGHTER, MinionTrait::PRISONER})) {
    if (getCollective()->isInCombat(c))
      info.tasks[c->getUniqueId()] = "fighting";
    info.creatures.push_back({getCollective(), c});
  }
  info.monsterHeader = "Monsters: " + convertToString(getNumMinions()) + " / " + convertToString(minionLimit);
  info.enemies.clear();
  for (Vec2 v : getCollective()->getAllSquares())
    if (const Creature* c = getLevel()->getSquare(v)->getCreature())
      if (c->getTribe() != getTribe())
        info.enemies.push_back({nullptr, c});
  info.numResource.clear();
  for (auto elem : getCollective()->resourceInfo)
    if (!elem.second.dontDisplay)
      info.numResource.push_back(
          {getResourceViewObject(elem.first), getCollective()->numResource(elem.first), elem.second.name});
  if (attacking) {
    if (info.warning.empty())
      info.warning = NameGenerator::get(NameGeneratorId::INSULTS)->getNext();
  } else
    info.warning = "";
  for (Warning w : ENUM_ALL(Warning))
    if (getCollective()->isWarning(w)) {
      info.warning = getWarningText(w);
      break;
    }
  info.time = getCollective()->getTime();
  info.gatheringTeam = gatheringTeam;
  info.team.clear();
  for (Creature* c : team)
    info.team.push_back(c->getUniqueId());
  info.techButtons.clear();
  for (TechInfo tech : getTechInfo())
    info.techButtons.push_back(tech.button);
}

void PlayerControl::updateMemory() {
/*  for (Vec2 v : getLevel()->getBounds())
    if (getCollective()->isKnownSquare(v))
      addToMemory(v);*/
}

void PlayerControl::update(Creature* c) {
  if (contains(getCollective()->getCreatures(), c))
    for (Vec2 pos : getCollective()->getLevel()->getVisibleTiles(c)) {
      getCollective()->addKnownTile(pos);
      addToMemory(pos);
    }
}

const MapMemory& PlayerControl::getMemory() const {
  return (*memory.get())[getLevel()->getUniqueId()];
}

MapMemory& PlayerControl::getMemory(Level* l) {
  return (*memory.get())[l->getUniqueId()];
}

ViewObject PlayerControl::getTrapObject(TrapType type) {
  for (const PlayerControl::BuildInfo& info : concat(workshopInfo, getBuildInfo(nullptr, nullptr)))
    if (info.buildType == BuildInfo::TRAP && info.trapInfo.type == type)
      return ViewObject(info.trapInfo.viewId, ViewLayer::LARGE_ITEM, "Unarmed trap")
        .setModifier(ViewObject::Modifier::PLANNED);
  FAIL << "trap not found" << int(type);
  return ViewObject(ViewId::EMPTY, ViewLayer::LARGE_ITEM, "Unarmed trap");
}

static const ViewObject& getConstructionObject(SquareType type) {
  static unordered_map<SquareType, ViewObject> objects;
  if (!objects.count(type)) {
    objects.insert(make_pair(type, SquareFactory::get(type)->getViewObject()));
    objects.at(type).setModifier(ViewObject::Modifier::PLANNED);
  }
  return objects.at(type);
}

ViewIndex PlayerControl::getViewIndex(Vec2 pos) const {
  ViewIndex index = getLevel()->getSquare(pos)->getViewIndex(this);
  if (getCollective()->getAllSquares().count(pos) 
      && index.hasObject(ViewLayer::FLOOR_BACKGROUND)
      && index.getObject(ViewLayer::FLOOR_BACKGROUND).id() == ViewId::FLOOR)
    index.getObject(ViewLayer::FLOOR_BACKGROUND).setId(ViewId::KEEPER_FLOOR);
  if (const Creature* c = getLevel()->getSquare(pos)->getCreature())
    if (contains(team, c) && gatheringTeam)
      index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_HIGHLIGHT);
  if (getCollective()->isMarkedToDig(pos))
    index.setHighlight(HighlightType::BUILD);
  else if (rectSelectCorner && rectSelectCorner2
      && pos.inRectangle(Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2})))
    index.setHighlight(HighlightType::RECT_SELECTION);
  const map<Vec2, Collective::TrapInfo>& traps = getCollective()->getTraps();
  const map<Vec2, Collective::ConstructionInfo>& constructions = getCollective()->getConstructions();
  if (!index.hasObject(ViewLayer::LARGE_ITEM)) {
    if (traps.count(pos))
      index.insert(getTrapObject(traps.at(pos).type()));
    if (getCollective()->isGuardPost(pos))
      index.insert(ViewObject(ViewId::GUARD_POST, ViewLayer::LARGE_ITEM, "Guard post"));
    if (constructions.count(pos) && !constructions.at(pos).built())
      index.insert(getConstructionObject(constructions.at(pos).type()));
  }
  if (surprises.count(pos) && !getCollective()->isKnownSquare(pos))
    index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Surprise"));
  if (getCollective()->hasEfficiency(pos) && index.hasObject(ViewLayer::FLOOR))
    index.getObject(ViewLayer::FLOOR).setAttribute(
        ViewObject::Attribute::EFFICIENCY, getCollective()->getEfficiency(pos));
  return index;
}

bool PlayerControl::staticPosition() const {
  return false;
}

int PlayerControl::getMaxSightRange() const {
  return 100000;
}

Vec2 PlayerControl::getPosition() const {
  return getKeeper()->getPosition();
}

enum Selection { SELECT, DESELECT, NONE } selection = NONE;

int PlayerControl::getImpCost() const {
  int numImps = 0;
  for (Creature* c : getCollective()->getCreatures({MinionTrait::WORKER}, {MinionTrait::PRISONER}))
    ++numImps;
  if (numImps < startImpNum)
    return 0;
  return basicImpCost * pow(2, double(numImps - startImpNum) / 5);
}

class MinionController : public Player {
  public:
  MinionController(Creature* c, Model* m, map<UniqueEntity<Level>::Id, MapMemory>* memory)
    : Player(c, m, false, memory) {}

  virtual void onKilled(const Creature* attacker) override {
    if (model->getView()->yesOrNoPrompt("Would you like to see the last messages?"))
      messageBuffer.showHistory();
    creature->popController();
  }

  virtual bool unpossess() override {
    return true;
  }

  virtual void onFellAsleep() {
    if (model->getView()->yesOrNoPrompt("You fell asleep. Do you want to leave your minion?"))
      creature->popController();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Player);
  }

  SERIALIZATION_CONSTRUCTOR(MinionController);
};

void PlayerControl::possess(const Creature* cr, View* view) {
  if (possessed && possessed != cr && possessed->isPlayer())
    possessed->popController();
  view->stopClock();
  CHECK(contains(getCreatures(), cr));
  CHECK(!cr->isDead());
  Creature* c = const_cast<Creature*>(cr);
  if (c->isAffected(LastingEffect::SLEEP))
    c->removeEffect(LastingEffect::SLEEP);
  getCollective()->freeFromGuardPost(c);
  updateMemory();
  c->pushController(PController(new MinionController(c, model, memory.get())));
  possessed = c;
}

bool PlayerControl::canBuildDoor(Vec2 pos) const {
  if (!getLevel()->getSquare(pos)->canConstruct({SquareId::TRIBE_DOOR, getTribe()}))
    return false;
  Rectangle innerRect = getLevel()->getBounds().minusMargin(1);
  auto wallFun = [=](Vec2 pos) {
      return getLevel()->getSquare(pos)->canConstruct(SquareId::FLOOR) ||
          !pos.inRectangle(innerRect); };
  return !getCollective()->getTraps().count(pos) && pos.inRectangle(innerRect) && 
      ((wallFun(pos - Vec2(0, 1)) && wallFun(pos - Vec2(0, -1))) ||
       (wallFun(pos - Vec2(1, 0)) && wallFun(pos - Vec2(-1, 0))));
}

bool PlayerControl::canPlacePost(Vec2 pos) const {
  return !getCollective()->isGuardPost(pos) && !getCollective()->getTraps().count(pos) &&
      getLevel()->getSquare(pos)->canEnterEmpty({MovementTrait::WALK}) && getCollective()->isKnownSquare(pos);
}
  
Creature* PlayerControl::getCreature(UniqueEntity<Creature>::Id id) {
  for (Creature* c : getCreatures())
    if (c->getUniqueId() == id)
      return c;
  FAIL << "Creature not found " << id;
  return nullptr;
}

void PlayerControl::handleCreatureButton(Creature* c, View* view) {
  if (!gatheringTeam)
    minionView(view, c);
  else if (getCollective()->hasAnyTrait(c, {MinionTrait::FIGHTER, MinionTrait::LEADER})) {
    if (contains(team, c))
      removeElement(team, c);
    else
      team.push_back(c);
  }
}

void PlayerControl::processInput(View* view, UserInput input) {
  if (retired)
    return;
  switch (input.type) {
    case UserInput::EDIT_TEAM:
        CHECK(!team.empty());
        CHECK(!gatheringTeam);
        gatheringTeam = true;
      break;
    case UserInput::GATHER_TEAM:
        if (gatheringTeam && !team.empty()) {
          gatheringTeam = false;
          break;
        } 
        if (!gatheringTeam && !team.empty()) {
          possess(team[0], view);
 //         gatheringTeam = false;
          for (Creature* c : team) {
            getCollective()->freeFromGuardPost(c);
            if (c->isAffected(LastingEffect::SLEEP))
              c->removeEffect(LastingEffect::SLEEP);
          }
        } else
          gatheringTeam = true;
        break;
    case UserInput::DRAW_LEVEL_MAP: view->drawLevelMap(this); break;
    case UserInput::CANCEL_TEAM: gatheringTeam = false; team.clear(); break;
    case UserInput::MARKET: handleMarket(view); break;
    case UserInput::DEITIES: model->keeperopedia.deity(view, Deity::getDeities()[input.getNum()]); break;
    case UserInput::TECHNOLOGY: {
        vector<TechInfo> techInfo = getTechInfo();
        techInfo[input.getNum()].butFun(this, view);
        break;}
    case UserInput::CREATURE_BUTTON: 
        handleCreatureButton(getCreature(input.getNum()), view);
        break;
    case UserInput::POSSESS: {
        Vec2 pos = input.getPosition();
        if (!pos.inRectangle(getLevel()->getBounds()))
          return;
        if (Creature* c = getLevel()->getSquare(pos)->getCreature())
          if (getCollective()->hasAnyTrait(c, {MinionTrait::PRISONER, MinionTrait::FIGHTER, MinionTrait::LEADER}))
            handleCreatureButton(c, view);
        break;
        }
    case UserInput::RECT_SELECTION:
        if (rectSelectCorner) {
          rectSelectCorner2 = input.getPosition();
        } else
          rectSelectCorner = input.getPosition();
        break;
    case UserInput::BUILD:
        handleSelection(input.getPosition(), getBuildInfo()[input.getBuildInfo().building], false);
        break;
    case UserInput::WORKSHOP:
        handleSelection(input.getPosition(), workshopInfo[input.getBuildInfo().building], false);
        break;
    case UserInput::LIBRARY:
        handleSelection(input.getPosition(), libraryInfo[input.getBuildInfo().building], false);
        break;
    case UserInput::BUTTON_RELEASE:
        selection = NONE;
        if (rectSelectCorner && rectSelectCorner2) {
          handleSelection(*rectSelectCorner, getBuildInfo()[input.getBuildInfo().building], true);
          for (Vec2 v : Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2}))
            handleSelection(v, getBuildInfo()[input.getBuildInfo().building], true);
        }
        rectSelectCorner = Nothing();
        rectSelectCorner2 = Nothing();
        selection = NONE;
        break;
    case UserInput::EXIT: model->exitAction(); break;
    case UserInput::IDLE: break;
    default: break;
  }
}

void PlayerControl::handleSelection(Vec2 pos, const BuildInfo& building, bool rectangle) {
  if (building.techId && !getCollective()->hasTech(*building.techId))
    return;
  if (!pos.inRectangle(getLevel()->getBounds()))
    return;
  switch (building.buildType) {
    case BuildInfo::IMP:
        if (getCollective()->numResource(ResourceId::MANA) >= getImpCost() && selection == NONE && !rectangle) {
          selection = SELECT;
          PCreature imp = CreatureFactory::fromId(CreatureId::IMP, getTribe(),
              MonsterAIFactory::collective(getCollective()));
          for (Vec2 v : pos.neighbors8(true))
            if (v.inRectangle(getLevel()->getBounds()) && getLevel()->getSquare(v)->canEnter(imp.get()) 
                && canSee(v)) {
              getCollective()->takeResource({ResourceId::MANA, getImpCost()});
              getCollective()->addCreature(std::move(imp), v, {MinionTrait::WORKER});
              break;
            }
        }
        break;
    case BuildInfo::MINION:
        if (getCollective()->hasResource(building.minionInfo.cost) && selection == NONE && !rectangle) {
          const BuildInfo::MinionInfo& elem = building.minionInfo;
          selection = SELECT;
          PCreature creature = CreatureFactory::fromId(elem.id, getTribe(),
              MonsterAIFactory::collective(getCollective()));
          if (pos.inRectangle(getLevel()->getBounds()) && getLevel()->getSquare(pos)->canEnter(creature.get()) 
                && canSee(pos)) {
              getCollective()->takeResource(elem.cost);
              getCollective()->addCreature(std::move(creature), pos, elem.traits);
              break;
            }
        }
        break;
    case BuildInfo::TRAP:
        if (getCollective()->getTraps().count(pos)) {
          getCollective()->removeTrap(pos);// Does this mean I can remove the order if the trap physically exists?
        } else
        if (canPlacePost(pos) && getCollective()->containsSquare(pos)) {
          getCollective()->addTrap(pos, building.trapInfo.type);
        }
      break;
    case BuildInfo::DESTROY:
        selection = SELECT;
        if (getLevel()->getSquare(pos)->canDestroy() && getCollective()->containsSquare(pos))
          getLevel()->getSquare(pos)->destroy();
        getLevel()->getSquare(pos)->removeTriggers();
        if (Creature* c = getLevel()->getSquare(pos)->getCreature())
          if (c->getName() == "boulder")
            c->die(nullptr, false);
        if (getCollective()->getTraps().count(pos)) {
          getCollective()->removeTrap(pos);
        } else
        if (getCollective()->getConstructions().count(pos))
          getCollective()->removeConstruction(pos);
        break;
    case BuildInfo::GUARD_POST:
        if (getCollective()->isGuardPost(pos) && selection != SELECT) {
          getCollective()->removeGuardPost(pos);
          selection = DESELECT;
        }
        else if (canPlacePost(pos) && selection != DESELECT) {
          getCollective()->addGuardPost(pos);
          selection = SELECT;
        }
        break;
    case BuildInfo::DIG:
        if (!tryLockingDoor(pos)) {
          if (getCollective()->isMarkedToDig(pos) && selection != SELECT) {
            getCollective()->dontDig(pos);
            selection = DESELECT;
          } else
            if (!getCollective()->isMarkedToDig(pos) && selection != DESELECT) {
              if (getLevel()->getSquare(pos)->canConstruct(SquareId::TREE_TRUNK)) {
                getCollective()->cutTree(pos);
                selection = SELECT;
              } else
                if (getLevel()->getSquare(pos)->canConstruct(SquareId::FLOOR)
                    || !getCollective()->isKnownSquare(pos)) {
                  getCollective()->dig(pos);
                  selection = SELECT;
                }
            }
        }
        break;
    case BuildInfo::FETCH:
        getCollective()->fetchAllItems(pos);
        break;
    case BuildInfo::CLAIM_TILE:
        getCollective()->claimSquare(pos);
        break;
    case BuildInfo::DISPATCH:
        getCollective()->setPriorityTasks(pos);
        break;
    case BuildInfo::SQUARE:
        if (!tryLockingDoor(pos)) {
          if (getCollective()->getConstructions().count(pos)) {
            if (selection != SELECT) {
              getCollective()->removeConstruction(pos);
              selection = DESELECT;
            }
          } else {
            BuildInfo::SquareInfo info = building.squareInfo;
            if (getCollective()->isKnownSquare(pos) && getLevel()->getSquare(pos)->canConstruct(info.type) 
                && !getCollective()->getTraps().count(pos)
                && (info.type.getId() != SquareId::TRIBE_DOOR || canBuildDoor(pos)) && selection != DESELECT) {
              getCollective()->addConstruction(pos, info.type, info.cost, info.buildImmediatly, info.noCredit);
              selection = SELECT;
            }
          }
        }
        break;
  }
}

bool PlayerControl::tryLockingDoor(Vec2 pos) {
  if (getCollective()->getConstructions().count(pos)) {
    Square* square = getLevel()->getSquare(pos);
    if (square->canLock()) {
      if (selection != DESELECT && !square->isLocked()) {
        square->lock();
        selection = SELECT;
      } else if (selection != SELECT && square->isLocked()) {
        square->lock();
        selection = DESELECT;
      }
      getCollective()->updateSectors(pos);
      return true;
    }
  }
  return false;
}

bool PlayerControl::isRetired() const {
  return retired;
}

const Creature* PlayerControl::getKeeper() const {
  return getCollective()->getLeader();
}

Creature* PlayerControl::getKeeper() {
  return getCollective()->getLeader();
}

double PlayerControl::getWarLevel() const {
  double ret = 0;
  for (const Creature* c : getCollective()->getCreatures({MinionTrait::FIGHTER}))
    ret += c->getDifficultyPoints();
  ret += getCollective()->getSquares(SquareId::IMPALED_HEAD).size() * 150;
  return ret * getCollective()->getWarMultiplier();
}

void PlayerControl::addToMemory(Vec2 pos) {
  Square* square = getLevel()->getSquare(pos);
  if (!square->isDirty())
    return;
  square->setNonDirty();
  getMemory(getLevel()).update(pos, square->getViewIndex(this));
}

void PlayerControl::addDeityServant(Deity* deity, Vec2 deityPos, Vec2 victimPos) {
  PTask task = Task::chain(Task::destroySquare(victimPos), Task::disappear());
  PCreature creature = deity->getServant(Tribe::get(TribeId::KILL_EVERYONE)).random(
      MonsterAIFactory::singleTask(task));
  for (Vec2 v : concat({deityPos}, deityPos.neighbors8(true)))
    if (getLevel()->getSquare(v)->canEnter(creature.get())) {
      getLevel()->addCreature(v, std::move(creature));
      break;
    }
}

void PlayerControl::considerDeityFight() {
  vector<pair<Vec2, SquareType>> altars;
  vector<pair<Vec2, SquareType>> deityAltars;
  vector<SquareType> altarTypes;
  for (SquareType squareType : getCollective()->getSquareTypes()) {
    const set<Vec2>& squares = getCollective()->getSquares(squareType);
    if (!squares.empty() && contains({SquareId::ALTAR, SquareId::CREATURE_ALTAR}, squareType.getId())) {
      altarTypes.push_back(squareType);
      for (Vec2 v : squares) {
        altars.push_back({v, squareType});
        if (squareType.getId() == SquareId::ALTAR)
          deityAltars.push_back({v, squareType});
      }
    }
  }
  if (altarTypes.size() < 2 || deityAltars.size() == 0)
    return;
  double fightChance = 10000.0 / pow(5, altars.size() - 2);
  if (Random.rollD(fightChance)) {
    pair<Vec2, SquareType> attacking = chooseRandom(deityAltars);
    pair<Vec2, SquareType> victim;
    do {
      victim = chooseRandom(altars);
    } while (victim.second.getId() == SquareId::ALTAR &&
        victim.second == attacking.second);
    addDeityServant(Deity::getDeity(attacking.second.get<DeityHabitat>()), attacking.first, victim.first);
    if (victim.second.getId() == SquareId::ALTAR)
      addDeityServant(Deity::getDeity(victim.second.get<DeityHabitat>()), victim.first, attacking.first);
  }
}

void PlayerControl::checkKeeperDanger() {
  if (!retired && possessed != getKeeper()) { 
    if ((getCollective()->isInCombat(getKeeper()) || getKeeper()->getHealth() < 1)
        && lastControlKeeperQuestion < getCollective()->getTime() - 50) {
      lastControlKeeperQuestion = getCollective()->getTime();
      if (model->getView()->yesOrNoPrompt("The keeper is in trouble. Do you want to control him?")) {
        possess(getKeeper(), model->getView());
        return;
      }
    }
    if (getKeeper()->isAffected(LastingEffect::POISON)
        && lastControlKeeperQuestion < getCollective()->getTime() - 5) {
      lastControlKeeperQuestion = getCollective()->getTime();
      if (model->getView()->yesOrNoPrompt("The keeper is in trouble. Do you want to control him?")) {
        possess(getKeeper(), model->getView());
        return;
      }
    }
  }
}

void PlayerControl::tick(double time) {
  if (startImpNum == -1)
    startImpNum = getCollective()->getCreatures(MinionTrait::WORKER).size();
  considerDeityFight();
  checkKeeperDanger();
  updateMemory();
  model->getView()->getJukebox()->update();
  if (retired) {
    if (const Creature* c = getLevel()->getPlayer())
      if (Random.roll(30) && !getCollective()->containsSquare(c->getPosition()))
        c->playerMessage("You sense horrible evil in the " + 
            getCardinalName((getKeeper()->getPosition() - c->getPosition()).getBearing().getCardinalDir()));
  }
  updateVisibleCreatures();
  if (getCollective()->hasMinionDebt() && !retired && !payoutWarning) {
    model->getView()->presentText("Warning", "You don't have enough gold for salaries. "
        "Your minions will refuse to work if they are not paid.\n \n"
        "You can get more gold by mining or retrieve it from killed heroes and conquered villages.");
    payoutWarning = true;
  }
  for (const Creature* c1 : getVisibleFriends()) {
    Creature* c = const_cast<Creature*>(c1);
    if (c->getSpawnType() && !contains(getCreatures(), c))
      getCollective()->addCreature(c, {MinionTrait::FIGHTER});
  }
}

bool PlayerControl::canSee(const Creature* c) const {
  return canSee(c->getPosition());
}

bool PlayerControl::canSee(Vec2 position) const {
  if (seeEverything)
    return true;
  if (getCollective()->getAllSquares().count(position))
    return true;
  for (Creature* c : getCollective()->getCreatures())
    if (c->canSee(position))
      return true;
  for (Vec2 pos : getCollective()->getSquares(SquareId::EYEBALL))
    if (getLevel()->canSee(pos, position, Vision::get(VisionId::NORMAL)))
      return true;
  return false;
}

vector<const Creature*> PlayerControl::getUnknownAttacker() const {
  return {};
}

const Tribe* PlayerControl::getTribe() const {
  return getCollective()->getTribe();
}

Tribe* PlayerControl::getTribe() {
  return getCollective()->getTribe();
}

bool PlayerControl::isEnemy(const Creature* c) const {
  return getKeeper()->isEnemy(c);
}

void PlayerControl::onTechBookEvent(Technology* tech) {
  if (retired) {
    messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
        + ", but you do not comprehend it.");
    return;   
  }
  vector<Technology*> nextTechs = Technology::getNextTechs(getCollective()->getTechnologies());
  if (tech == nullptr) {
    if (!nextTechs.empty())
      tech = chooseRandom(nextTechs);
    else
      tech = chooseRandom(Technology::getAll());
  }
  if (!contains(getCollective()->getTechnologies(), tech)) {
    if (!contains(nextTechs, tech))
      messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
          + ", but you do not comprehend it.");
    else {
      messageBuffer.addImportantMessage("You have acquired the knowledge of " + tech->getName());
      getCollective()->acquireTech(tech, true);
    }
  } else {
    messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
        + ", which you already possess.");
  }
}

MoveInfo PlayerControl::getPossessedMove(Creature* c) {
  if (possessed->getLevel() == c->getLevel()) 
    if (auto action = c->moveTowards(possessed->getPosition()))
      return {1.0, action};
  return NoMove;
}

MoveInfo PlayerControl::getMove(Creature* c) {
  if (possessed && contains(team, c))
    return getPossessedMove(c);
  if (c == getKeeper() && !getCollective()->getAllSquares().empty()
      && getCollective()->getSquares(SquareId::LIBRARY).empty() // after library is built let him wander
      && !getCollective()->containsSquare(c->getPosition()))
    return c->moveTowards(chooseRandom(getCollective()->getAllSquares()));
  else
    return NoMove;
}

void PlayerControl::addKeeper(Creature* c) {
  getCollective()->addCreature(c, {MinionTrait::LEADER});
}

void PlayerControl::addImp(Creature* c) {
  getCollective()->addCreature(c, {MinionTrait::WORKER, MinionTrait::NO_EQUIPMENT});
}

void PlayerControl::onConqueredLand(const string& name) {
  if (retired)
    return;
  model->conquered(*getKeeper()->getFirstName() + " the Keeper", name, getCollective()->getKills(),
      getCollective()->getDangerLevel() + getCollective()->getPoints());
}

void PlayerControl::onCreatureKilled(const Creature* victim, const Creature* killer) {
  if (!getKeeper() && !retired) {
    model->gameOver(victim, getCollective()->getKills().size(), "enemies",
        getCollective()->getDangerLevel() + getCollective()->getPoints());
  }
  Creature* c = const_cast<Creature*>(victim);
  if (contains(team, c)) {
    removeElement(team, c);
    if (team.empty())
      gatheringTeam = false;
  }
}

const Level* PlayerControl::getViewLevel() const {
  return getLevel();
}

void PlayerControl::uncoverRandomLocation() {
  const Location* location = nullptr;
  for (auto loc : randomPermutation(getLevel()->getAllLocations()))
    if (!getCollective()->isKnownSquare(loc->getBounds().middle())) {
      location = loc;
      break;
    }
  double radius = 8.5;
  if (location)
    for (Vec2 v : location->getBounds().middle().circle(radius))
      if (getLevel()->inBounds(v))
        getCollective()->addKnownTile(v);
}

void PlayerControl::onWorshipEvent(Creature* who, const Deity* to, WorshipType type) {
  if (type == WorshipType::DESTROY_ALTAR) {
    model->getView()->presentText("", "A shrine to " + to->getName() + " has been devastated by " + who->getAName() + ".");
    return;
  }
  if (!contains(getCreatures(), who))
    return;
  for (EpithetId id : to->getEpithets())
    switch (id) {
      case EpithetId::DEATH:
        if (!who->isUndead() && Random.roll(500))
          who->makeUndead();
        if (type == WorshipType::SACRIFICE && Random.roll(10))
          getKeeper()->makeUndead();
        break;
      case EpithetId::SECRETS:
        if (Random.roll(200) || type == WorshipType::SACRIFICE)
          uncoverRandomLocation();
        break;
      default: break;
    }
}

void PlayerControl::onWorshipCreatureEvent(Creature* who, const Creature* to, WorshipType type) {
  if (type == WorshipType::DESTROY_ALTAR) {
    model->getView()->presentText("", "Shrine to " + to->getName() + " has been devastated by " + who->getAName());
    return;
  }
}

template <class Archive>
void PlayerControl::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, MinionController);
}

REGISTER_TYPES(PlayerControl);

