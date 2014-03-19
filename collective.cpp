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
    & BOOST_SERIALIZATION_NVP(credit)
    & BOOST_SERIALIZATION_NVP(technologies)
    & BOOST_SERIALIZATION_NVP(creatures)
    & BOOST_SERIALIZATION_NVP(minions)
    & BOOST_SERIALIZATION_NVP(imps)
    & BOOST_SERIALIZATION_NVP(minionByType)
    & BOOST_SERIALIZATION_NVP(markedItems)
    & BOOST_SERIALIZATION_NVP(taskMap)
    & BOOST_SERIALIZATION_NVP(traps)
    & BOOST_SERIALIZATION_NVP(trapMap)
    & BOOST_SERIALIZATION_NVP(doors)
    & BOOST_SERIALIZATION_NVP(minionTasks)
    & BOOST_SERIALIZATION_NVP(minionTaskStrings)
    & BOOST_SERIALIZATION_NVP(mySquares)
    & BOOST_SERIALIZATION_NVP(myTiles)
    & BOOST_SERIALIZATION_NVP(level)
    & BOOST_SERIALIZATION_NVP(keeper)
    & BOOST_SERIALIZATION_NVP(memory)
    & BOOST_SERIALIZATION_NVP(team)
    & BOOST_SERIALIZATION_NVP(teamLevelChanges)
    & BOOST_SERIALIZATION_NVP(levelChangeHistory)
    & BOOST_SERIALIZATION_NVP(possessed)
    & BOOST_SERIALIZATION_NVP(minionEquipment)
    & BOOST_SERIALIZATION_NVP(guardPosts)
    & BOOST_SERIALIZATION_NVP(mana)
    & BOOST_SERIALIZATION_NVP(points)
    & BOOST_SERIALIZATION_NVP(model)
    & BOOST_SERIALIZATION_NVP(kills)
    & BOOST_SERIALIZATION_NVP(showWelcomeMsg)
    & BOOST_SERIALIZATION_NVP(delayedPos)
    & BOOST_SERIALIZATION_NVP(startImpNum)
    & BOOST_SERIALIZATION_NVP(retired)
    & BOOST_SERIALIZATION_NVP(tribe)
    & BOOST_SERIALIZATION_NVP(alarmInfo.finishTime)
    & BOOST_SERIALIZATION_NVP(alarmInfo.position);
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
void Collective::TrapInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(armed)
    & BOOST_SERIALIZATION_NVP(marked);
}

SERIALIZABLE(Collective::TrapInfo);

template <class Archive>
void Collective::DoorInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(cost)
    & BOOST_SERIALIZATION_NVP(built)
    & BOOST_SERIALIZATION_NVP(marked);
}

SERIALIZABLE(Collective::DoorInfo);

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

Collective::BuildInfo::BuildInfo(SquareInfo info, Optional<TechId> id, const string& h)
    : squareInfo(info), buildType(SQUARE), techId(id), help(h) {}
Collective::BuildInfo::BuildInfo(TrapInfo info, Optional<TechId> id, const string& h)
    : trapInfo(info), buildType(TRAP), techId(id), help(h) {}
Collective::BuildInfo::BuildInfo(DoorInfo info, Optional<TechId> id, const string& h)
    : doorInfo(info), buildType(DOOR), techId(id), help(h) {}
Collective::BuildInfo::BuildInfo(BuildType type, const string& h) : buildType(type), help(h) {
  CHECK(contains({DIG, IMP, GUARD_POST, DESTROY}, type));
}

const vector<Collective::BuildInfo> Collective::buildInfo {
    BuildInfo(BuildInfo::DIG),
    BuildInfo({SquareType::STOCKPILE, ResourceId::GOLD, 0, "Storage"}),
    BuildInfo({SquareType::TREASURE_CHEST, ResourceId::WOOD, 5, "Treasure room"}),
    BuildInfo({SquareType::BED, ResourceId::WOOD, 10, "Bed"}),
    BuildInfo({SquareType::TRAINING_DUMMY, ResourceId::IRON, 20, "Training room"}),
    BuildInfo({SquareType::LIBRARY, ResourceId::WOOD, 20, "Library"}),
    BuildInfo({SquareType::LABORATORY, ResourceId::STONE, 15, "Laboratory"}, TechId::ALCHEMY),
    BuildInfo({SquareType::WORKSHOP, ResourceId::IRON, 15, "Workshop"}, TechId::CRAFTING),
    BuildInfo({SquareType::ANIMAL_TRAP, ResourceId::WOOD, 12, "Beast cage"}, Nothing(), "Place it in the forest."),
    BuildInfo({SquareType::GRAVE, ResourceId::STONE, 20, "Graveyard"}, TechId::NECRO),
    BuildInfo(BuildInfo::DESTROY),
    BuildInfo(BuildInfo::IMP),
    BuildInfo(BuildInfo::GUARD_POST, "Place it anywhere to send a minion."),
};

const vector<Collective::BuildInfo> Collective::workshopInfo {
    BuildInfo({ResourceId::WOOD, 5, "Door", ViewId::DOOR}, TechId::CRAFTING),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}, TechId::TRAPS),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}, TechId::TRAPS),
    BuildInfo({TrapType::ALARM, "Alarm trap", ViewId::ALARM_TRAP}, TechId::TRAPS),
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


vector<MinionType> minionTypes {
  MinionType::IMP,
  MinionType::NORMAL,
  MinionType::UNDEAD,
  MinionType::GOLEM,
  MinionType::BEAST,
};

Collective::ResourceInfo info;

constexpr const char* const Collective::warningText[numWarnings];

const map<Collective::ResourceId, Collective::ResourceInfo> Collective::resourceInfo {
  {ResourceId::GOLD, { SquareType::TREASURE_CHEST, Item::typePredicate(ItemType::GOLD), ItemId::GOLD_PIECE, "gold"}},
  {ResourceId::WOOD, { SquareType::STOCKPILE, Item::namePredicate("wood plank"), ItemId::WOOD_PLANK, "wood"}},
  {ResourceId::IRON, { SquareType::STOCKPILE, Item::namePredicate("iron ore"), ItemId::IRON_ORE, "iron"}},
  {ResourceId::STONE, { SquareType::STOCKPILE, Item::namePredicate("rock"), ItemId::ROCK, "stone"}},
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
};

Collective::Collective(Model* m, Tribe* t) : mana(200), model(m), tribe(t) {
  memory.reset(new map<const Level*, MapMemory>);
  // init the map so the values can be safely read with .at()
  mySquares[SquareType::TREE_TRUNK].clear();
  mySquares[SquareType::FLOOR].clear();
  for (BuildInfo info : buildInfo)
    if (info.buildType == BuildInfo::SQUARE)
      mySquares[info.squareInfo.type].clear();
    else if (info.buildType == BuildInfo::TRAP)
      trapMap[info.trapInfo.type].clear();
  credit = {
    {ResourceId::GOLD, 100},
    {ResourceId::WOOD, 0},
    {ResourceId::IRON, 0},
    {ResourceId::STONE, 0},
  };
  for (MinionType t : minionTypes)
    minionByType[t].clear();
}


const int basicImpCost = 20;
const int minionLimit = 20;

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
  if (possessed && possessed != keeper && isInCombat(keeper) && lastControlKeeperQuestion < getTime() - 50) {
    lastControlKeeperQuestion = getTime();
    if (view->yesOrNoPrompt("The keeper is engaged in combat. Do you want to control him?")) {
      possessed->popController();
      possess(keeper, view);
      return;
    }
  }
  if (possessed && (!possessed->isPlayer() || possessed->isDead())) {
 /*   if (contains(team, possessed))
      removeElement(team, possessed);*/
    if ((possessed->isDead() || possessed->isSleeping()) && !team.empty()) {
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
    view->presentText("Welcome", "In short: you are a warlock who has been banished from the lawful world for practicing black magic. You are going to claim the land of " + NameGenerator::worldNames.getNext() + " and make it your domain. Best way to achieve this is to kill all the village leaders.\n \n"
"Use the mouse to dig into the mountain. You will need access to trees, iron and gold ore. Build rooms and traps and prepare for war. You can control a minion at any time by clicking on them in the minions tab.\n \n You can turn these messages off in the options (press F2).");
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

void Collective::minionView(View* view, Creature* creature, int prevIndex) {
  auto index = view->chooseFromList(capitalFirst(creature->getNameAndTitle()) 
      + ", level " + convertToString(creature->getExpLevel()),
      {"Possess", "Manage equipment", "Information"}, prevIndex);
  if (!index)
    return;
  switch (*index) {
    case 0: possess(creature, view); return;
    case 1: handleEquipment(view, creature); break;
    case 2: messageBuffer.addMessage(MessageBuffer::important(creature->getDescription())); break;
  }
  minionView(view, creature, *index);
}

void Collective::autoEquipment(Creature* creature) {
  if (!creature->isHumanoid())
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
    list.push_back(View::ListElem(Equipment::slotTitles.at(slot), View::TITLE));
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
    if (item)
      list.push_back(item->getNameAndModifiers() + (creature->getEquipment().isEquiped(item) 
            ? " (equiped)" : " (pending)"));
    else
      list.push_back("[Nothing]");
  }
  Optional<int> newIndex = model->getView()->chooseFromList(creature->getName() + "'s equipment", list, prevItem);
  if (!newIndex)
    return;
  int index = *newIndex;
  if (Item* item = slotItems[index])
    minionEquipment.discard(item);
  else {
    Item* currentItem = creature->getEquipment().getItem(slots[index]);
    const Item* chosenItem = chooseEquipmentItem(view, currentItem, [&](const Item* it) {
        return minionEquipment.getOwner(it) != creature
            && creature->canEquipIfEmptySlot(it) && it->getEquipmentSlot() == slots[index]; });
    if (chosenItem) {
      minionEquipment.own(creature, chosenItem);
    }
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

const Item* Collective::chooseEquipmentItem(View* view, Item* currentItem, ItemPredicate predicate) const {
  vector<View::ListElem> options;
  vector<Item*> currentItemVec;
  if (currentItem) {
    currentItemVec.push_back(currentItem);
    options.push_back(View::ListElem("Currently equiped: ", View::TITLE));
    options.push_back(currentItem->getNameAndModifiers());
  }
  options.emplace_back("Free:", View::TITLE);
  vector<View::ListElem> options2 { View::ListElem("Used:", View::TITLE) };
  vector<Item*> availableItems;
  vector<Item*> usedItems;
  for (Item* item : getAllItems(predicate))
    if (item != currentItem) {
      if (const Creature* c = minionEquipment.getOwner(item)) {
        options2.emplace_back(item->getNameAndModifiers() + " owned by " + c->getAName());
        usedItems.push_back(item);
      } else
        availableItems.push_back(item);
    }
  vector<Item*> availableStacked;
  for (auto elem : Item::stackItems(availableItems)) {
    options.emplace_back(elem.first);
    availableStacked.push_back(elem.second.front());
  }
  auto index = view->chooseFromList("Choose an item to equip: ", concat(options, options2));
  if (!index)
    return nullptr;
  return concat<Item*>({currentItemVec, availableStacked, usedItems})[*index];
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
  {CreatureId::VAMPIRE, 50, TechId::NECRO_ADV},
  {CreatureId::VAMPIRE_LORD, 100, TechId::NECRO_ADV},
};

vector<Collective::SpawnInfo> animationInfo {
  {CreatureId::STONE_GOLEM, 50, TechId::GOLEM},
  {CreatureId::IRON_GOLEM, 50, TechId::GOLEM_ADV},
  {CreatureId::LAVA_GOLEM, 100, TechId::GOLEM_ADV},
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
  {CreatureId::SPECIAL_MONSTER_KEEPER, 100, TechId::BEAST_MUT},
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
  {CreatureId::SPECIAL_HUMANOID, 100, TechId::HUMANOID_MUT},
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

void Collective::handleSpawning(View* view, SquareType spawnSquare, const string& info1, const string& info2,
    const string& title, MinionType minionType, vector<SpawnInfo> spawnInfo) {
  set<Vec2> cages = mySquares.at(spawnSquare);
  int prevItem = 0;
  bool allInactive = false;
  while (1) {
    vector<View::ListElem> options;
    if (minions.size() >= minionLimit) {
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
  if (minions.size() >= minionLimit) {
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
  for (Vec2 v : elem.first.neighbors8(true))
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
            if (elem.cost > 0)
              cost = {getResourceViewObject(elem.resourceId), elem.cost};
            buttons.push_back({
                SquareFactory::get(elem.type)->getViewObject(),
                elem.name,
                cost,
                (elem.cost > 0 ? "[" + convertToString(mySquares.at(elem.type).size()) + "]" : ""),
                elem.cost <= numGold(elem.resourceId) && isTech });
           }
           break;
      case BuildInfo::DIG: {
             buttons.push_back({
                 ViewObject(ViewId::DIG_ICON, ViewLayer::LARGE_ITEM, ""),
                 "dig or cut tree", Nothing(), "", true});
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
                 numTraps > 0 && isTech});
           }
           break;
      case BuildInfo::DOOR: {
             BuildInfo::DoorInfo& elem = button.doorInfo;
             pair<ViewObject, int> cost = {getResourceViewObject(elem.resourceId), elem.cost};
             buttons.push_back({
                 ViewObject(elem.viewId, ViewLayer::LARGE_ITEM, ""),
                 elem.name,
                 cost,
                 "[" + convertToString(doors.size()) + "]",
                 elem.cost <= numGold(elem.resourceId) && isTech});
           }
           break;
      case BuildInfo::IMP: {
           pair<ViewObject, int> cost = {ViewObject::mana(), getImpCost()};
           buttons.push_back({
               ViewObject(ViewId::IMP, ViewLayer::CREATURE, ""),
               "Imp",
               cost,
               "[" + convertToString(imps.size()) + "]",
               getImpCost() <= mana});
           break; }
      case BuildInfo::DESTROY:
           buttons.push_back({
               ViewObject(ViewId::DESTROY_BUTTON, ViewLayer::CREATURE, ""), "Remove construction", Nothing(), "",
                   true});
           break;
      case BuildInfo::GUARD_POST:
           buttons.push_back({
               ViewObject(ViewId::GUARD_POST, ViewLayer::CREATURE, ""), "Guard post", Nothing(), "", true});
           break;
    }
    if (!isTech)
      buttons.back().help = "Requires " + Technology::get(*button.techId)->getName();
    if (buttons.back().help.empty())
      buttons.back().help = button.help;
  }
  return buttons;
}

vector<Collective::TechInfo> Collective::getTechInfo() const {
  vector<TechInfo> ret;
  ret.push_back({{ViewId::BILE_DEMON, "Humanoids"},
      [this](View* view) { handleHumanoidBreeding(view); }});
  ret.push_back({{ViewId::BEAR, "Beasts"},
      [this](View* view) { handleBeastTaming(view); }}); 
  if (hasTech(TechId::GOLEM))
    ret.push_back({{ViewId::IRON_GOLEM, "Golems"},
      [this](View* view) { handleMatterAnimation(view); }});      
  if (hasTech(TechId::NECRO))
    ret.push_back({{ViewId::VAMPIRE, "Necromancy"},
      [this](View* view) { handleNecromancy(view); }});
  ret.push_back({{ViewId::MANA, "Sorcery"}, [this](View* view) {handlePersonalSpells(view);}});
  ret.push_back({{ViewId::EMPTY, ""}, [](View*) {}});
  ret.push_back({{ViewId::LIBRARY, "Library"},
      [this](View* view) { handleLibrary(view); }});
  ret.push_back({{ViewId::GOLD, "Black market"},
      [this](View* view) { handleMarket(view); }});
  ret.push_back({{ViewId::EMPTY, ""}, [](View*) {}});
  ret.push_back({{ViewId::BOOK, "Keeperopedia"},
      [this](View* view) { model->keeperopedia.present(view); }});
  return ret;
}

void Collective::refreshGameInfo(View::GameInfo& gameInfo) const {
  gameInfo.infoType = View::GameInfo::InfoType::BAND;
  View::GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.buildings = fillButtons(buildInfo);
  info.workshop = fillButtons(workshopInfo);
  info.tasks = minionTaskStrings;
  for (Creature* c : minions)
    if (isInCombat(c))
      info.tasks[c->getUniqueId()] = "fighting";
  info.monsterHeader = "Monsters: " + convertToString(minions.size()) + " / " + convertToString(minionLimit);
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

const MapMemory& Collective::getMemory(const Level* l) const {
  return (*memory.get())[l];
}

MapMemory& Collective::getMemory(const Level* l) {
  return (*memory.get())[l];
}

static ViewObject getTrapObject(TrapType type) {
  switch (type) {
    case TrapType::BOULDER: return ViewObject(ViewId::UNARMED_BOULDER_TRAP, ViewLayer::LARGE_ITEM, "Unarmed trap");
    case TrapType::POISON_GAS: return ViewObject(ViewId::UNARMED_GAS_TRAP, ViewLayer::LARGE_ITEM, "Unarmed trap");
    case TrapType::ALARM: return ViewObject(ViewId::UNARMED_ALARM_TRAP, ViewLayer::LARGE_ITEM, "Unarmed trap");
  }
  FAIL << "pofke";
  return ViewObject(ViewId::UNARMED_GAS_TRAP, ViewLayer::LARGE_ITEM, "Unarmed trap");
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
    if (doors.count(pos))
      index.insert(ViewObject(ViewId::PLANNED_DOOR, ViewLayer::LARGE_ITEM, "Planned door"));
  }
  if (const Location* loc = level->getLocation(pos)) {
    if (loc->isMarkedAsSurprise() && loc->getBounds().middle() == pos && !getMemory(level).hasViewIndex(pos))
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

void Collective::TaskMap::addTask(PTask task, const Creature* c) {
  taken[task.get()] = c;
  taskMap[c] = task.get();
  addTask(std::move(task));
}

void Collective::TaskMap::addTask(PTask task) {
  tasks.push_back(std::move(task));
}

void Collective::TaskMap::removeTask(Task* task) {
  completionCost.erase(task);
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

void Collective::TaskMap::markSquare(Vec2 pos, PTask task, CostInfo cost) {
  addTask(std::move(task));
  marked[pos] = tasks.back().get();
  if (cost.value)
    completionCost[tasks.back().get()] = cost;
}

void Collective::TaskMap::unmarkSquare(Vec2 pos) {
  Task* t = marked.at(pos);
  removeTask(t);
  marked.erase(pos);
}

Collective::CostInfo Collective::TaskMap::getCompletionCost(Vec2 pos) {
  if (marked.count(pos) && completionCost.count(marked.at(pos)))
    return completionCost.at(marked.at(pos));
  else
    return {ResourceId::GOLD, 0};
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
  if (imps.size() < startImpNum)
    return 0;
  return basicImpCost * pow(2, double(imps.size() - startImpNum) / 5);
}

void Collective::possess(const Creature* cr, View* view) {
  view->stopClock();
  CHECK(contains(creatures, cr));
  CHECK(!cr->isDead());
  Creature* c = const_cast<Creature*>(cr);
  if (c->isSleeping())
    c->wakeUp();
  freeFromGuardPost(c);
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
      level->getSquare(pos)->canEnterEmpty(Creature::getDefault()) && getMemory(level).hasViewIndex(pos);
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
  if (!possessed && (isInCombat(keeper) || keeper->getHealth() < 1) && lastControlKeeperQuestion < getTime() - 50) {
    lastControlKeeperQuestion = getTime();
    if (view->yesOrNoPrompt("The keeper is engaged in combat. Do you want to control him?")) {
      possess(keeper, view);
      return;
    }
  }
  switch (action.getType()) {
    case CollectiveAction::GATHER_TEAM:
        if (gatheringTeam && !team.empty()) {
          possess(team[0], view);
 //         gatheringTeam = false;
          for (Creature* c : team) {
            freeFromGuardPost(c);
            if (c->isSleeping())
              c->wakeUp();
          }
        } else
          gatheringTeam = true;
        break;
    case CollectiveAction::DRAW_LEVEL_MAP: view->drawLevelMap(level, this); break;
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
            possess(c, view);
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
        if (getTrapItems(trapType).size() > 0 && canPlacePost(pos) && myTiles.count(pos)) {
          traps[pos] = {trapType, false, 0};
          trapMap[trapType].push_back(pos);
          updateTraps();
        }
      }
      break;
    case BuildInfo::DOOR: {
        BuildInfo::DoorInfo info = building.doorInfo;
        if (numGold(info.resourceId) >= info.cost && canBuildDoor(pos)){
          doors[pos] = {{info.resourceId, info.cost}, false, false};
          updateTraps();
        }
      }
      break;
    case BuildInfo::DESTROY:
        selection = SELECT;
        if (level->getSquare(pos)->canDestroy() && myTiles.count(pos))
          level->getSquare(pos)->destroy(10000);
        level->getSquare(pos)->removeTriggers();
        if (Creature* c = level->getSquare(pos)->getCreature())
          if (c->getName() == "boulder")
            c->die(nullptr, false);
        if (traps.count(pos)) {
          removeElement(trapMap.at(traps.at(pos).type), pos);
          traps.erase(pos);
        }
        if (doors.count(pos))
          doors.erase(pos);
        break;
    case BuildInfo::GUARD_POST:
        if (guardPosts.count(pos) && selection != SELECT) {
          guardPosts.erase(pos);
          selection = DESELECT;
        }
        else if (canPlacePost(pos) && guardPosts.size() < minions.size() && selection != DESELECT) {
          guardPosts[pos] = {nullptr};
          selection = SELECT;
        }
        break;
    case BuildInfo::DIG:
        if (taskMap.isMarked(pos) && selection != SELECT) {
          returnGold(taskMap.getCompletionCost(pos));
          taskMap.unmarkSquare(pos);
          selection = DESELECT;
        } else
          if (!taskMap.isMarked(pos) && selection != DESELECT) {
            if (level->getSquare(pos)->canConstruct(SquareType::TREE_TRUNK)) {
              taskMap.markSquare(pos, Task::construction(this, pos, SquareType::TREE_TRUNK), {ResourceId::GOLD, 0});
              selection = SELECT;
            } else
              if (level->getSquare(pos)->canConstruct(SquareType::FLOOR) || !getMemory(level).hasViewIndex(pos)) {
                taskMap.markSquare(pos, Task::construction(this, pos, SquareType::FLOOR), { ResourceId::GOLD, 0});
                selection = SELECT;
              }
          }
        break;
    case BuildInfo::SQUARE:
        if (taskMap.isMarked(pos) && selection != SELECT) {
          returnGold(taskMap.getCompletionCost(pos));
          taskMap.unmarkSquare(pos);
          selection = DESELECT;
        } else {
          BuildInfo::SquareInfo info = building.squareInfo;
          bool diggingSquare = !getMemory(level).hasViewIndex(pos) ||
            (level->getSquare(pos)->canConstruct(info.type));
          if (!taskMap.isMarked(pos) && selection != DESELECT && diggingSquare && 
              numGold(info.resourceId) >= info.cost && 
              (info.type != SquareType::TRIBE_DOOR || canBuildDoor(pos)) &&
              (info.type == SquareType::FLOOR || canSee(pos))) {
            taskMap.markSquare(pos, Task::construction(this, pos, info.type), {info.resourceId, info.cost});
            selection = SELECT;
            takeGold({info.resourceId, info.cost});
          }
        }
        break;
  }
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
  if (contains({SquareType::TRIBE_DOOR}, type) && doors.count(pos)) {
    doors.at(pos).built = true;
    doors.at(pos).marked = 0;
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

void Collective::onAppliedSquare(Vec2 pos) {
  if (mySquares.at(SquareType::LIBRARY).count(pos)) {
    mana += 0.3 + max(0., 2 - double(getDangerLevel()) / 500);
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
      vector<PItem> items  = ItemFactory::workshop(technologies).random();
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

double Collective::getDangerLevel() const {
  double ret = 0;
  for (const Creature* c : minions)
    ret += c->getDifficultyPoints();
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

void Collective::addToMemory(Vec2 pos, const Creature* c) {
  if (!c) {
    getMemory(level).addObject(pos, level->getSquare(pos)->getViewObject());
    if (auto obj = level->getSquare(pos)->getBackgroundObject())
      getMemory(level).addObject(pos, *obj);
  }
  else {
    ViewIndex index = level->getSquare(pos)->getViewIndex(c);
    getMemory(level).clearSquare(pos);
    for (ViewLayer l : { ViewLayer::ITEM, ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM})
      if (index.hasObject(l))
        getMemory(level).addObject(pos, index.getObject(l));
  }
}

void Collective::update(Creature* c) {
  if (!contains(creatures, c) || c->getLevel() != level)
    return;
  for (Vec2 pos : level->getVisibleTiles(c))
    addToMemory(pos, c);
}

bool Collective::isDownstairsVisible() const {
  vector<Vec2> v = level->getLandingSquares(StairDirection::DOWN, StairKey::DWARF);
  return v.size() == 1 && getMemory(level).hasViewIndex(v[0]);
}

// after this time applying trap or building door is rescheduled (imp death, etc).
const static int timeToBuild = 50;

void Collective::updateTraps() {
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems {
    {TrapType::BOULDER, getTrapItems(TrapType::BOULDER, myTiles)},
    {TrapType::POISON_GAS, getTrapItems(TrapType::POISON_GAS, myTiles)},
    {TrapType::ALARM, getTrapItems(TrapType::ALARM, myTiles)}};
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
  for (auto& elem : doors)
    if (!isDelayed(elem.first)
        && elem.second.marked <= getTime() 
        && !elem.second.built && numGold(elem.second.cost.id) >= elem.second.cost.value) {
      taskMap.addTask(Task::construction(this, elem.first, SquareType::TRIBE_DOOR));
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
          level->getSquare(v)->canEnterEmpty(Creature::getDefault()) && myTiles.count(v)) {
        dist[v] = dist[pos] + 1;
        q.push(v);
      }
  }
}

bool Collective::isDelayed(Vec2 pos) {
  return delayedPos.count(pos) && delayedPos.at(pos) > getTime();
}

void Collective::tick() {
  if (retired)
    if (const Creature* c = level->getPlayer())
      if (Random.roll(30) && !myTiles.count(c->getPosition()))
        c->privateMessage("You sense horrible evil in the " + 
            getCardinalName((keeper->getPosition() - c->getPosition()).getBearing().getCardinalDir()));
  warning[int(Warning::MANA)] = mana < 100;
  warning[int(Warning::WOOD)] = numGold(ResourceId::WOOD) == 0;
  warning[int(Warning::DIGGING)] = mySquares.at(SquareType::FLOOR).empty();
  warning[int(Warning::MINIONS)] = minions.size() <= 1;
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
  updateTraps();
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
  return getMemory(level).hasViewIndex(position);
}

void Collective::setLevel(Level* l) {
  for (Vec2 v : l->getBounds())
    if (contains({"gold ore", "iron ore", "stone"}, l->getSquare(v)->getName()))
      getMemory(l).addObject(v, l->getSquare(v)->getViewObject());
  level = l;
}

vector<const Creature*> Collective::getUnknownAttacker() const {
  return {};
}

Tribe* Collective::getTribe() const {
  return tribe;
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
      if (c->isSleeping())
        c->wakeUp();
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

MoveInfo Collective::getBeastMove(Creature* c) {
  if (!Random.roll(5))
    return NoMove;
  Vec2 radius(7, 7);
  for (Vec2 v : randomPermutation(Rectangle(c->getPosition() - radius, c->getPosition() + radius).getAllSquares()))
    if (v.inRectangle(level->getBounds()) && !getMemory(level).hasViewIndex(v)) {
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
  if (possessed && contains(team, c)) {
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
    if (v) {
      if (auto move = c->getMoveTowards(*v))
        return {1.0, [=] {
          c->move(*move);
        }};
      else
        return NoMove;
    }
  }
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

MoveInfo Collective::getMinionMove(Creature* c) {
  if (MoveInfo possessedMove = getPossessedMove(c))
    return possessedMove;
  if (c->getLevel() != level)
    return getBacktrackMove(c);
  if (c != keeper)
    if (MoveInfo alarmMove = getAlarmMove(c))
      return alarmMove;
  if (MoveInfo dropMove = getDropItems(c))
    return dropMove;
  if (contains(minionByType.at(MinionType::BEAST), c))
    return getBeastMove(c);
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
           dist < (closest->getPosition() - c->getPosition()).length8()) && !lockedTasks.count(make_pair(c, task->getUniqueId()))) {
      bool valid = task->getMove(c);
      if (valid)
        closest = task.get();
      else
        lockedTasks.insert(make_pair(c, task->getUniqueId()));
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
  if (!contains(imps, c)) {
    CHECK(contains(minions, c));
    return getMinionMove(c);
  }
  if (c->getLevel() != level)
    return NoMove;
  if (startImpNum == -1)
    startImpNum = imps.size();
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
  MinionTask t1;
  if (c == keeper)
    return MarkovChain<MinionTask>(MinionTask::STUDY, {
      {MinionTask::STUDY, {}},
      {MinionTask::SLEEP, {{ MinionTask::STUDY, 1}}}});
  if (contains(minionByType.at(MinionType::GOLEM), c))
    return MarkovChain<MinionTask>(MinionTask::TRAIN, {
      {MinionTask::TRAIN, {}}});
  if (contains(minionByType.at(MinionType::UNDEAD), c))
    return MarkovChain<MinionTask>(MinionTask::GRAVE, {
      {MinionTask::GRAVE, {{ MinionTask::TRAIN, 0.5}}},
      {MinionTask::TRAIN, {{ MinionTask::GRAVE, 0.005}}}});
  if (contains(minionByType.at(MinionType::NORMAL), c)) {
    double workshopTime = (c->getName() == "gnome" ? 0.5 : 0.3);
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

  return MarkovChain<MinionTask>(MinionTask::SLEEP, {
      {MinionTask::SLEEP, {{ MinionTask::TRAIN, 0.5}}},
      {MinionTask::TRAIN, {{ MinionTask::SLEEP, 0.005}}}});
}

void Collective::addCreature(PCreature creature, Vec2 v, MinionType type) {
  // strip all spawned minions
  creature->getEquipment().removeAllItems();
  addCreature(creature.get(), type);
  level->addCreature(v, std::move(creature));
}

void Collective::addCreature(Creature* c, MinionType type) {
  if (keeper == nullptr) {
    keeper = c;
    type = MinionType::KEEPER;
    Vec2 radius(30, 30);
    for (Vec2 pos : Rectangle(c->getPosition() - radius, c->getPosition() + radius))
      if (pos.distD(c->getPosition()) <= radius.x && pos.inRectangle(level->getBounds()) 
          && level->getSquare(pos)->canEnterEmpty(Creature::getDefault()))
        for (Vec2 v : concat({pos}, pos.neighbors8()))
          if (v.inRectangle(level->getBounds()))
            addToMemory(v, nullptr);
  }
  creatures.push_back(c);
  if (type != MinionType::IMP) {
    minions.push_back(c);
    minionByType[type].push_back(c);
    minionTasks.insert(make_pair(c->getUniqueId(), getTasksForMinion(c)));
  } else {
    imps.push_back(c);
  }
  for (const Item* item : c->getEquipment().getItems())
    minionEquipment.own(c, item);
}

void Collective::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == level) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
      }
    if (doors.count(pos)) {
      DoorInfo& info = doors.at(pos);
      info.marked = getTime() + 10; // wait a little before considering rebuilding
      info.built = false;
    }
  }
}

void Collective::onTriggerEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == level)
    traps.at(pos).armed = false;
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

void Collective::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim == keeper && !retired) {
    model->gameOver(keeper, kills.size(), "enemies", getDangerLevel() + points);
  }
  if (contains(creatures, victim)) {
    Creature* c = const_cast<Creature*>(victim);
    removeElement(creatures, c);
    for (auto& elem : guardPosts)
      if (elem.second.attender == c)
        elem.second.attender = nullptr;
    if (contains(team, c))
      removeElement(team, c);
    if (Task* task = taskMap.getTask(c)) {
      if (!task->canTransfer()) {
        task->cancel();
        taskMap.removeTask(task);
      } else
        taskMap.freeTask(task);
    }
    if (contains(imps, c))
      removeElement(imps, c);
    if (contains(minions, c))
      removeElement(minions, c);
    for (MinionType type : minionTypes)
      if (contains(minionByType.at(type), c))
        removeElement(minionByType.at(type), c);
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
