#include "stdafx.h"

#include "collective.h"
#include "level.h"
#include "task.h"
#include "player.h"
#include "message_buffer.h"
#include "model.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"

template <class Archive> 
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CreatureView)
    & SUBCLASS(EventListener)
    & SVAR(credit)
    & SVAR(technologies)
    & SVAR(numFreeTech)
    & SVAR(creatures)
    & SVAR(minions)
    & SVAR(minionByType)
    & SVAR(markedItems)
    & SVAR(taskMap)
    & SVAR(traps)
    & SVAR(constructions)
    & SVAR(minionTasks)
    & SVAR(minionTaskStrings)
    & SVAR(mySquares)
    & SVAR(myTiles)
    & SVAR(level)
    & SVAR(keeper)
    & SVAR(memory)
    & SVAR(knownTiles)
    & SVAR(gatheringTeam)
    & SVAR(team)
    & SVAR(teamLevelChanges)
    & SVAR(levelChangeHistory)
    & SVAR(possessed)
    & SVAR(minionEquipment)
    & SVAR(guardPosts)
    & SVAR(mana)
    & SVAR(points)
    & SVAR(model)
    & SVAR(kills)
    & SVAR(showWelcomeMsg)
    & SVAR(delayedPos)
    & SVAR(lastCombat)
    & SVAR(lastControlKeeperQuestion)
    & SVAR(startImpNum)
    & SVAR(retired)
    & SVAR(tribe)
    & SVAR(alarmInfo)
    & SVAR(prisonerInfo)
    & SVAR(executions);
  CHECK_SERIAL;
}

SERIALIZABLE(Collective);

template <class Archive>
void Collective::TaskMap::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(tasks)
    & BOOST_SERIALIZATION_NVP(marked)
    & BOOST_SERIALIZATION_NVP(taken)
    & BOOST_SERIALIZATION_NVP(taskMap)
    & BOOST_SERIALIZATION_NVP(lockedTasks)
    & BOOST_SERIALIZATION_NVP(completionCost);
}

SERIALIZABLE(Collective::TaskMap);

template <class Archive>
void Collective::AlarmInfo::serialize(Archive& ar, const unsigned int version) {
   ar& BOOST_SERIALIZATION_NVP(finishTime)
     & BOOST_SERIALIZATION_NVP(position);
}

SERIALIZABLE(Collective::AlarmInfo);

template <class Archive>
void Collective::TrapInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(armed)
    & BOOST_SERIALIZATION_NVP(marked);
}

SERIALIZABLE(Collective::TrapInfo);

template <class Archive>
void Collective::ConstructionInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(cost)
    & BOOST_SERIALIZATION_NVP(built)
    & BOOST_SERIALIZATION_NVP(marked)
    & BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(task);
}

SERIALIZABLE(Collective::ConstructionInfo);

template <class Archive>
void Collective::GuardPostInfo::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(attender);
}

SERIALIZABLE(Collective::GuardPostInfo);

template <class Archive>
void Collective::CostInfo::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(id) & BOOST_SERIALIZATION_NVP(value);
}

SERIALIZABLE(Collective::CostInfo);

template <class Archive>
void Collective::PrisonerInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(state)
    & BOOST_SERIALIZATION_NVP(attender);
}

SERIALIZABLE(Collective::PrisonerInfo);

Collective::BuildInfo::BuildInfo(SquareInfo info, Optional<TechId> id, const string& h, char key)
    : squareInfo(info), buildType(SQUARE), techId(id), help(h), hotkey(key) {}
Collective::BuildInfo::BuildInfo(TrapInfo info, Optional<TechId> id, const string& h, char key)
    : trapInfo(info), buildType(TRAP), techId(id), help(h), hotkey(key) {}
Collective::BuildInfo::BuildInfo(BuildType type, const string& h, char key) : buildType(type), help(h), hotkey(key) {
  CHECK(contains({DIG, IMP, GUARD_POST, DESTROY, FETCH}, type));
}
Collective::BuildInfo::BuildInfo(BuildType type, SquareInfo info, const string& h, char key) 
  : squareInfo(info), buildType(type), help(h), hotkey(key) {
  CHECK(type == IMPALED_HEAD);
}

const vector<Collective::BuildInfo> Collective::buildInfo {
    BuildInfo(BuildInfo::DIG, "", 'd'),
    BuildInfo({SquareType::STOCKPILE, {ResourceId::GOLD, 0}, "Storage", true}, Nothing(), ""),
    BuildInfo({SquareType::TREASURE_CHEST, {ResourceId::WOOD, 5}, "Treasure room"}, Nothing(), "", 'c'),
    BuildInfo({SquareType::BED, {ResourceId::WOOD, 10}, "Bed"}, Nothing(), "", 'b'),
    BuildInfo({SquareType::TRAINING_DUMMY, {ResourceId::IRON, 20}, "Training room"}, Nothing(), "", 't'),
    BuildInfo({SquareType::LIBRARY, {ResourceId::WOOD, 20}, "Library"}, Nothing(), "", 'y'),
    BuildInfo({SquareType::LABORATORY, {ResourceId::STONE, 15}, "Laboratory"}, TechId::ALCHEMY, "", 'r'),
    BuildInfo({SquareType::WORKSHOP, {ResourceId::IRON, 15}, "Workshop"}, TechId::CRAFTING, "", 'w'),
    BuildInfo({SquareType::ANIMAL_TRAP, {ResourceId::WOOD, 12}, "Beast cage"}, Nothing(),
        "Place it in the forest.", 'a'),
    BuildInfo({SquareType::GRAVE, {ResourceId::STONE, 20}, "Graveyard"}, Nothing(), "", 'v'),
    BuildInfo({SquareType::PRISON, {ResourceId::IRON, 20}, "Prison"}, Nothing(), "", 'p'),
    BuildInfo({SquareType::TORTURE_TABLE, {ResourceId::IRON, 20}, "Torture room"}, Nothing(), "", 'u'),
    BuildInfo(BuildInfo::DESTROY, "", 'e'),
    BuildInfo(BuildInfo::IMP, "", 'i'),
    BuildInfo(BuildInfo::GUARD_POST, "Place it anywhere to send a minion.", 'g'),
    BuildInfo(BuildInfo::FETCH, "Order imps to fetch items from outside the dungeon.", 'f'),
};

const vector<Collective::BuildInfo> Collective::workshopInfo {
    BuildInfo({SquareType::TRIBE_DOOR, {ResourceId::WOOD, 5}, "Door"}, TechId::CRAFTING, "", 'o'),
    BuildInfo({TrapType::ALARM, "Alarm trap", ViewId::ALARM_TRAP}, TechId::TRAPS,
        "Summons all minions"),
    BuildInfo({TrapType::WEB, "Web trap", ViewId::WEB_TRAP}, TechId::TRAPS,
        "Immobilises the trespasser for some time."),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}, TechId::TRAPS,
        "Releases a cloud of poisonous gas."),
    BuildInfo({TrapType::TERROR, "Terror trap", ViewId::TERROR_TRAP}, TechId::TRAPS,
        "Causes the trespasser to panic."),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}, TechId::TRAPS,
        "Causes a huge boulder to roll towards the enemy."),
    BuildInfo({TrapType::SURPRISE, "Surprise trap", ViewId::SURPRISE_TRAP}, TechId::TRAPS,
        "Teleports nearby minions to deal with the trespasser."),
    BuildInfo(BuildInfo::IMPALED_HEAD, {SquareType::IMPALED_HEAD, {ResourceId::GOLD, 0}, "Prisoner head"},
        "Impaled head of an executed prisoner. Aggravates enemies."),
};

vector<Collective::RoomInfo> Collective::getRoomInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : buildInfo)
    if (bInfo.buildType == BuildInfo::SQUARE) {
      BuildInfo::SquareInfo info = bInfo.squareInfo;
      ret.push_back({info.name, bInfo.help, bInfo.techId});
    }
  return ret;
}

vector<Collective::RoomInfo> Collective::getWorkshopInfo() {
  vector<RoomInfo> ret;
  for (BuildInfo bInfo : workshopInfo)
    if (bInfo.buildType == BuildInfo::TRAP) {
      BuildInfo::TrapInfo info = bInfo.trapInfo;
      ret.push_back({info.name, bInfo.help, bInfo.techId});
    }
    else if (bInfo.buildType == BuildInfo::IMPALED_HEAD)
      ret.push_back({"Prisoner head", bInfo.help, Nothing()});
  return ret;
}

vector<MinionType> minionTypes {
  MinionType::KEEPER,
  MinionType::IMP,
  MinionType::NORMAL,
  MinionType::UNDEAD,
  MinionType::GOLEM,
  MinionType::BEAST,
  MinionType::PRISONER,
};

Collective::ResourceInfo info;

constexpr const char* const Collective::warningText[numWarnings];

const map<Collective::ResourceId, Collective::ResourceInfo> Collective::resourceInfo {
  {ResourceId::GOLD, { SquareType::TREASURE_CHEST, Item::typePredicate(ItemType::GOLD), ItemId::GOLD_PIECE, "gold",
                       Collective::Warning::GOLD}},
  {ResourceId::WOOD, { SquareType::STOCKPILE, Item::namePredicate("wood plank"), ItemId::WOOD_PLANK, "wood",
                       Collective::Warning::WOOD}},
  {ResourceId::IRON, { SquareType::STOCKPILE, Item::namePredicate("iron ore"), ItemId::IRON_ORE, "iron",
                       Collective::Warning::IRON}},
  {ResourceId::STONE, { SquareType::STOCKPILE, Item::namePredicate("rock"), ItemId::ROCK, "stone",
                       Collective::Warning::STONE}},
};

vector<Collective::ItemFetchInfo> Collective::getFetchInfo() const {
  return {
    {unMarkedItems(ItemType::CORPSE), SquareType::GRAVE, true, {}, Warning::GRAVES},
    {unMarkedItems(ItemType::GOLD), SquareType::TREASURE_CHEST, false, {}, Warning::CHESTS},
    {[this](const Item* it) {
        return minionEquipment.isItemUseful(it) && !isItemMarked(it);
      }, SquareType::STOCKPILE, false, {}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getName() == "wood plank" && !isItemMarked(it); },
      SquareType::STOCKPILE, false, {SquareType::TREE_TRUNK}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getName() == "iron ore" && !isItemMarked(it); },
      SquareType::STOCKPILE, false, {}, Warning::STORAGE},
    {[this](const Item* it) {
        return it->getName() == "rock" && !isItemMarked(it); },
      SquareType::STOCKPILE, false, {}, Warning::STORAGE},
  };
}

struct MinionTaskInfo {
  SquareType square;
  string desc;
  Collective::Warning warning;
};
map<MinionTask, MinionTaskInfo> taskInfo {
    {MinionTask::LABORATORY, {SquareType::LABORATORY, "lab", Collective::Warning::LABORATORY}},
    {MinionTask::TRAIN, {SquareType::TRAINING_DUMMY, "training", Collective::Warning::TRAINING}},
    {MinionTask::WORKSHOP, {SquareType::WORKSHOP, "crafting", Collective::Warning::WORKSHOP}},
    {MinionTask::SLEEP, {SquareType::BED, "sleeping", Collective::Warning::BEDS}},
    {MinionTask::GRAVE, {SquareType::GRAVE, "sleeping", Collective::Warning::GRAVES}},
    {MinionTask::STUDY, {SquareType::LIBRARY, "studying", Collective::Warning::LIBRARY}},
    {MinionTask::PRISON, {SquareType::PRISON, "prison", Collective::Warning::NO_PRISON}},
    {MinionTask::TORTURE, {SquareType::TORTURE_TABLE, "tortured", Collective::Warning::TORTURE_ROOM}},
};

Collective::Collective(Model* m, Tribe* t) : mana(200), model(m), tribe(t) {
  bool hotkeys[128] = {0};
  for (BuildInfo info : concat(buildInfo, workshopInfo)) {
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
  memory.reset(new map<const Level*, MapMemory>);
  // init the map so the values can be safely read with .at()
  mySquares[SquareType::TREE_TRUNK].clear();
  mySquares[SquareType::IMPALED_HEAD].clear();
  mySquares[SquareType::FLOOR].clear();
  mySquares[SquareType::TRIBE_DOOR].clear();
  for (BuildInfo info : buildInfo)
    if (info.buildType == BuildInfo::SQUARE)
      mySquares[info.squareInfo.type].clear();
  credit = {
    {ResourceId::GOLD, 0},
    {ResourceId::WOOD, 0},
    {ResourceId::IRON, 0},
    {ResourceId::STONE, 0},
  };
  for (MinionType t : minionTypes)
    minionByType[t].clear();
}


const int basicImpCost = 20;
const int minionLimit = 40;

void Collective::unpossess() {
  CHECK(possessed);
  if (possessed->isPlayer())
    possessed->popController();
  ViewObject::setHallu(false);
  possessed = nullptr;
 /* team.clear();
  gatheringTeam = false;*/
  teamLevelChanges.clear();
}

void Collective::render(View* view) {
  if (retired)
    return;
  if (possessed && (!possessed->isPlayer() || possessed->isDead())) {
 /*   if (contains(team, possessed))
      removeElement(team, possessed);*/
    if ((possessed->isDead() || possessed->isAffected(Creature::SLEEP)) && !team.empty()) {
      possess(team.front(), view);
    } else {
      view->setTimeMilli(possessed->getTime() * 300);
      view->clearMessages();
      unpossess();
    }
  }
  if (!possessed) {
    view->refreshView(this);
  } else
    view->stopClock();
  if (showWelcomeMsg && Options::getValue(OptionId::HINTS)) {
    view->refreshView(this);
    showWelcomeMsg = false;
    view->presentText("Welcome", "In short: you are a warlock who has been banished from the lawful world for practicing black magic. You are going to claim the land of " + NameGenerator::worldNames.getNext() + " and make it your domain. Best way to achieve this is to kill everyone.\n \n"
"Use the mouse to dig into the mountain. You can select rectangular areas using the shift key. You will need access to trees, iron and gold ore. Build rooms and traps and prepare for war. You can control a minion at any time by clicking on them in the minions tab or on the map.\n \n You can turn these messages off in the options (press F2).");
  }
}

bool Collective::isTurnBased() {
  return retired || (possessed != nullptr && possessed->isPlayer());
}

void Collective::retire() {
  if (possessed)
    unpossess();
  retired = true;
}

vector<pair<Item*, Vec2>> Collective::getTrapItems(TrapType type, set<Vec2> squares) const {
  vector<pair<Item*, Vec2>> ret;
  if (squares.empty())
    squares = mySquares.at(SquareType::WORKSHOP);
  for (Vec2 pos : squares) {
    vector<Item*> v = level->getSquare(pos)->getItems([type, this](Item* it) {
        return it->getTrapType() == type && !isItemMarked(it); });
    for (Item* it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}

ViewObject Collective::getResourceViewObject(ResourceId id) const {
  return ItemFactory::fromId(resourceInfo.at(id).itemId)->getViewObject();
}

static vector<ItemId> marketItems {
  ItemId::HEALING_POTION,
  ItemId::SLEEP_POTION,
  ItemId::BLINDNESS_POTION,
  ItemId::INVISIBLE_POTION,
  ItemId::POISON_POTION,
  ItemId::SLOW_POTION,
  ItemId::SPEED_POTION,
  ItemId::WARNING_AMULET,
  ItemId::HEALING_AMULET,
  ItemId::DEFENSE_AMULET,
  ItemId::FRIENDLY_ANIMALS_AMULET,
};

MinionType Collective::getMinionType(const Creature* c) const {
  for (auto elem : minionTypes)
    if (contains(minionByType.at(elem), c))
      return elem;
  FAIL << "Minion type not found " << c->getName();
  return MinionType(0);
}

void Collective::setMinionType(Creature* c, MinionType type) {
  removeElement(minionByType.at(getMinionType(c)), c);
  minionByType.at(type).push_back(c);
  if (!contains({MinionType::IMP, MinionType::BEAST}, type))
    minionTasks.at(c->getUniqueId()) = getTasksForMinion(c);
}

struct TaskOption {
  MinionTask task;
  Collective::MinionOption option;
  string description;
};

vector<TaskOption> taskOptions { 
  {MinionTask::TRAIN, Collective::MinionOption::TRAINING, "Training"},
  {MinionTask::WORKSHOP, Collective::MinionOption::WORKSHOP, "Workshop"},
  {MinionTask::LABORATORY, Collective::MinionOption::LAB, "Lab"},
};

void Collective::getMinionOptions(Creature* c, vector<MinionOption>& mOpt, vector<View::ListElem>& lOpt) {
  switch (getMinionType(c)) {
    case MinionType::IMP:
      mOpt = {MinionOption::PRISON, MinionOption::TORTURE, MinionOption::EXECUTE};
      lOpt = {"Send to prison", "Torture", "Execute"};
      break;
    case MinionType::PRISONER:
      switch (prisonerInfo.at(c).state) {
        case PrisonerInfo::EXECUTE:
          lOpt = {View::ListElem("Execution ordered", View::TITLE)};
          break;
        case PrisonerInfo::PRISON:
          mOpt = {MinionOption::PRISON, MinionOption::TORTURE, MinionOption::EXECUTE, MinionOption::LABOR };
          lOpt = {"Send to prison", "Torture", "Execute", "Send to labor" };
          break;
        case PrisonerInfo::SURRENDER: FAIL << "state not handled: " << int(prisonerInfo.at(c).state);
      }
      break;
    case MinionType::BEAST:
      mOpt = {MinionOption::POSSESS, MinionOption::INFO };
      lOpt = {"Possess", "Description" };
      break;
    default:
      mOpt = {MinionOption::POSSESS, MinionOption::EQUIPMENT, MinionOption::INFO };
      lOpt = {"Possess", "Equipment", "Description" };
      if (c != keeper) {
        lOpt.emplace_back("Order task:", View::TITLE);
        for (auto elem : taskOptions)
          if (minionTasks.at(c->getUniqueId()).containsState(elem.task)) {
            lOpt.push_back(elem.description);
            mOpt.push_back(elem.option);
          }
      }
      if (c->isAffected(Creature::SLEEP)) {
        mOpt.push_back(MinionOption::WAKE_UP);
        lOpt.push_back("Wake up");
      }
      break;
  }
}

void Collective::setMinionTask(Creature* c, MinionTask task) {
  minionTasks.at(c->getUniqueId()).setState(task);
}

MinionTask Collective::getMinionTask(Creature* c) const {
  return minionTasks.at(c->getUniqueId()).getState();
}

void Collective::minionView(View* view, Creature* creature, int prevIndex) {
  vector<MinionOption> mOpt;
  vector<View::ListElem> lOpt;
  getMinionOptions(creature, mOpt, lOpt);
  auto index = view->chooseFromList(capitalFirst(creature->getNameAndTitle()) 
      + ", level: " + convertToString(creature->getExpLevel()) + ", kills: "
      + convertToString(creature->getKills().size()), lOpt, prevIndex);
  if (!index)
    return;
  switch (mOpt[*index]) {
    case MinionOption::POSSESS: possess(creature, view); return;
    case MinionOption::EQUIPMENT: handleEquipment(view, creature); break;
    case MinionOption::INFO: messageBuffer.addMessage(MessageBuffer::important(creature->getDescription())); break;
    case MinionOption::WAKE_UP: creature->removeEffect(Creature::SLEEP); return;
    case MinionOption::PRISON:
      setMinionTask(creature, MinionTask::PRISON);
      setMinionType(creature, MinionType::PRISONER);
      return;
    case MinionOption::TORTURE:
      setMinionType(creature, MinionType::PRISONER);
      setMinionTask(creature, MinionTask::TORTURE);
      return;
    case MinionOption::EXECUTE:
      setMinionType(creature, MinionType::PRISONER);
      prisonerInfo.at(creature) = {PrisonerInfo::EXECUTE, nullptr};
      minionTaskStrings[creature->getUniqueId()] = "execution";
      break;
    case MinionOption::LABOR:
      setMinionType(creature, MinionType::IMP);
      minionTaskStrings[creature->getUniqueId()] = "labor";
      break;
    default:
      for (auto elem : taskOptions)
        if (mOpt[*index] == elem.option) {
          setMinionTask(creature, elem.task);
          return;
        }
      FAIL << "Unknown option " << int(mOpt[*index]);
      break;
  }
  minionView(view, creature, *index);
}

void Collective::autoEquipment(Creature* creature) {
  if (!creature->isHumanoid() || getMinionType(creature) == MinionType::PRISONER || creature->getName() == "gnome")
    return;
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  vector<Item*> myItems = getAllItems([&](const Item* it) {
      return minionEquipment.getOwner(it) == creature && it->canEquip(); });
  for (Item* it : myItems) {
    if (contains(slots, it->getEquipmentSlot()))
      removeElement(slots, it->getEquipmentSlot());
    else  // a rare occurence that minion owns 2 items of the same slot,
          //should happen only when an item leaves the fortress and then is braught back
      minionEquipment.discard(it);
  }
  for (Item* it : getAllItems([&](const Item* it) {
      return minionEquipment.canTakeItem(creature, it); }, false)) {
    if (!it->canEquip() || contains(slots, it->getEquipmentSlot())) {
      minionEquipment.own(creature, it);
      if (it->canEquip())
        removeElement(slots, it->getEquipmentSlot());
      break;
    }
  }
}

void Collective::handleEquipment(View* view, Creature* creature, int prevItem) {
  if (!creature->isHumanoid()) {
    view->presentText("", creature->getTheName() + " can't use any equipment.");
    return;
  }
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  vector<View::ListElem> list;
  vector<Item*> ownedItems = getAllItems([this, creature](const Item* it) {
      return minionEquipment.getOwner(it) == creature; });
  vector<Item*> slotItems;
  for (auto slot : slots) {
    list.emplace_back(Equipment::slotTitles.at(slot), View::TITLE);
    Item* item = nullptr;
    for (Item* it : ownedItems)
      if (it->canEquip() && it->getEquipmentSlot() == slot) {
        if (item) // a rare occurence that minion owns 2 items of the same slot,
                  //should happen only when an item leaves the fortress and then is braught back
          minionEquipment.discard(it);
        else
          item = it;
      }
    slotItems.push_back(item);
    if (item) {
      removeElement(ownedItems, item);
      list.push_back(item->getNameAndModifiers() + (creature->getEquipment().isEquiped(item) 
            ? " (equiped)" : " (pending)"));
    } else
      list.push_back("[Nothing]");
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
    while (1) {
      const Item* chosenItem = chooseEquipmentItem(view, nullptr, [&](const Item* it) {
          return minionEquipment.getOwner(it) != creature && !it->canEquip()
              && minionEquipment.needs(creature, it); }, &itIndex);
      if (chosenItem)
        minionEquipment.own(creature, chosenItem);
      else break;
    }
  } else
  if (index >= slotItems.size()) {  // a consumable item
    minionEquipment.discard(consumables[index - slotItems.size()].second[0]);
  } else
  if (Item* item = slotItems[index])
    minionEquipment.discard(item);
  else {
    Item* currentItem = creature->getEquipment().getItem(slots[index]);
    const Item* chosenItem = chooseEquipmentItem(view, currentItem, [&](const Item* it) {
        return minionEquipment.getOwner(it) != creature
            && creature->canEquipIfEmptySlot(it) && it->getEquipmentSlot() == slots[index]; });
    if (chosenItem)
      minionEquipment.own(creature, chosenItem);
  }
  handleEquipment(view, creature, index);
}

vector<Item*> Collective::getAllItems(ItemPredicate predicate, bool includeMinions) const {
  vector<Item*> allItems;
  for (Vec2 v : myTiles)
    append(allItems, level->getSquare(v)->getItems(predicate));
  if (includeMinions)
    for (Creature* c : creatures)
      append(allItems, c->getEquipment().getItems(predicate));
  return allItems;
}

Item* Collective::chooseEquipmentItem(View* view, Item* currentItem, ItemPredicate predicate, int* prevIndex) const {
  vector<View::ListElem> options;
  vector<Item*> currentItemVec;
  if (currentItem) {
    currentItemVec.push_back(currentItem);
    options.push_back(View::ListElem("Currently equiped: ", View::TITLE));
    options.push_back(currentItem->getNameAndModifiers());
  }
  options.emplace_back("Free:", View::TITLE);
  vector<Item*> availableItems;
  vector<Item*> usedItems;
  for (Item* item : getAllItems(predicate))
    if (item != currentItem) {
      if (minionEquipment.getOwner(item))
        usedItems.push_back(item);
      else
        availableItems.push_back(item);
    }
  if (!currentItem && availableItems.empty() && usedItems.empty())
    return nullptr;
  vector<pair<string, vector<Item*>>> usedStacks = Item::stackItems(usedItems,
      [&](const Item* it) {
        const Creature* c = NOTNULL(minionEquipment.getOwner(it));
        return " owned by " + c->getAName() + " (level " + convertToString(c->getExpLevel()) + ")";});
  vector<Item*> allStacked;
  for (auto elem : concat(Item::stackItems(availableItems), usedStacks)) {
    if (!usedStacks.empty() && elem == usedStacks.front())
      options.emplace_back("Used:", View::TITLE);
    options.emplace_back(elem.first);
    allStacked.push_back(elem.second.front());
  }
  auto index = view->chooseFromList("Choose an item to equip: ", options, prevIndex ? *prevIndex : 0);
  if (!index)
    return nullptr;
  if (prevIndex)
    *prevIndex = *index;
  return concat(currentItemVec, allStacked)[*index];
}

void Collective::handleMarket(View* view, int prevItem) {
  if (mySquares[SquareType::STOCKPILE].empty()) {
    view->presentText("Information", "You need a storage room to use the market.");
    return;
  }
  vector<View::ListElem> options;
  vector<PItem> items;
  for (ItemId id : marketItems) {
    items.push_back(ItemFactory::fromId(id));
    options.emplace_back(items.back()->getName() + "    $" + convertToString(items.back()->getPrice()),
        items.back()->getPrice() > numGold(ResourceId::GOLD) ? View::INACTIVE : View::NORMAL);
  }
  auto index = view->chooseFromList("Buy items", options, prevItem);
  if (!index)
    return;
  Vec2 dest = chooseRandom(mySquares[SquareType::STOCKPILE]);
  takeGold({ResourceId::GOLD, items[*index]->getPrice()});
  level->getSquare(dest)->dropItem(std::move(items[*index]));
  view->updateView(this);
  handleMarket(view, *index);
}

struct SpellLearningInfo {
  SpellId id;
  TechId techId;
};

vector<SpellLearningInfo> spellLearning {
    { SpellId::HEALING, TechId::SPELLS },
    { SpellId::SUMMON_INSECTS, TechId::SPELLS},
    { SpellId::DECEPTION, TechId::SPELLS},
    { SpellId::SPEED_SELF, TechId::SPELLS_ADV},
    { SpellId::STR_BONUS, TechId::SPELLS_ADV},
    { SpellId::DEX_BONUS, TechId::SPELLS_ADV},
    { SpellId::FIRE_SPHERE_PET, TechId::SPELLS_ADV},
    { SpellId::TELEPORT, TechId::SPELLS_MAS},
    { SpellId::INVISIBILITY, TechId::SPELLS_MAS},
    { SpellId::WORD_OF_POWER, TechId::SPELLS_MAS},
};

vector<SpellInfo> Collective::getSpellLearning(const Technology* tech) {
  vector<SpellInfo> ret;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      ret.push_back(Creature::getSpell(elem.id));
  return ret;
}

static string requires(TechId id) {
  return " (requires: " + Technology::get(id)->getName() + ")";
}

void Collective::handlePersonalSpells(View* view) {
  vector<View::ListElem> options {
      View::ListElem("The Keeper can learn spells for use in combat and other situations. ", View::TITLE),
      View::ListElem("You can cast them with 's' when you are in control of the Keeper.", View::TITLE)};
  vector<SpellId> knownSpells;
  for (SpellInfo spell : keeper->getSpells())
    knownSpells.push_back(spell.id);
  for (auto elem : spellLearning) {
    SpellInfo spell = Creature::getSpell(elem.id);
    View::ElemMod mod = View::NORMAL;
    string suff;
    if (!contains(knownSpells, spell.id)) {
      mod = View::INACTIVE;
      suff = requires(elem.techId);
    }
    options.emplace_back(spell.name + suff, mod);
  }
  view->presentList("Sorcery", options);
}

vector<Collective::SpawnInfo> raisingInfo {
  {CreatureId::ZOMBIE, 30, TechId::NECRO},
  {CreatureId::MUMMY, 50, TechId::NECRO},
  {CreatureId::VAMPIRE, 50, TechId::VAMPIRE},
  {CreatureId::VAMPIRE_LORD, 100, TechId::VAMPIRE_ADV},
};

vector<Collective::SpawnInfo> animationInfo {
  {CreatureId::STONE_GOLEM, 50, TechId::GOLEM},
  {CreatureId::IRON_GOLEM, 50, TechId::GOLEM_ADV},
  {CreatureId::LAVA_GOLEM, 100, TechId::GOLEM_MAS},
};


void Collective::handleMatterAnimation(View* view) {
  handleSpawning(view, SquareType::LABORATORY,
      "You need to build a laboratory to animate golems.", "You need a larger laboratory.", "Golem animation",
      MinionType::GOLEM, animationInfo);
}

vector<Collective::SpawnInfo> tamingInfo {
  {CreatureId::RAVEN, 5, Nothing()},
  {CreatureId::WOLF, 30, TechId::BEAST},
  {CreatureId::CAVE_BEAR, 50, TechId::BEAST},
  {CreatureId::SPECIAL_MONSTER_KEEPER, 150, TechId::BEAST_MUT},
};

void Collective::handleBeastTaming(View* view) {
  handleSpawning(view, SquareType::ANIMAL_TRAP,
      "You need to build cages to trap beasts.", "You need more cages.", "Beast taming",
      MinionType::BEAST, tamingInfo);
}

vector<Collective::SpawnInfo> breedingInfo {
  {CreatureId::GNOME, 30, Nothing()},
  {CreatureId::GOBLIN, 50, TechId::GOBLIN},
  {CreatureId::BILE_DEMON, 80, TechId::OGRE},
  {CreatureId::SPECIAL_HUMANOID, 150, TechId::HUMANOID_MUT},
};

void Collective::handleHumanoidBreeding(View* view) {
  handleSpawning(view, SquareType::BED,
      "You need to build beds to breed humanoids.", "You need more beds.", "Humanoid breeding",
      MinionType::NORMAL, breedingInfo);
}

vector<CreatureId> Collective::getSpawnInfo(const Technology* tech) {
  vector<CreatureId> ret;
  for (SpawnInfo elem : concat<SpawnInfo>({breedingInfo, tamingInfo, animationInfo, raisingInfo}))
    if (elem.techId && Technology::get(*elem.techId) == tech)
      ret.push_back(elem.id);
  return ret;
}

int Collective::getNumMinions() const {
  return minions.size() - minionByType.at(MinionType::PRISONER).size();
}

void Collective::handleSpawning(View* view, SquareType spawnSquare, const string& info1, const string& info2,
    const string& title, MinionType minionType, vector<SpawnInfo> spawnInfo) {
  set<Vec2> cages = mySquares.at(spawnSquare);
  int prevItem = 0;
  bool allInactive = false;
  while (1) {
    vector<View::ListElem> options;
    if (getNumMinions() >= minionLimit) {
      allInactive = true;
      options.emplace_back("You have reached the limit of the number of minions.", View::TITLE);
    } else
    if (cages.empty()) {
      allInactive = true;
      options.emplace_back(info1, View::TITLE);
    } else
    if (cages.size() <= minionByType.at(minionType).size()) {
      allInactive = true;
      options.emplace_back(info2, View::TITLE);
    }

    vector<pair<PCreature, int>> creatures;
    for (SpawnInfo info : spawnInfo) {
      creatures.push_back({CreatureFactory::fromId(info.id, tribe, MonsterAIFactory::collective(this)),
          info.manaCost});
      bool isTech = !info.techId || hasTech(*info.techId);
      string suf;
      if (!isTech)
        suf = requires(*info.techId);
      options.emplace_back(creatures.back().first->getSpeciesName()
          + "  mana: " + convertToString(info.manaCost) + suf,
          allInactive || !isTech || info.manaCost > mana ? View::INACTIVE : View::NORMAL);
    }
    auto index = view->chooseFromList(title, options, prevItem);
    if (!index)
      return;
    Vec2 pos = chooseRandom(cages);
    PCreature& creature = creatures[*index].first;
    mana -= creatures[*index].second;
    for (Vec2 v : concat({pos}, pos.neighbors8(true)))
      if (level->getSquare(v)->canEnter(creature.get())) {
        addCreature(std::move(creature), v, minionType);
        break;
      }
    if (creature)
      messageBuffer.addMessage(MessageBuffer::important("The spell failed."));
    view->updateView(this);
    prevItem = *index;
  }
}

void Collective::handleNecromancy(View* view, int prevItem, bool firstTime) {
  set<Vec2> graves = mySquares.at(SquareType::GRAVE);
  vector<View::ListElem> options;
  bool allInactive = false;
  if (getNumMinions() >= minionLimit) {
    allInactive = true;
    options.emplace_back("You have reached the limit of the number of minions.", View::TITLE);
  } else
  if (graves.empty()) {
    allInactive = true;
    options.emplace_back("You need to build a graveyard and collect corpses to raise undead.", View::TITLE);
  } else
  if (graves.size() <= minionByType.at(MinionType::UNDEAD).size()) {
    allInactive = true;
    options.emplace_back("You need to build more graves for your undead to sleep in.", View::TITLE);
  }
  vector<pair<Vec2, Item*>> corpses;
  for (Vec2 pos : graves) {
    for (Item* it : level->getSquare(pos)->getItems([](const Item* it) {
        return it->getType() == ItemType::CORPSE && it->getCorpseInfo()->canBeRevived; }))
      corpses.push_back({pos, it});
  }
  if (!allInactive && corpses.empty()) {
    options.emplace_back("You need to collect some corpses to raise undead.", View::TITLE);
    allInactive = true;
  }
  vector<pair<PCreature, int>> creatures;
  for (SpawnInfo info : raisingInfo) {
    creatures.push_back({CreatureFactory::fromId(info.id, tribe, MonsterAIFactory::collective(this)),
        info.manaCost});
    string suf;
    bool isTech = !info.techId || hasTech(*info.techId);
    if (!isTech)
      suf = requires(*info.techId);
    options.emplace_back(creatures.back().first->getName() + "  mana: " + convertToString(info.manaCost) + suf,
          allInactive || !isTech || info.manaCost > mana ? View::INACTIVE : View::NORMAL);
  }
  auto index = view->chooseFromList("Raise undead: " + convertToString(corpses.size()) + " bodies available",
    options, prevItem);
  if (!index)
    return;
  // TODO: try many corpses before failing
  auto elem = chooseRandom(corpses);
  PCreature& creature = creatures[*index].first;
  mana -= creatures[*index].second;
  for (Vec2 v : concat({elem.first}, elem.first.neighbors8(true)))
    if (level->getSquare(v)->canEnter(creature.get())) {
      level->getSquare(elem.first)->removeItems({elem.second});
      addCreature(std::move(creature), v, MinionType::UNDEAD);
      break;
    }
  if (creature)
    messageBuffer.addMessage(MessageBuffer::important("You have failed to reanimate the corpse."));
  view->updateView(this);
  handleNecromancy(view, *index, false);
}


bool Collective::hasTech(TechId id) const {
  return contains(technologies, Technology::get(id));
}

int Collective::getMinLibrarySize() const {
  return 2 * technologies.size();
}

double Collective::getTechCost() {
  int numTech = technologies.size() - numFreeTech;
  int numDouble = 4;
  return pow(2, double(numTech) / numDouble);
}

void Collective::handleLibrary(View* view) {
  if (mySquares.at(SquareType::LIBRARY).empty()) {
    view->presentText("", "You need to build a library to start research.");
    return;
  }
  vector<View::ListElem> options;
  bool allInactive = false;
  if (mySquares.at(SquareType::LIBRARY).size() <= getMinLibrarySize()) {
    allInactive = true;
    options.emplace_back("You need a larger library to continue research.", View::TITLE);
  }
  int neededPoints = 50 * getTechCost();
  options.push_back(View::ListElem("You have " + convertToString(int(mana)) + " mana. " +
        convertToString(neededPoints) + " needed to advance.", View::TITLE));
  vector<Technology*> techs = filter(Technology::getNextTechs(technologies),
      [](const Technology* tech) { return tech->canResearch(); });
  for (Technology* tech : techs) {
    string text = tech->getName();
    options.emplace_back(text, neededPoints <= mana && !allInactive ? View::NORMAL : View::INACTIVE);
  }
  options.emplace_back("Researched:", View::TITLE);
  for (Technology* tech : technologies)
    options.emplace_back(tech->getName(), View::INACTIVE);
  auto index = view->chooseFromList("Library", options);
  if (!index)
    return;
  Technology* tech = techs[*index];
  mana -= neededPoints;
  acquireTech(tech);
  view->updateView(this);
  handleLibrary(view);
}

void Collective::acquireTech(Technology* tech, bool free) {
  technologies.push_back(tech);
  if (free)
    ++numFreeTech;
  for (auto elem : spellLearning)
    if (Technology::get(elem.techId) == tech)
      keeper->addSpell(elem.id);
}

typedef View::GameInfo::BandInfo::Button Button;

vector<Button> Collective::fillButtons(const vector<BuildInfo>& buildInfo) const {
  vector<Button> buttons;
  for (BuildInfo button : buildInfo) {
    bool isTech = (!button.techId || hasTech(*button.techId));
    switch (button.buildType) {
      case BuildInfo::SQUARE: {
            BuildInfo::SquareInfo& elem = button.squareInfo;
            Optional<pair<ViewObject, int>> cost;
            if (elem.cost.value > 0)
              cost = {getResourceViewObject(elem.cost.id), elem.cost.value};
            buttons.push_back({
                SquareFactory::get(elem.type)->getViewObject(),
                elem.name,
                cost,
                (elem.cost.value > 0 ? "[" + convertToString(mySquares.at(elem.type).size()) + "]" : ""),
                isTech ? "" : "Requires " + Technology::get(*button.techId)->getName() });
           }
           break;
      case BuildInfo::DIG: {
             buttons.push_back({
                 ViewObject(ViewId::DIG_ICON, ViewLayer::LARGE_ITEM, ""),
                 "Dig or cut tree", Nothing(), "", ""});
           }
           break;
      case BuildInfo::IMPALED_HEAD: {
           BuildInfo::SquareInfo& elem = button.squareInfo;
             buttons.push_back({
                 ViewObject(ViewId::IMPALED_HEAD, ViewLayer::LARGE_ITEM, ""),
                 elem.name, Nothing(), "(" + convertToString(executions) + " ready)",
                 executions > 0  ? "" : "inactive"});
           }
           break;
      case BuildInfo::FETCH: {
             buttons.push_back({
                 ViewObject(ViewId::FETCH_ICON, ViewLayer::LARGE_ITEM, ""),
                 "Fetch items", Nothing(), "", ""});
           }
           break;
      case BuildInfo::TRAP: {
             BuildInfo::TrapInfo& elem = button.trapInfo;
             int numTraps = getTrapItems(elem.type).size();
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
               "Imp",
               cost,
               "[" + convertToString(minionByType.at(MinionType::IMP).size()) + "]",
               getImpCost() <= mana ? "" : "inactive"});
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
  }
  return buttons;
}

vector<Collective::TechInfo> Collective::getTechInfo() const {
  vector<TechInfo> ret;
  ret.push_back({{ViewId::BILE_DEMON, "Humanoids", 'h'},
      [this](View* view) { handleHumanoidBreeding(view); }});
  ret.push_back({{ViewId::BEAR, "Beasts", 's'},
      [this](View* view) { handleBeastTaming(view); }}); 
  if (hasTech(TechId::GOLEM))
    ret.push_back({{ViewId::IRON_GOLEM, "Golems", 'm'},
      [this](View* view) { handleMatterAnimation(view); }});      
  if (hasTech(TechId::NECRO))
    ret.push_back({{ViewId::VAMPIRE, "Necromancy", 'n'},
      [this](View* view) { handleNecromancy(view); }});
  ret.push_back({{ViewId::MANA, "Sorcery"}, [this](View* view) {handlePersonalSpells(view);}});
  ret.push_back({{ViewId::EMPTY, ""}, [](View*) {}});
  ret.push_back({{ViewId::LIBRARY, "Library", 'l'},
      [this](View* view) { handleLibrary(view); }});
  ret.push_back({{ViewId::GOLD, "Black market"},
      [this](View* view) { handleMarket(view); }});
  ret.push_back({{ViewId::EMPTY, ""}, [](View*) {}});
  ret.push_back({{ViewId::BOOK, "Keeperopedia"},
      [this](View* view) { model->keeperopedia.present(view); }});
  return ret;
}

void Collective::refreshGameInfo(View::GameInfo& gameInfo) const {
  gameInfo.villageInfo.villages.clear();
  bool attacking = false;
  for (VillageControl* c : model->getVillageControls()) {
    gameInfo.villageInfo.villages.push_back(c->getVillageInfo());
    if (c->currentlyAttacking())
      attacking = true;
  }
  gameInfo.infoType = View::GameInfo::InfoType::BAND;
  View::GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.buildings = fillButtons(buildInfo);
  info.workshop = fillButtons(workshopInfo);
  info.tasks = minionTaskStrings;
  for (Creature* c : minions)
    if (isInCombat(c))
      info.tasks[c->getUniqueId()] = "fighting";
  info.monsterHeader = "Monsters: " + convertToString(getNumMinions()) + " / " + convertToString(minionLimit);
  info.creatures.clear();
  for (Creature* c : minions)
    info.creatures.push_back(c);
  info.enemies.clear();
  for (Vec2 v : myTiles)
    if (Creature* c = level->getSquare(v)->getCreature())
      if (c->getTribe() != tribe)
        info.enemies.push_back(c);
  info.numGold.clear();
  for (auto elem : resourceInfo)
    info.numGold.push_back({getResourceViewObject(elem.first), numGold(elem.first), elem.second.name});
  info.numGold.push_back({ViewObject::mana(), int(mana), "mana"});
  info.numGold.push_back({ViewObject(ViewId::DANGER, ViewLayer::CREATURE, ""), int(getDangerLevel()) + points,
      "points"});
  if (attacking) {
    if (info.warning.empty())
      info.warning = NameGenerator::insults.getNext();
  } else
    info.warning = "";
  for (int i : Range(numWarnings))
    if (warning[i]) {
      info.warning = warningText[i];
      break;
    }
  info.time = getTime();
  info.gatheringTeam = gatheringTeam;
  info.team.clear();
  for (Creature* c : team)
    info.team.push_back(c);
  info.techButtons.clear();
  for (TechInfo tech : getTechInfo())
    info.techButtons.push_back(tech.button);
}

bool Collective::isItemMarked(const Item* it) const {
  return markedItems.contains(it->getUniqueId());
}

void Collective::markItem(const Item* it) {
  markedItems.insert(it->getUniqueId());
}

void Collective::unmarkItem(UniqueId id) {
  markedItems.erase(id);
}

void Collective::updateMemory() {
  for (Vec2 v : level->getBounds())
    if (knownTiles[v])
      addToMemory(v);
}

const MapMemory& Collective::getMemory(const Level* l) const {
  return (*memory.get())[l];
}

MapMemory& Collective::getMemory(const Level* l) {
  return (*memory.get())[l];
}

ViewObject Collective::getTrapObject(TrapType type) {
  for (const Collective::BuildInfo& info : workshopInfo)
    if (info.buildType == BuildInfo::TRAP && info.trapInfo.type == type)
      return ViewObject(info.trapInfo.viewId, ViewLayer::LARGE_ITEM, "Unarmed trap")
        .setModifier(ViewObject::PLANNED);
  FAIL << "trap not found" << int(type);
  return ViewObject(ViewId::EMPTY, ViewLayer::LARGE_ITEM, "Unarmed trap");
}

ViewIndex Collective::getViewIndex(Vec2 pos) const {
  ViewIndex index = level->getSquare(pos)->getViewIndex(this);
  if (taskMap.isMarked(pos))
    index.setHighlight(HighlightType::BUILD);
  else if (rectSelectCorner && rectSelectCorner2 && pos.inRectangle(Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2})))
    index.setHighlight(HighlightType::RECT_SELECTION);
  if (!index.hasObject(ViewLayer::LARGE_ITEM)) {
    if (traps.count(pos))
      index.insert(getTrapObject(traps.at(pos).type));
    if (guardPosts.count(pos))
      index.insert(ViewObject(ViewId::GUARD_POST, ViewLayer::LARGE_ITEM, "Guard post"));
    if (constructions.count(pos) && !constructions.at(pos).built)
      index.insert(SquareFactory::get(constructions.at(pos).type)->getViewObject().setModifier(ViewObject::PLANNED));
  }
  if (const Location* loc = level->getLocation(pos)) {
    if (loc->isMarkedAsSurprise() && loc->getBounds().middle() == pos && !knownPos(pos))
      index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Surprise"));
  }
  return index;
}

bool Collective::staticPosition() const {
  return false;
}

Vec2 Collective::getPosition() const {
  return keeper->getPosition();
}

enum Selection { SELECT, DESELECT, NONE } selection = NONE;

vector<Task*> Collective::TaskMap::getTasks() {
  return transform2<Task*>(tasks, [](PTask& t) { return t.get(); });
}

Task* Collective::TaskMap::getTask(const Creature* c) const {
  if (taskMap.count(c))
    return taskMap.at(c);
  else
    return nullptr;
}

Task* Collective::TaskMap::addTask(PTask task, const Creature* c) {
  taken[task.get()] = c;
  taskMap[c] = task.get();
  return addTask(std::move(task));
}

Task* Collective::TaskMap::addTask(PTask task, CostInfo cost) {
  tasks.push_back(std::move(task));
  completionCost[tasks.back().get()] = cost;
  return tasks.back().get();
}

Collective::CostInfo Collective::TaskMap::removeTask(Task* task) {
  CostInfo cost {ResourceId::GOLD, 0};
  if (completionCost.count(task)) {
    cost = completionCost.at(task);
    completionCost.erase(task);
  }
  if (marked.count(task->getPosition()))
    marked.erase(task->getPosition());
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
      break;
    }
  if (taken.count(task)) {
    taskMap.erase(taken.at(task));
    taken.erase(task);
  }
  return cost;
}

Collective::CostInfo Collective::TaskMap::removeTask(UniqueId id) {
  for (PTask& task : tasks)
    if (task->getUniqueId() == id) {
      return removeTask(task.get());
    }
  return {ResourceId::GOLD, 0};
}
bool Collective::TaskMap::isLocked(const Creature* c, const Task* t) const {
  return lockedTasks.count({c, t->getUniqueId()});
}

void Collective::TaskMap::lock(const Creature* c, const Task* t) {
  lockedTasks.insert({c, t->getUniqueId()});
}

void Collective::TaskMap::clearAllLocked() {
  lockedTasks.clear();
}

bool Collective::TaskMap::isMarked(Vec2 pos) const {
  return marked.count(pos);
}

void Collective::TaskMap::markSquare(Vec2 pos, PTask task) {
  addTask(std::move(task));
  marked[pos] = tasks.back().get();
}

void Collective::TaskMap::unmarkSquare(Vec2 pos) {
  Task* t = marked.at(pos);
  removeTask(t);
  marked.erase(pos);
}

bool Collective::hasGold(CostInfo cost) const {
  return numGold(cost.id) >= cost.value;
}

int Collective::numGold(ResourceId id) const {
  int ret = credit.at(id);
  for (Vec2 pos : mySquares.at(resourceInfo.at(id).storageType))
    ret += level->getSquare(pos)->getItems(resourceInfo.at(id).predicate).size();
  return ret;
}

void Collective::takeGold(CostInfo cost) {
  int num = cost.value;
  if (num == 0)
    return;
  CHECK(num > 0);
  if (credit.at(cost.id)) {
    if (credit.at(cost.id) >= num) {
      credit[cost.id] -= num;
      return;
    } else {
      num -= credit.at(cost.id);
      credit[cost.id] = 0;
    }
  }
  for (Vec2 pos : randomPermutation(mySquares[resourceInfo.at(cost.id).storageType])) {
    vector<Item*> goldHere = level->getSquare(pos)->getItems(resourceInfo.at(cost.id).predicate);
    for (Item* it : goldHere) {
      level->getSquare(pos)->removeItem(it);
      if (--num == 0)
        return;
    }
  }
  FAIL << "Didn't have enough gold";
}

void Collective::returnGold(CostInfo amount) {
  if (amount.value == 0)
    return;
  CHECK(amount.value > 0);
  if (mySquares[resourceInfo.at(amount.id).storageType].empty()) {
    credit[amount.id] += amount.value;
  } else
    level->getSquare(chooseRandom(mySquares[resourceInfo.at(amount.id).storageType]))->
        dropItems(ItemFactory::fromId(resourceInfo.at(amount.id).itemId, amount.value));
}

int Collective::getImpCost() const {
  int numImps = 0;
  for (Creature* c : minionByType.at(MinionType::IMP))
    if (!contains(minions, c)) // see if it's not a prisoner
      ++numImps;
  if (numImps < startImpNum)
    return 0;
  return basicImpCost * pow(2, double(numImps - startImpNum) / 5);
}

void Collective::possess(const Creature* cr, View* view) {
  view->stopClock();
  CHECK(contains(creatures, cr));
  CHECK(!cr->isDead());
  Creature* c = const_cast<Creature*>(cr);
  if (c->isAffected(Creature::SLEEP))
    c->removeEffect(Creature::SLEEP);
  freeFromGuardPost(c);
  updateMemory();
  c->pushController(new Player(c, view, model, false, memory.get()));
  possessed = c;
  c->getLevel()->setPlayer(c);
}

bool Collective::canBuildDoor(Vec2 pos) const {
  if (!level->getSquare(pos)->canConstruct(SquareType::TRIBE_DOOR))
    return false;
  Rectangle innerRect = level->getBounds().minusMargin(1);
  auto wallFun = [=](Vec2 pos) {
      return level->getSquare(pos)->canConstruct(SquareType::FLOOR) ||
          !pos.inRectangle(innerRect); };
  return !traps.count(pos) && pos.inRectangle(innerRect) && 
      ((wallFun(pos - Vec2(0, 1)) && wallFun(pos - Vec2(0, -1))) ||
       (wallFun(pos - Vec2(1, 0)) && wallFun(pos - Vec2(-1, 0))));
}

bool Collective::canPlacePost(Vec2 pos) const {
  return !guardPosts.count(pos) && !traps.count(pos) &&
      level->getSquare(pos)->canEnterEmpty(Creature::getDefault()) && knownPos(pos);
}
  
void Collective::freeFromGuardPost(const Creature* c) {
  for (auto& elem : guardPosts)
    if (elem.second.attender == c)
      elem.second.attender = nullptr;
}

Creature* Collective::getCreature(UniqueId id) {
  for (Creature* c : creatures)
    if (c->getUniqueId() == id)
      return c;
  FAIL << "Creature not found " << id;
  return nullptr;
}

void Collective::processInput(View* view, CollectiveAction action) {
  if (retired)
    return;
  switch (action.getType()) {
    case CollectiveAction::GATHER_TEAM:
        if (gatheringTeam && !team.empty()) {
          possess(team[0], view);
 //         gatheringTeam = false;
          for (Creature* c : team) {
            freeFromGuardPost(c);
            if (c->isAffected(Creature::SLEEP))
              c->removeEffect(Creature::SLEEP);
          }
        } else
          gatheringTeam = true;
        break;
    case CollectiveAction::DRAW_LEVEL_MAP: view->drawLevelMap(this); break;
    case CollectiveAction::CANCEL_TEAM: gatheringTeam = false; team.clear(); break;
    case CollectiveAction::MARKET: handleMarket(view); break;
    case CollectiveAction::TECHNOLOGY: {
        vector<TechInfo> techInfo = getTechInfo();
        techInfo[action.getNum()].butFun(view);
        break;}
    case CollectiveAction::CREATURE_BUTTON: 
        if (!gatheringTeam)
          minionView(view, getCreature(action.getNum()));
        else {
          if (contains(team, getCreature(action.getNum())))
            removeElement(team, getCreature(action.getNum()));
          else
            team.push_back(getCreature(action.getNum()));
        }
        break;
    case CollectiveAction::POSSESS: {
        Vec2 pos = action.getPosition();
        if (!pos.inRectangle(level->getBounds()))
          return;
        if (Creature* c = level->getSquare(pos)->getCreature())
          if (contains(minions, c)) {
            minionView(view, c);
//            possess(c, view);
            break;
          }
        }
        break;
    case CollectiveAction::RECT_SELECTION:
        if (rectSelectCorner) {
          rectSelectCorner2 = action.getPosition();
        } else
          rectSelectCorner = action.getPosition();
        break;
    case CollectiveAction::BUILD:
        handleSelection(action.getPosition(), buildInfo[action.getNum()], false);
        break;
    case CollectiveAction::WORKSHOP:
        handleSelection(action.getPosition(), workshopInfo[action.getNum()], false);
        break;
    case CollectiveAction::BUTTON_RELEASE:
        selection = NONE;
        if (rectSelectCorner && rectSelectCorner2) {
          handleSelection(*rectSelectCorner, buildInfo[action.getNum()], true);
          for (Vec2 v : Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2}))
            handleSelection(v, buildInfo[action.getNum()], true);
        }
        rectSelectCorner = Nothing();
        rectSelectCorner2 = Nothing();
        selection = NONE;
        break;
    case CollectiveAction::EXIT: model->exitAction(); break;
    case CollectiveAction::IDLE: break;
  }
}

void Collective::handleSelection(Vec2 pos, const BuildInfo& building, bool rectangle) {
  if (building.techId && !hasTech(*building.techId))
    return;
  if (!pos.inRectangle(level->getBounds()))
    return;
  switch (building.buildType) {
    case BuildInfo::IMP:
        if (mana >= getImpCost() && selection == NONE && !rectangle) {
          selection = SELECT;
          PCreature imp = CreatureFactory::fromId(CreatureId::IMP, tribe,
              MonsterAIFactory::collective(this));
          for (Vec2 v : pos.neighbors8(true))
            if (v.inRectangle(level->getBounds()) && level->getSquare(v)->canEnter(imp.get()) 
                && canSee(v)) {
              mana -= getImpCost();
              addCreature(std::move(imp), v, MinionType::IMP);
              break;
            }
        }
        break;
    case BuildInfo::TRAP: {
        TrapType trapType = building.trapInfo.type;
        if (traps.count(pos)) {
          traps.erase(pos);
        } else
        if (canPlacePost(pos) && myTiles.count(pos)) {
          traps[pos] = {trapType, false, 0};
          updateConstructions();
        }
      }
      break;
    case BuildInfo::DESTROY:
        selection = SELECT;
        if (level->getSquare(pos)->canDestroy() && myTiles.count(pos))
          level->getSquare(pos)->destroy();
        level->getSquare(pos)->removeTriggers();
        if (Creature* c = level->getSquare(pos)->getCreature())
          if (c->getName() == "boulder")
            c->die(nullptr, false);
        if (traps.count(pos)) {
          traps.erase(pos);
        }
        if (constructions.count(pos)) {
          returnGold(taskMap.removeTask(constructions.at(pos).task));
          constructions.erase(pos);
        }
        break;
    case BuildInfo::GUARD_POST:
        if (guardPosts.count(pos) && selection != SELECT) {
          guardPosts.erase(pos);
          selection = DESELECT;
        }
        else if (canPlacePost(pos) && guardPosts.size() < getNumMinions() && selection != DESELECT) {
          guardPosts[pos] = {nullptr};
          selection = SELECT;
        }
        break;
    case BuildInfo::DIG:
        if (!tryLockingDoor(pos)) {
          if (taskMap.isMarked(pos) && selection != SELECT) {
            taskMap.unmarkSquare(pos);
            selection = DESELECT;
          } else
            if (!taskMap.isMarked(pos) && selection != DESELECT) {
              if (level->getSquare(pos)->canConstruct(SquareType::TREE_TRUNK)) {
                taskMap.markSquare(pos, Task::construction(this, pos, SquareType::TREE_TRUNK));
                selection = SELECT;
              } else
                if (level->getSquare(pos)->canConstruct(SquareType::FLOOR) || !knownPos(pos)) {
                  taskMap.markSquare(pos, Task::construction(this, pos, SquareType::FLOOR));
                  selection = SELECT;
                }
            }
        }
        break;
    case BuildInfo::FETCH:
        for (ItemFetchInfo elem : getFetchInfo())
          fetchItems(pos, elem);
        break;
    case BuildInfo::IMPALED_HEAD:
    case BuildInfo::SQUARE:
        if (!tryLockingDoor(pos)) {
          if (constructions.count(pos)) {
            if (selection != SELECT) {
              returnGold(taskMap.removeTask(constructions.at(pos).task));
              if (constructions.at(pos).type == SquareType::IMPALED_HEAD)
                ++executions;
              constructions.erase(pos);
              selection = DESELECT;
            }
          } else {
            BuildInfo::SquareInfo info = building.squareInfo;
            if (knownPos(pos) && level->getSquare(pos)->canConstruct(info.type) && !traps.count(pos)
                && (info.type != SquareType::TRIBE_DOOR || canBuildDoor(pos)) && selection != DESELECT) {
              if (info.buildImmediatly) {
                while (!level->getSquare(pos)->construct(info.type)) {}
                onConstructed(pos, info.type);
              } else {
                if (info.type == SquareType::IMPALED_HEAD) {
                  if (executions <= 0)
                    break;
                  else
                    --executions;
                }
                constructions[pos] = {info.cost, false, 0, info.type, -1};
                selection = SELECT;
                updateConstructions();
              }
            }
          }
        }
        break;
  }
}

bool Collective::tryLockingDoor(Vec2 pos) {
  return false;
  if (constructions.count(pos)) {
    Square* square = level->getSquare(pos);
    if (square->canLock()) {
      if (selection != DESELECT && !square->isLocked()) {
        square->lock();
        selection = SELECT;
      } else if (selection != SELECT && square->isLocked()) {
        square->lock();
        selection = DESELECT;
      }
      return true;
    }
  }
  return false;
}

void Collective::onConstructed(Vec2 pos, SquareType type) {
  if (!contains({SquareType::ANIMAL_TRAP, SquareType::TREE_TRUNK}, type))
    myTiles.insert(pos);
  CHECK(!mySquares[type].count(pos));
  mySquares[type].insert(pos);
  if (contains({SquareType::FLOOR, SquareType::BRIDGE}, type))
    taskMap.clearAllLocked();
  if (taskMap.isMarked(pos))
    taskMap.unmarkSquare(pos);
  if (constructions.count(pos)) {
    constructions.at(pos).built = true;
    constructions.at(pos).marked = 0;
    constructions.at(pos).task = -1;
  }
}

void Collective::onPickedUp(Vec2 pos, EntitySet items) {
  for (UniqueId id : items)
    unmarkItem(id);
}
  
void Collective::onCantPickItem(EntitySet items) {
  for (UniqueId id : items)
    unmarkItem(id);
}

void Collective::onBrought(Vec2 pos, vector<Item*> items) {
}

void Collective::onAppliedItem(Vec2 pos, Item* item) {
  CHECK(item->getTrapType());
  if (traps.count(pos)) {
    traps[pos].marked = 0;
    traps[pos].armed = true;
  }
}

set<TrapType> Collective::getNeededTraps() const {
  set<TrapType> ret;
  for (auto elem : traps)
    if (!elem.second.marked && !elem.second.armed)
      ret.insert(elem.second.type);
  return ret;
}

void Collective::onAppliedSquare(Vec2 pos) {
  if (mySquares.at(SquareType::LIBRARY).count(pos)) {
    mana += 0.3 + max(0., 2 - (mana + double(getDangerLevel(false))) / 700);
  }
  if (mySquares.at(SquareType::TRAINING_DUMMY).count(pos)) {
    if (hasTech(TechId::ARCHERY) && Random.roll(30))
      NOTNULL(level->getSquare(pos)->getCreature())->addSkill(Skill::archery);
  }
  if (mySquares.at(SquareType::LABORATORY).count(pos))
    if (Random.roll(30)) {
      level->getSquare(pos)->dropItems(ItemFactory::laboratory(technologies).random());
      Statistics::add(StatId::POTION_PRODUCED);
    }
  if (mySquares.at(SquareType::WORKSHOP).count(pos))
    if (Random.roll(40)) {
      set<TrapType> neededTraps = getNeededTraps();
      vector<PItem> items;
      for (int i : Range(10)) {
        items = ItemFactory::workshop(technologies).random();
        if (neededTraps.empty() || (items[0]->getTrapType() && neededTraps.count(*items[0]->getTrapType())))
          break;
      }
      if (items[0]->getType() == ItemType::WEAPON)
        Statistics::add(StatId::WEAPON_PRODUCED);
      if (items[0]->getType() == ItemType::ARMOR)
        Statistics::add(StatId::ARMOR_PRODUCED);
      level->getSquare(pos)->dropItems(std::move(items));
    }
}

void Collective::onAppliedItemCancel(Vec2 pos) {
  if (traps.count(pos))
    traps.at(pos).marked = 0;
}

bool Collective::isRetired() const {
  return retired;
}

const Creature* Collective::getKeeper() const {
  return keeper;
}

Vec2 Collective::getDungeonCenter() const {
  if (!myTiles.empty())
    return Vec2::getCenterOfWeight(vector<Vec2>(myTiles.begin(), myTiles.end()));
  else
    return keeper->getPosition();
}

double Collective::getDangerLevel(bool includeExecutions) const {
  double ret = 0;
  for (const Creature* c : minions)
    ret += c->getDifficultyPoints();
  if (includeExecutions)
    ret += mySquares.at(SquareType::IMPALED_HEAD).size() * 150;
  return ret;
}

void Collective::learnLocation(const Location* loc) {
  for (Vec2 pos : loc->getBounds())
    getMemory(loc->getLevel()).addObject(pos, level->getSquare(pos)->getViewObject());
}

ItemPredicate Collective::unMarkedItems(ItemType type) const {
  return [this, type](const Item* it) {
      return it->getType() == type && !isItemMarked(it); };
}

void Collective::addToMemory(Vec2 pos) {
  ViewIndex index = level->getSquare(pos)->getViewIndex(this);
  getMemory(level).clearSquare(pos);
  for (ViewLayer l : { ViewLayer::ITEM, ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM})
    if (index.hasObject(l))
      getMemory(level).addObject(pos, index.getObject(l));
}

void Collective::update(Creature* c) {
  if (!retired && possessed != keeper && (isInCombat(keeper) || keeper->getHealth() < 1)
      && lastControlKeeperQuestion < getTime() - 50) {
    lastControlKeeperQuestion = getTime();
    if (model->getView()->yesOrNoPrompt("The keeper is engaged in combat. Do you want to control him?")) {
      if (possessed && possessed != keeper)
        possessed->popController();
      possess(keeper, model->getView());
      return;
    }
  }
  if (!contains(creatures, c) || c->getLevel() != level)
    return;
  for (Vec2 pos : level->getVisibleTiles(c))
    knownTiles[pos] = true;
//    addToMemory(pos, c);
}

bool Collective::isDownstairsVisible() const {
  vector<Vec2> v = level->getLandingSquares(StairDirection::DOWN, StairKey::DWARF);
  return v.size() == 1 && knownPos(v[0]);
}

// after this time applying trap or building door is rescheduled (imp death, etc).
const static int timeToBuild = 50;

void Collective::updateConstructions() {
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems;
  for (const BuildInfo& info : workshopInfo)
    if (info.buildType == BuildInfo::TRAP)
      trapItems[info.trapInfo.type] = getTrapItems(info.trapInfo.type, myTiles);
  for (auto elem : traps)
    if (!isDelayed(elem.first)) {
      vector<pair<Item*, Vec2>>& items = trapItems.at(elem.second.type);
      if (!items.empty()) {
        if (!elem.second.armed && elem.second.marked <= getTime()) {
          taskMap.addTask(Task::applyItem(this, items.back().second, items.back().first, elem.first));
          markItem(items.back().first);
          items.pop_back();
          traps[elem.first].marked = getTime() + timeToBuild;
        }
      }
    }
  for (auto& elem : constructions)
    if (!isDelayed(elem.first) && elem.second.marked <= getTime() && !elem.second.built) {
      if ((warning[int(resourceInfo.at(elem.second.cost.id).warning)]
          = (numGold(elem.second.cost.id) < elem.second.cost.value)))
        continue;
      elem.second.task = taskMap.addTask(Task::construction(this, elem.first, elem.second.type),
          elem.second.cost)->getUniqueId();
      elem.second.marked = getTime() + timeToBuild;
      takeGold(elem.second.cost);
    }
}

double Collective::getTime() const {
  return keeper->getTime();
}

void Collective::delayDangerousTasks(const vector<Vec2>& enemyPos, double delayTime) {
  int infinity = 1000000;
  int radius = 10;
  Table<int> dist(Rectangle::boundingBox(enemyPos).minusMargin(-radius).intersection(level->getBounds()), infinity);
  queue<Vec2> q;
  for (Vec2 v : enemyPos) {
    dist[v] = 0;
    q.push(v);
  }
  while (!q.empty()) {
    Vec2 pos = q.front();
    q.pop();
    delayedPos[pos] = delayTime;
    if (dist[pos] >= radius)
      continue;
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(dist.getBounds()) && dist[v] == infinity &&
          /*level->getSquare(v)->canEnterEmpty(Creature::getDefault()) &&*/ myTiles.count(v)) {
        dist[v] = dist[pos] + 1;
        q.push(v);
      }
  }
}

bool Collective::isDelayed(Vec2 pos) {
  return delayedPos.count(pos) && delayedPos.at(pos) > getTime();
}

void Collective::tick() {
  if (retired) {
    if (const Creature* c = level->getPlayer())
      if (Random.roll(30) && !myTiles.count(c->getPosition()))
        c->privateMessage("You sense horrible evil in the " + 
            getCardinalName((keeper->getPosition() - c->getPosition()).getBearing().getCardinalDir()));
  }
  updateVisibleEnemies();
  warning[int(Warning::MANA)] = mana < 100;
  warning[int(Warning::WOOD)] = numGold(ResourceId::WOOD) == 0;
  warning[int(Warning::DIGGING)] = mySquares.at(SquareType::FLOOR).empty();
  warning[int(Warning::MINIONS)] = getNumMinions() <= 1;
  for (auto elem : taskInfo)
    if (!mySquares.at(elem.second.square).empty())
      warning[int(elem.second.warning)] = false;
  map<Vec2, int> extendedTiles;
  queue<Vec2> extendedQueue;
  vector<Vec2> enemyPos;
  for (Vec2 pos : myTiles) {
    if (Creature* c = level->getSquare(pos)->getCreature()) {
      if (c->getTribe() != tribe)
        enemyPos.push_back(c->getPosition());
    }
 /*   if (taskMap.isMarked(pos) && marked.at(pos)->isImpossible(level) && !taken.count(marked.at(pos)))
      removeTask(marked.at(pos));*/
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(level->getBounds()) && !myTiles.count(v) && !extendedTiles.count(v) 
          && level->getSquare(v)->canEnterEmpty(Creature::getDefault())) {
        extendedTiles[v] = 1;
        extendedQueue.push(v);
      }
  }
  const int maxRadius = 10;
  while (!extendedQueue.empty()) {
    Vec2 pos = extendedQueue.front();
    extendedQueue.pop();
    if (Creature* c = level->getSquare(pos)->getCreature())
      if (c->getTribe() != tribe)
        enemyPos.push_back(c->getPosition());
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(level->getBounds()) && !myTiles.count(v) && !extendedTiles.count(v) 
          && level->getSquare(v)->canEnterEmpty(Creature::getDefault())) {
        int a = extendedTiles[v] = extendedTiles[pos] + 1;
        if (a < maxRadius)
          extendedQueue.push(v);
      }

  }
  if (!enemyPos.empty())
    delayDangerousTasks(enemyPos, getTime() + 20);
  else
    alarmInfo.finishTime = -1000;
  bool allSurrender = !retired;
  for (Vec2 v : enemyPos)
    if (!prisonerInfo.count(NOTNULL(level->getSquare(v)->getCreature()))) {
      allSurrender = false;
      break;
    }
  if (allSurrender) {
    for (auto elem : copyThis(prisonerInfo))
      if (elem.second.state == PrisonerInfo::SURRENDER) {
        Creature* c = elem.first;
        Vec2 pos = c->getPosition();
        if (myTiles.count(pos) && !c->isDead()) {
          level->globalMessage(pos, c->getTheName() + " surrenders.");
          c->die(nullptr, true, false);
          addCreature(CreatureFactory::fromId(
                CreatureId::PRISONER, tribe, MonsterAIFactory::collective(this)), pos, MinionType::PRISONER);
        }
        prisonerInfo.erase(c);
      }
  }
  updateConstructions();
  for (ItemFetchInfo elem : getFetchInfo()) {
    for (Vec2 pos : myTiles)
      fetchItems(pos, elem);
    for (SquareType type : elem.additionalPos)
      for (Vec2 pos : mySquares.at(type))
        fetchItems(pos, elem);
  }
}

static Vec2 chooseRandomClose(Vec2 start, const set<Vec2>& squares) {
  int minD = 10000;
  int margin = 5;
  int a;
  vector<Vec2> close;
  for (Vec2 v : squares)
    if ((a = v.dist8(start)) < minD)
      minD = a;
  for (Vec2 v : squares)
    if (v.dist8(start) < minD + margin)
      close.push_back(v);
  CHECK(!close.empty());
  return chooseRandom(close);
}

void Collective::fetchItems(Vec2 pos, ItemFetchInfo elem) {
  if (isDelayed(pos) || (traps.count(pos) && traps.at(pos).type == TrapType::BOULDER && traps.at(pos).armed == true))
    return;
  vector<Item*> equipment = level->getSquare(pos)->getItems(elem.predicate);
  if (mySquares[elem.destination].count(pos))
    return;
  if (!equipment.empty()) {
    if (mySquares[elem.destination].empty())
      warning[int(elem.warning)] = true;
    else {
      warning[int(elem.warning)] = false;
      if (elem.oneAtATime)
        equipment = {equipment[0]};
      Vec2 target = chooseRandomClose(pos, mySquares[elem.destination]);
      taskMap.addTask(Task::bringItem(this, pos, equipment, target));
      for (Item* it : equipment)
        markItem(it);
    }
  }
}

bool Collective::canSee(const Creature* c) const {
  return canSee(c->getPosition());
}

bool Collective::canSee(Vec2 position) const {
  return knownPos(position);
}

bool Collective::knownPos(Vec2 position) const {
  return knownTiles[position];
}

void Collective::setLevel(Level* l) {
  for (Vec2 v : l->getBounds())
    if (contains({"gold ore", "iron ore", "stone"}, l->getSquare(v)->getName()))
      getMemory(l).addObject(v, l->getSquare(v)->getViewObject());
  level = l;
  knownTiles = Table<bool>(level->getBounds(), false);
}

vector<const Creature*> Collective::getUnknownAttacker() const {
  return {};
}

Tribe* Collective::getTribe() const {
  return tribe;
}

bool Collective::isEnemy(const Creature* c) const {
  return keeper->isEnemy(c);
}

void Collective::onChangeLevelEvent(const Creature* c, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {
  if (c == possessed) { 
    teamLevelChanges[from] = pos;
    if (!levelChangeHistory.count(to))
      levelChangeHistory[to] = toPos;
  }
}

static const int alarmTime = 100;

void Collective::onAlarmEvent(const Level* l, Vec2 pos) {
  if (l == level) {
    alarmInfo.finishTime = getTime() + alarmTime;
    alarmInfo.position = pos;
    for (Creature* c : minions)
      if (c->isAffected(Creature::SLEEP))
        c->removeEffect(Creature::SLEEP);
  }
}

void Collective::onTechBookEvent(Technology* tech) {
  vector<Technology*> nextTechs = Technology::getNextTechs(technologies);
  if (tech == nullptr) {
    if (!nextTechs.empty())
      tech = chooseRandom(nextTechs);
    else
      tech = chooseRandom(Technology::getAll());
  }
  if (!contains(technologies, tech)) {
    if (!contains(nextTechs, tech))
      messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
          + ", but you do not comprehend it.");
    else {
      messageBuffer.addImportantMessage("You have acquired the knowledge of " + tech->getName());
      acquireTech(tech, true);
    }
  } else {
    messageBuffer.addImportantMessage("The tome describes the knowledge of " + tech->getName()
        + ", which you already possess.");
  }
}

void Collective::onEquipEvent(const Creature* c, const Item* it) {
  if (possessed == c)
    minionEquipment.own(c, it);
}

void Collective::onPickupEvent(const Creature* c, const vector<Item*>& items) {
  if (possessed == c)
    for (Item* it : items)
      if (minionEquipment.isItemUseful(it))
        minionEquipment.own(c, it);
}

void Collective::onSurrenderEvent(Creature* who, const Creature* to) {
  if (contains(creatures, to) && !contains(creatures, who) && !prisonerInfo.count(who))
    prisonerInfo[who] = {PrisonerInfo::SURRENDER, nullptr};
}

void Collective::onTortureEvent(Creature* who, const Creature* torturer) {
  mana += 1;
}

MoveInfo Collective::getBeastMove(Creature* c) {
  if (!Random.roll(5))
    return NoMove;
  Vec2 radius(7, 7);
  for (Vec2 v : randomPermutation(Rectangle(c->getPosition() - radius, c->getPosition() + radius).getAllSquares()))
    if (v.inRectangle(level->getBounds()) && !knownPos(v)) {
      if (auto move = c->getMoveTowards(v))
        return {1.0, [c, move]() { return c->move(*move); }};
      else
        return NoMove;
    }
  return NoMove;
}

MoveInfo Collective::getGuardPostMove(Creature* c) {
  for (auto& elem : guardPosts) {
    bool isTraining = contains({MinionTask::TRAIN, MinionTask::LABORATORY, MinionTask::WORKSHOP},
        minionTasks.at(c->getUniqueId()).getState());
    if (elem.second.attender == c) {
      if (isTraining) {
        minionTasks.at(c->getUniqueId()).update();
        minionTaskStrings[c->getUniqueId()] = "guarding";
        if (c->getPosition().dist8(elem.first) > 1) {
          if (auto move = c->getMoveTowards(elem.first))
            return {1.0, [=] {
              c->move(*move);
            }};
        } else
          return NoMove;
      } else
        elem.second.attender = nullptr;
    }
  }
  for (auto& elem : guardPosts) {
    bool isTraining = contains({MinionTask::TRAIN}, minionTasks.at(c->getUniqueId()).getState());
    if (elem.second.attender == nullptr && isTraining) {
      elem.second.attender = c;
      if (Task* t = taskMap.getTask(c))
        taskMap.removeTask(t);
    }
  }
  return NoMove;
}

MoveInfo Collective::getPossessedMove(Creature* c) {
  Optional<Vec2> v;
  if (possessed->getLevel() != c->getLevel()) {
    if (teamLevelChanges.count(c->getLevel())) {
      v = teamLevelChanges.at(c->getLevel());
      if (v == c->getPosition())
        return {1.0, [=] {
          c->applySquare();
        }};
    }
  } else 
    v = possessed->getPosition();
  if (v)
    if (auto move = c->getMoveTowards(*v))
      return {1.0, [=] {
        c->move(*move);
      }};
  return NoMove;
}

MoveInfo Collective::getBacktrackMove(Creature* c) {
  if (!levelChangeHistory.count(c->getLevel()))
    return NoMove;
  Vec2 target = levelChangeHistory.at(c->getLevel());
  if (c->getPosition() == target)
    return {1.0, [=] {
      c->applySquare();
    }};
  else if (auto move = c->getMoveTowards(target))
    return {1.0, [=] {
      c->move(*move);
    }};
  else
    return NoMove;
}

MoveInfo Collective::getAlarmMove(Creature* c) {
  if (alarmInfo.finishTime > c->getTime())
    if (auto move = c->getMoveTowards(alarmInfo.position))
      return {1.0, [=] {
        c->move(*move);
      }};
  return NoMove;
}

MoveInfo Collective::getDropItems(Creature *c) {
  if (myTiles.count(c->getPosition())) {
    vector<Item*> items = c->getEquipment().getItems([this, c](const Item* item) {
        return minionEquipment.isItemUseful(item) && minionEquipment.getOwner(item) != c; });
    if (!items.empty())
      return {1.0, [=] {
        c->drop(items);
      }};
  }
  return NoMove;   
}

MoveInfo Collective::getExecutionMove(Creature* c) {
  if (contains({MinionType::BEAST, MinionType::PRISONER, MinionType::KEEPER}, getMinionType(c)))
    return NoMove;
  for (auto& elem : prisonerInfo)
    if (contains(creatures, elem.first) 
        && (elem.second.attender == c || !elem.second.attender || elem.second.attender->isDead())) {
      if (elem.second.state == PrisonerInfo::EXECUTE) {
        elem.second.attender = c;
        if (elem.first->getPosition().dist8(c->getPosition()) == 1)
          return {1.0, [=] () {
            c->attack(elem.first);
            c->globalMessage(c->getTheName() + " executes " + elem.first->getTheName());
          }};
        else if (auto move = c->getMoveTowards(elem.first->getPosition()))
          return {1.0, [=] () {
            c->move(*move);
          }};
        else
          return NoMove;
      }
      if (getMinionTask(elem.first) == MinionTask::TORTURE) {
        elem.second.attender = c;
        if (elem.first->getPosition().dist8(c->getPosition()) == 1
            && elem.first->getSquare()->getApplyType(elem.first) == SquareApplyType::TORTURE)
          return {1.0, [=] () {
            c->torture(elem.first);
            c->globalMessage(c->getTheName() + " tortures " + elem.first->getTheName());
          }};
        else if (auto move = c->getMoveTowards(elem.first->getPosition()))
          return {1.0, [=] () {
            c->move(*move);
          }};
        else
          return NoMove;
      }
    }
  return NoMove;
}

MoveInfo Collective::getMinionMove(Creature* c) {
  if (possessed && contains(team, c))
    return getPossessedMove(c);
  if (c->getLevel() != level)
    return getBacktrackMove(c);
  if (!contains({MinionType::KEEPER, MinionType::PRISONER}, getMinionType(c)))
    if (MoveInfo alarmMove = getAlarmMove(c))
      return alarmMove;
  if (MoveInfo dropMove = getDropItems(c))
    return dropMove;
  if (contains(minionByType.at(MinionType::BEAST), c))
    return getBeastMove(c);
  if (MoveInfo execMove = getExecutionMove(c))
    return execMove;
  if (MoveInfo guardPostMove = getGuardPostMove(c))
    return guardPostMove;
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  autoEquipment(c);
  if (c != keeper || !underAttack())
    for (Vec2 v : mySquares[SquareType::STOCKPILE])
      for (Item* it : level->getSquare(v)->getItems([this, c] (const Item* it) {
            return minionEquipment.getOwner(it) == c; })) {
        if (c->canEquip(it))
          taskMap.addTask(Task::equipItem(this, v, it), c);
        else
          taskMap.addTask(Task::pickItem(this, v, {it}), c);
        return taskMap.getTask(c)->getMove(c);
      }
  minionTasks.at(c->getUniqueId()).update();
  if (c->getHealth() < 1 && c->canSleep())
    for (MinionTask t : {MinionTask::SLEEP, MinionTask::GRAVE})
      if (minionTasks.at(c->getUniqueId()).containsState(t)) {
        minionTasks.at(c->getUniqueId()).setState(t);
        break;
      }
  if (c == keeper && !myTiles.empty() && !myTiles.count(c->getPosition())) {
    if (auto move = c->getMoveTowards(chooseRandom(myTiles)))
      return {1.0, [=] {
        c->move(*move);
      }};
  }
  MinionTaskInfo info = taskInfo.at(minionTasks.at(c->getUniqueId()).getState());
  if (mySquares[info.square].empty()) {
    minionTasks.at(c->getUniqueId()).updateToNext();
    warning[int(info.warning)] = true;
    return NoMove;
  }
  warning[int(info.warning)] = false;
  taskMap.addTask(Task::applySquare(this, mySquares[info.square]), c);
  minionTaskStrings[c->getUniqueId()] = info.desc;
  return taskMap.getTask(c)->getMove(c);
}

bool Collective::underAttack() const {
  for (Vec2 v : myTiles)
    if (const Creature* c = level->getSquare(v)->getCreature())
      if (c->getTribe() != tribe)
        return true;
  return false;
}

Task* Collective::TaskMap::getTaskForImp(Creature* c) {
  Task* closest = nullptr;
  for (PTask& task : tasks) {
    double dist = (task->getPosition() - c->getPosition()).length8();
    if ((!taken.count(task.get()) || (task->canTransfer() 
                                && (task->getPosition() - taken.at(task.get())->getPosition()).length8() > dist))
        && (!closest ||
           dist < (closest->getPosition() - c->getPosition()).length8())
        && !isLocked(c, task.get())
        && (!delayedTasks.count(task->getUniqueId()) || delayedTasks.at(task->getUniqueId()) < c->getTime())) {
      bool valid = task->getMove(c);
      if (valid)
        closest = task.get();
      else
        lock(c, task.get());
    }
  }
  return closest;
}

void Collective::TaskMap::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  taskMap[c] = task;
  taken[task] = c;
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(contains(creatures, c));
  if (!contains(minionByType.at(MinionType::IMP), c)) {
    CHECK(contains(minions, c));
    return getMinionMove(c);
  }
  if (c->getLevel() != level)
    return NoMove;
  if (startImpNum == -1)
    startImpNum = minionByType.at(MinionType::IMP).size();
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  if (Task* closest = taskMap.getTaskForImp(c)) {
    taskMap.takeTask(c, closest);
    return closest->getMove(c);
  } else {
    if (!myTiles.count(c->getPosition()) && keeper->getLevel() == c->getLevel()) {
      Vec2 keeperPos = keeper->getPosition();
      if (keeperPos.dist8(c->getPosition()) < 3)
        return NoMove;
      if (auto move = c->getMoveTowards(keeperPos))
        return {1.0, [=] {
          c->move(*move);
        }};
      else
        return NoMove;
    } else
      return NoMove;
  }
}

MarkovChain<MinionTask> Collective::getTasksForMinion(Creature* c) {
  switch (getMinionType(c)) {
    case MinionType::KEEPER:
      return MarkovChain<MinionTask>(MinionTask::STUDY, {
          {MinionTask::STUDY, {}},
          {MinionTask::SLEEP, {{ MinionTask::STUDY, 1}}}});
    case MinionType::PRISONER:
      return MarkovChain<MinionTask>(MinionTask::PRISON, {
          {MinionTask::PRISON, {}},
          {MinionTask::TORTURE, {{ MinionTask::PRISON, 0.001}}}});
    case MinionType::GOLEM:
      return MarkovChain<MinionTask>(MinionTask::TRAIN, {{MinionTask::TRAIN, {}}});
    case MinionType::UNDEAD:
      return MarkovChain<MinionTask>(MinionTask::GRAVE, {
          {MinionTask::GRAVE, {{ MinionTask::TRAIN, 0.5}}},
          {MinionTask::TRAIN, {{ MinionTask::GRAVE, 0.005}}}});
    case MinionType::NORMAL: {
      double workshopTime = (c->getName() == "gnome" ? 0.6 : 0.2);
      double labTime = (c->getName() == "gnome" ? 0.3 : 0.1);
      double trainTime = 1 - workshopTime - labTime;
      double changeFreq = 0.01;
      return MarkovChain<MinionTask>(MinionTask::SLEEP, {
          {MinionTask::SLEEP, {{ MinionTask::TRAIN, trainTime}, { MinionTask::WORKSHOP, workshopTime},
          {MinionTask::LABORATORY, labTime}}},
          {MinionTask::WORKSHOP, {{ MinionTask::SLEEP, changeFreq}}},
          {MinionTask::LABORATORY, {{ MinionTask::SLEEP, changeFreq}}},
          {MinionTask::TRAIN, {{ MinionTask::SLEEP, changeFreq}}}});
      }
    case MinionType::BEAST:
    case MinionType::IMP: FAIL << "Not handled by this method";
  }
  FAIL <<"pokewf";
  return MarkovChain<MinionTask>(MinionTask::TRAIN, {{MinionTask::TRAIN, {}}});
}

Creature* Collective::addCreature(PCreature creature, Vec2 v, MinionType type) {
  // strip all spawned minions
  creature->getEquipment().removeAllItems();
  addCreature(creature.get(), type);
  Creature* ret = creature.get();
  level->addCreature(v, std::move(creature));
  return ret;
}

void Collective::addCreature(Creature* c, MinionType type) {
  if (keeper == nullptr) {
    keeper = c;
    type = MinionType::KEEPER;
    minionByType[type].push_back(c);
    Vec2 radius(30, 30);
    for (Vec2 pos : Rectangle(c->getPosition() - radius, c->getPosition() + radius))
      if (pos.distD(c->getPosition()) <= radius.x && pos.inRectangle(level->getBounds()) 
          && level->getSquare(pos)->canEnterEmpty(Creature::getDefault()))
        for (Vec2 v : concat({pos}, pos.neighbors8()))
          if (v.inRectangle(level->getBounds()))
            knownTiles[v] = true;
  }
  creatures.push_back(c);
  minionByType[type].push_back(c);
  if (!contains({MinionType::IMP}, type))
    minions.push_back(c);
  if (!contains({MinionType::BEAST, MinionType::IMP}, type))
    minionTasks.insert(make_pair(c->getUniqueId(), getTasksForMinion(c)));
  for (const Item* item : c->getEquipment().getItems())
    minionEquipment.own(c, item);
  if (type == MinionType::PRISONER)
    prisonerInfo[c] = {PrisonerInfo::PRISON, nullptr};
}

void Collective::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == level) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
      }
    if (constructions.count(pos)) {
      ConstructionInfo& info = constructions.at(pos);
      info.marked = getTime() + 10; // wait a little before considering rebuilding
      info.built = false;
      info.task = -1;
    }
  }
}

void Collective::onTriggerEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == level) {
    traps.at(pos).armed = false;
    if (traps.at(pos).type == TrapType::SURPRISE)
      handleSurprise(pos);
  }
}

void Collective::handleSurprise(Vec2 pos) {
  Vec2 rad(8, 8);
  bool wasMsg = false;
  Creature* c = NOTNULL(level->getSquare(pos)->getCreature());
  for (Vec2 v : randomPermutation(Rectangle(pos - rad, pos + rad).getAllSquares()))
    if (Creature* other = level->getSquare(v)->getCreature())
      if (contains(minions, other)
          && !contains({MinionType::IMP, MinionType::KEEPER, MinionType::PRISONER}, getMinionType(other))
          && v.dist8(pos) > 1) {
        for (Vec2 dest : pos.neighbors8(true))
          if (level->canMoveCreature(other, dest - v)) {
            level->moveCreature(other, dest - v);
            other->privateMessage("Time for a welcoming committee");
            if (!wasMsg) {
              c->privateMessage("Surprise!");
              wasMsg = true;
            }
            break;
          }
      }
}

void Collective::onConqueredLand(const string& name) {
  if (retired)
    return;
  model->conquered(*keeper->getFirstName() + " the Keeper", name, kills, getDangerLevel() + points);
}

void Collective::onCombatEvent(const Creature* c) {
  CHECK(c != nullptr);
  if (contains(minions, c))
    lastCombat[c] = c->getTime();
}

bool Collective::isInCombat(const Creature* c) const {
  double timeout = 5;
  return lastCombat.count(c) && lastCombat.at(c) > c->getTime() -5;
}

void Collective::TaskMap::freeTask(Task* task) {
  if (taken.count(task)) {
    taskMap.erase(taken.at(task));
    taken.erase(task);
  }
}

void Collective::TaskMap::freeTaskDelay(Task* t, double d) {
  freeTask(t);
  delayedTasks[t->getUniqueId()] = d;
}

void Collective::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim == keeper && !retired) {
    model->gameOver(keeper, kills.size(), "enemies", getDangerLevel() + points);
  }
  if (contains(creatures, victim)) {
    Creature* c = const_cast<Creature*>(victim);
    removeElement(creatures, c);
    if (getMinionType(victim) == MinionType::PRISONER && killer && contains(creatures, killer))
      ++executions;
    prisonerInfo.erase(c);
    for (auto& elem : guardPosts)
      if (elem.second.attender == c)
        elem.second.attender = nullptr;
    if (contains(team, c)) {
      removeElement(team, c);
      if (team.empty())
        gatheringTeam = false;
    }
    if (Task* task = taskMap.getTask(c)) {
      if (!task->canTransfer()) {
        task->cancel();
        returnGold(taskMap.removeTask(task));
      } else
        taskMap.freeTaskDelay(task, getTime() + 50);
    }
    if (contains(minions, c))
      removeElement(minions, c);
    removeElement(minionByType.at(getMinionType(c)), c);
  } else if (victim->getTribe() != tribe && (!killer || killer->getTribe() == tribe)) {
    double incMana = victim->getDifficultyPoints() / 3;
    mana += incMana;
    kills.push_back(victim);
    points += victim->getDifficultyPoints();
    Debug() << "Mana increase " << incMana << " from " << victim->getName();
    keeper->increaseExpLevel(double(victim->getDifficultyPoints()) / 200);
  }
}
  
const Level* Collective::getLevel() const {
  return level;
}
