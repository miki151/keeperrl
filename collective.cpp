#include "stdafx.h"

#include "collective.h"
#include "level.h"
#include "task.h"
#include "player.h"
#include "message_buffer.h"
#include "model.h"
#include "statistics.h"
#include "options.h"

template <class Archive> 
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CreatureView)
    & SUBCLASS(EventListener)
    & BOOST_SERIALIZATION_NVP(credit)
    & BOOST_SERIALIZATION_NVP(techLevels)
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
    & BOOST_SERIALIZATION_NVP(showWelcomeMsg);
}

SERIALIZABLE(Collective);

template <class Archive>
void Collective::TaskMap::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(tasks)
    & BOOST_SERIALIZATION_NVP(marked)
    & BOOST_SERIALIZATION_NVP(taken)
    & BOOST_SERIALIZATION_NVP(taskMap)
    & BOOST_SERIALIZATION_NVP(delayed)
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

const vector<Collective::BuildInfo> Collective::buildInfo {
    BuildInfo(BuildInfo::DIG),
    BuildInfo({SquareType::STOCKPILE, ResourceId::GOLD, 0, "Storage"}),
    BuildInfo({SquareType::TREASURE_CHEST, ResourceId::WOOD, 5, "Treasure room"}),
    BuildInfo({ResourceId::WOOD, 5, "Door", ViewId::DOOR}),
    BuildInfo({SquareType::BED, ResourceId::WOOD, 10, "Bed"}),
    BuildInfo({SquareType::TRAINING_DUMMY, ResourceId::IRON, 20, "Training room"}),
    BuildInfo({SquareType::LIBRARY, ResourceId::WOOD, 20, "Library"}),
    BuildInfo({SquareType::LABORATORY, ResourceId::STONE, 15, "Laboratory"}),
    BuildInfo({SquareType::WORKSHOP, ResourceId::IRON, 15, "Workshop"}),
    BuildInfo({SquareType::ANIMAL_TRAP, ResourceId::WOOD, 12, "Beast cage"}, "Place it in the forest."),
    BuildInfo({SquareType::GRAVE, ResourceId::STONE, 20, "Graveyard"}),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}),
    BuildInfo(BuildInfo::DESTROY),
    BuildInfo(BuildInfo::IMP),
    BuildInfo(BuildInfo::GUARD_POST, "Place it anywhere to send a minion."),
};

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

vector<TechId> techIds {
    TechId::HUMANOID_BREEDING,
    TechId::NECROMANCY,
    TechId::BEAST_TAMING,
    TechId::MATTER_ANIMATION,
    TechId::SPELLCASTING};

vector<Collective::ItemFetchInfo> Collective::getFetchInfo() const {
  return {
    {unMarkedItems(ItemType::CORPSE), SquareType::GRAVE, true, {}, Warning::GRAVES},
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

Collective::Collective(Model* m) : mana(200), model(m) {
  memory = new map<const Level*, MapMemory>;
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
  for (TechId id: techIds)
    techLevels[id] = 0;
  for (MinionType t : minionTypes)
    minionByType[t].clear();
}


const int basicImpCost = 20;
int startImpNum = -1;
const int minionLimit = 20;


void Collective::render(View* view) {
  if (possessed && possessed != keeper && isInCombat(keeper) && lastControlKeeperQuestion < keeper->getTime() - 50) {
    lastControlKeeperQuestion = keeper->getTime();
    if (view->yesOrNoPrompt("The keeper is engaged in combat. Do you want to control him?")) {
      possessed->popController();
      possess(keeper, view);
      return;
    }
  }
  if (possessed && (!possessed->isPlayer() || possessed->isDead())) {
    if (contains(team, possessed))
      removeElement(team, possessed);
    if ((possessed->isDead() || possessed->isSleeping()) && !team.empty()) {
      possess(team.front(), view);
    } else {
      view->setTimeMilli(possessed->getTime() * 300);
      view->clearMessages();
      ViewObject::setHallu(false);
      possessed = nullptr;
      team.clear();
      gatheringTeam = false;
      teamLevelChanges.clear();
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
  return possessed != nullptr && possessed->isPlayer();
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

static string getTechName(TechId id) {
  switch (id) {
    case TechId::BEAST_TAMING: return "beast taming";
    case TechId::MATTER_ANIMATION: return "golem animation";
    case TechId::NECROMANCY: return "necromancy";
    case TechId::HUMANOID_BREEDING: return "humanoid breeding";
    case TechId::SPELLCASTING: return "spellcasting";
  }
  FAIL << "pwofk";
  return "";
}

static ViewObject getTechViewObject(TechId id) {
  switch (id) {
    case TechId::BEAST_TAMING: return ViewObject(ViewId::BEAR, ViewLayer::CREATURE, "");
    case TechId::MATTER_ANIMATION: return ViewObject(ViewId::IRON_GOLEM, ViewLayer::CREATURE, "");
    case TechId::NECROMANCY: return ViewObject(ViewId::VAMPIRE_LORD, ViewLayer::CREATURE, "");
    case TechId::HUMANOID_BREEDING: return ViewObject(ViewId::BILE_DEMON, ViewLayer::CREATURE, "");
    case TechId::SPELLCASTING: return ViewObject(ViewId::SCROLL, ViewLayer::CREATURE, "");
  }
  FAIL << "pwofk";
  return ViewObject();
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

static string getTechLevelName(int level) {
  CHECK(level >= 0 && level < 4);
  return vector<string>({"basic", "advanced", "expert", "master"})[level];
}

struct SpellLearningInfo {
  SpellId id;
  int techLevel;
};

vector<SpellLearningInfo> spellLearning {
    { SpellId::HEALING, 0 },
    { SpellId::SUMMON_INSECTS, 0},
    { SpellId::DECEPTION, 1},
    { SpellId::SPEED_SELF, 1},
    { SpellId::STR_BONUS, 1},
    { SpellId::DEX_BONUS, 2},
    { SpellId::FIRE_SPHERE_PET, 2},
    { SpellId::TELEPORT, 2},
    { SpellId::INVISIBILITY, 3},
    { SpellId::WORD_OF_POWER, 3},
};


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
    if (!contains(knownSpells, spell.id))
      mod = View::INACTIVE;
    options.emplace_back(spell.name + "  level: " + getTechLevelName(elem.techLevel), mod);
  }
  view->presentList(capitalFirst(getTechName(TechId::SPELLCASTING)), options);
}

vector<Collective::SpawnInfo> raisingInfo {
  {CreatureId::SKELETON, 30, 0},
  {CreatureId::ZOMBIE, 50, 0},
  {CreatureId::MUMMY, 50, 1},
  {CreatureId::VAMPIRE, 50, 2},
  {CreatureId::VAMPIRE_LORD, 100, 3},
};

void Collective::handleNecromancy(View* view, int prevItem, bool firstTime) {
  int techLevel = techLevels[TechId::NECROMANCY];
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
    creatures.push_back({CreatureFactory::fromId(info.id, Tribe::player, MonsterAIFactory::collective(this)),
        info.manaCost});
    options.emplace_back(creatures.back().first->getName() + "  mana: " + convertToString(info.manaCost) +
          "  level: " + getTechLevelName(info.minLevel),
          allInactive || info.minLevel > techLevel || info.manaCost > mana ? View::INACTIVE : View::NORMAL);
  }
  auto index = view->chooseFromList("Necromancy level: " + getTechLevelName(techLevel) + ", " +
      convertToString(corpses.size()) + " bodies available", options, prevItem);
  if (!index)
    return;
  // TODO: try many corpses before failing
  auto elem = chooseRandom(corpses);
  PCreature& creature = creatures[*index].first;
  mana -= creatures[*index].second;
  for (Vec2 v : elem.first.neighbors8(true))
    if (level->getSquare(v)->canEnter(creature.get())) {
      level->getSquare(elem.first)->removeItems({elem.second});
      addCreature(creature.get(), MinionType::UNDEAD);
      level->addCreature(v, std::move(creature));
      break;
    }
  if (creature)
    messageBuffer.addMessage(MessageBuffer::important("You have failed to reanimate the corpse."));
  view->updateView(this);
  handleNecromancy(view, *index, false);
}

vector<Collective::SpawnInfo> animationInfo {
  {CreatureId::CLAY_GOLEM, 30, 0},
  {CreatureId::STONE_GOLEM, 50, 1},
  {CreatureId::IRON_GOLEM, 50, 2},
  {CreatureId::LAVA_GOLEM, 100, 3},
};


void Collective::handleMatterAnimation(View* view) {
  handleSpawning(view, TechId::MATTER_ANIMATION, SquareType::LABORATORY,
      "You need to build a laboratory to animate golems.", "You need a larger laboratory.", "Golem animation",
      MinionType::GOLEM, animationInfo);
}

vector<Collective::SpawnInfo> tamingInfo {
  {CreatureId::RAVEN, 5, 0},
  {CreatureId::WOLF, 30, 1},
  {CreatureId::CAVE_BEAR, 50, 2},
  {CreatureId::SPECIAL_MONSTER_KEEPER, 100, 3},
};

void Collective::handleBeastTaming(View* view) {
  handleSpawning(view, TechId::BEAST_TAMING, SquareType::ANIMAL_TRAP,
      "You need to build cages to trap beasts.", "You need more cages.", "Beast taming",
      MinionType::BEAST, tamingInfo);
}

vector<Collective::SpawnInfo> breedingInfo {
  {CreatureId::GNOME, 30, 0},
  {CreatureId::GOBLIN, 50, 1},
  {CreatureId::BILE_DEMON, 80, 2},
  {CreatureId::SPECIAL_HUMANOID, 100, 3},
};

void Collective::handleHumanoidBreeding(View* view) {
  handleSpawning(view, TechId::HUMANOID_BREEDING, SquareType::BED,
      "You need to build beds to breed humanoids.", "You need more beds.", "Humanoid breeding",
      MinionType::NORMAL, breedingInfo);
}

void Collective::handleSpawning(View* view, TechId techId, SquareType spawnSquare,
    const string& info1, const string& info2, const string& title, MinionType minionType,
    vector<SpawnInfo> spawnInfo) {
  int techLevel = techLevels[techId];
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
      creatures.push_back({CreatureFactory::fromId(info.id, Tribe::player, MonsterAIFactory::collective(this)),
          info.manaCost});
      options.emplace_back(creatures.back().first->getName() + "  mana: " + convertToString(info.manaCost) + 
            "   level: " + getTechLevelName(info.minLevel),
            allInactive || info.minLevel > techLevel || info.manaCost > mana ? View::INACTIVE : View::NORMAL);
    }
    auto index = view->chooseFromList(title + " level: " + getTechLevelName(techLevel), options, prevItem);
    if (!index)
      return;
    Vec2 pos = chooseRandom(cages);
    PCreature& creature = creatures[*index].first;
    mana -= creatures[*index].second;
    for (Vec2 v : pos.neighbors8(true))
      if (level->getSquare(v)->canEnter(creature.get())) {
        addCreature(creature.get(), minionType);
        level->addCreature(v, std::move(creature));
        break;
      }
    if (creature)
      messageBuffer.addMessage(MessageBuffer::important("The spell failed."));
    view->updateView(this);
    prevItem = *index;
  }
}

int infMana = 1000000;
vector<int> techAdvancePoints { 100, 200, 300, infMana};

void Collective::handleLibrary(View* view) {
  if (mySquares.at(SquareType::LIBRARY).empty()) {
    view->presentText("", "You need to build a library to start research.");
    return;
  }
  if (mySquares.at(SquareType::LIBRARY).size() <= numTotalTech()) {
    view->presentText("", "You need a larger library to continue research.");
    return;
  }
  vector<View::ListElem> options;
  options.push_back(View::ListElem("You have " + convertToString(int(mana)) + " mana.", View::TITLE));
  for (TechId id : techIds) {
    string text = getTechName(id) + ": " + getTechLevelName(techLevels.at(id));
    int neededPoints = techAdvancePoints[techLevels.at(id)];
    if (neededPoints < infMana)
      text += "  (" + convertToString(neededPoints) + " mana to advance)";
    options.emplace_back(text, neededPoints <= mana ? View::NORMAL : View::INACTIVE);
  }
  auto index = view->chooseFromList("Library", options);
  if (!index)
    return;
  TechId id = techIds[*index];
  mana -= techAdvancePoints[techLevels.at(id)];
  ++techLevels[id];
  if (id == TechId::SPELLCASTING)
    for (auto elem : spellLearning)
      if (elem.techLevel == techLevels[id])
        keeper->addSpell(elem.id);
  view->updateView(this);
  handleLibrary(view);
}

int Collective::numTotalTech() const {
  int ret = 0;
  for (auto id : techIds)
    ret += techLevels.at(id);
  return ret;
}

void Collective::refreshGameInfo(View::GameInfo& gameInfo) const {
  gameInfo.infoType = View::GameInfo::InfoType::BAND;
  View::GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.name = "KeeperRL";
  info.buttons.clear();
  for (BuildInfo button : buildInfo) {
    switch (button.buildType) {
      case BuildInfo::SQUARE: {
            BuildInfo::SquareInfo& elem = button.squareInfo;
            Optional<pair<ViewObject, int>> cost;
            if (elem.cost > 0)
              cost = {getResourceViewObject(elem.resourceId), elem.cost};
            info.buttons.push_back({
                SquareFactory::get(elem.type)->getViewObject(),
                elem.name,
                cost,
                (elem.cost > 0 ? "[" + convertToString(mySquares.at(elem.type).size()) + "]" : ""),
                elem.cost <= numGold(elem.resourceId) });
           }
           break;
      case BuildInfo::DIG: {
             info.buttons.push_back({
                 ViewObject(ViewId::DIG_ICON, ViewLayer::LARGE_ITEM, ""),
                 "dig or cut tree", Nothing(), "", true});
           }
           break;
      case BuildInfo::TRAP: {
             BuildInfo::TrapInfo& elem = button.trapInfo;
             int numTraps = getTrapItems(elem.type).size();
             info.buttons.push_back({
                 ViewObject(elem.viewId, ViewLayer::LARGE_ITEM, ""),
                 elem.name,
                 Nothing(),
                 "(" + convertToString(numTraps) + " ready)",
                 numTraps > 0});
           }
           break;
      case BuildInfo::DOOR: {
             BuildInfo::DoorInfo& elem = button.doorInfo;
             pair<ViewObject, int> cost = {getResourceViewObject(elem.resourceId), elem.cost};
             info.buttons.push_back({
                 ViewObject(elem.viewId, ViewLayer::LARGE_ITEM, ""),
                 elem.name,
                 cost,
                 "[" + convertToString(doors.size()) + "]",
                 elem.cost <= numGold(elem.resourceId)});
           }
           break;
      case BuildInfo::IMP: {
           pair<ViewObject, int> cost = {ViewObject::mana(), getImpCost()};
           info.buttons.push_back({
               ViewObject(ViewId::IMP, ViewLayer::CREATURE, ""),
               "Imp",
               cost,
               "[" + convertToString(imps.size()) + "]",
               getImpCost() <= mana});
           break; }
      case BuildInfo::DESTROY:
           info.buttons.push_back({
               ViewObject(ViewId::DESTROY_BUTTON, ViewLayer::CREATURE, ""), "Remove construction", Nothing(), "",
                   true});
           break;
      case BuildInfo::GUARD_POST:
           info.buttons.push_back({
               ViewObject(ViewId::GUARD_POST, ViewLayer::CREATURE, ""), "Guard post", Nothing(), "", true});
           break;
    }
    info.buttons.back().help = button.help;
  }
  info.activeButton = currentButton;
  info.tasks = minionTaskStrings;
  for (Creature* c : minions)
    if (isInCombat(c))
      info.tasks[c] = "fighting";
  info.monsterHeader = "Monsters: " + convertToString(minions.size()) + " / " + convertToString(minionLimit);
  info.creatures.clear();
  for (Creature* c : minions)
    info.creatures.push_back(c);
  info.enemies.clear();
  for (Vec2 v : myTiles)
    if (Creature* c = level->getSquare(v)->getCreature())
      if (c->getTribe() != Tribe::player)
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
  info.time = keeper->getTime();
  info.gatheringTeam = gatheringTeam;
  info.team.clear();
  for (Creature* c : team)
    info.team.push_back(c);
  info.techButtons.clear();
  for (TechId id : techIds) {
    info.techButtons.push_back({getTechViewObject(id), getTechName(id)});
  }
  info.techButtons.push_back({Nothing(), ""});
  info.techButtons.push_back({ViewObject(ViewId::LIBRARY, ViewLayer::CREATURE, ""), "library"});
  info.techButtons.push_back({ViewObject(ViewId::GOLD, ViewLayer::CREATURE, ""), "black market"});
}

bool Collective::isItemMarked(const Item* it) const {
  return markedItems.count(it->getUniqueId());
}

void Collective::markItem(const Item* it) {
  markedItems.insert(it->getUniqueId());
}

void Collective::unmarkItem(const Item* it) {
  markedItems.erase(it->getUniqueId());
}

const MapMemory& Collective::getMemory(const Level* l) const {
  return (*memory)[l];
}

MapMemory& Collective::getMemory(const Level* l) {
  return (*memory)[l];
}

static ViewObject getTrapObject(TrapType type) {
  switch (type) {
    case TrapType::BOULDER: return ViewObject(ViewId::UNARMED_BOULDER_TRAP, ViewLayer::LARGE_ITEM, "Unarmed trap");
    case TrapType::POISON_GAS: return ViewObject(ViewId::UNARMED_GAS_TRAP, ViewLayer::LARGE_ITEM, "Unarmed trap");
  }
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

void Collective::TaskMap::delayTask(Task* task, double time) {
  CHECK(task->canTransfer());
  delayed[task] = time;
 // Debug() << "Delaying " << (taken.count(task) ? "taken " : "") << task->getInfo();
  if (taken.count(task)) {
    taskMap.erase(taken.at(task));
    taken.erase(task);
  }
}

bool Collective::TaskMap::isDelayed(Task* task, double time) {
  if (delayed.count(task)) {
    if (delayed.at(task) > time)
      return true;
    else
      delayed.erase(task);
  }
  return false;

}

void Collective::TaskMap::removeTask(Task* task) {
  task->cancel();
  delayed.erase(task);
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
  tasks.push_back(std::move(task));
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
  c->pushController(new Player(c, view, model, false, memory));
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

void Collective::processInput(View* view) {
  if (!possessed && isInCombat(keeper) && lastControlKeeperQuestion < keeper->getTime() - 50) {
    lastControlKeeperQuestion = keeper->getTime();
    if (view->yesOrNoPrompt("The keeper is engaged in combat. Do you want to control him?")) {
      possess(keeper, view);
      return;
    }
  }
  CollectiveAction action = view->getClick();
  switch (action.getType()) {
    case CollectiveAction::GATHER_TEAM:
        if (gatheringTeam && !team.empty()) {
          possess(team[0], view);
          gatheringTeam = false;
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
    case CollectiveAction::TECHNOLOGY:
        if (action.getNum() < techIds.size())
          switch (techIds[action.getNum()]) {
            case TechId::NECROMANCY: handleNecromancy(view); break;
            case TechId::SPELLCASTING: handlePersonalSpells(view); break;
            case TechId::MATTER_ANIMATION: handleMatterAnimation(view); break;
            case TechId::BEAST_TAMING: handleBeastTaming(view); break;
            case TechId::HUMANOID_BREEDING: handleHumanoidBreeding(view); break;
            default: break;
          };
        if (action.getNum() == techIds.size() + 1)
          handleLibrary(view);
        if (action.getNum() == techIds.size() + 2)
          handleMarket(view);
        break;
    case CollectiveAction::ROOM_BUTTON: currentButton = action.getNum(); break;
    case CollectiveAction::CREATURE_BUTTON: 
        if (!gatheringTeam)
          possess(action.getCreature(), view);
        else {
          if (contains(team, action.getCreature()))
            removeElement(team, action.getCreature());
          else
            team.push_back(const_cast<Creature*>(action.getCreature()));
        }
        break;
    case CollectiveAction::CREATURE_DESCRIPTION: messageBuffer.addMessage(MessageBuffer::important(
                                                       action.getCreature()->getDescription())); break;
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
    case CollectiveAction::GO_TO:
        handleSelection(action.getPosition(), false);
        break;
    case CollectiveAction::BUTTON_RELEASE:
        selection = NONE;
        if (rectSelectCorner && rectSelectCorner2) {
          handleSelection(*rectSelectCorner, true);
          for (Vec2 v : Rectangle::boundingBox({*rectSelectCorner, *rectSelectCorner2}))
            handleSelection(v, true);
        }
        rectSelectCorner = Nothing();
        rectSelectCorner2 = Nothing();
        selection = NONE;
        break;
    case CollectiveAction::IDLE: break;
  }
}

void Collective::handleSelection(Vec2 pos, bool rectangle) {
  if (!pos.inRectangle(level->getBounds()))
    return;
  switch (buildInfo[currentButton].buildType) {
    case BuildInfo::IMP:
        if (mana >= getImpCost() && selection == NONE && !rectangle) {
          selection = SELECT;
          PCreature imp = CreatureFactory::fromId(CreatureId::IMP, Tribe::player,
              MonsterAIFactory::collective(this));
          for (Vec2 v : pos.neighbors8(true))
            if (v.inRectangle(level->getBounds()) && level->getSquare(v)->canEnter(imp.get()) 
                && canSee(v)) {
              mana -= getImpCost();
              addCreature(imp.get(), MinionType::IMP);
              level->addCreature(v, std::move(imp));
              break;
            }
        }
        break;
    case BuildInfo::TRAP: {
        TrapType trapType = buildInfo[currentButton].trapInfo.type;
        if (getTrapItems(trapType).size() > 0 && canPlacePost(pos) && myTiles.count(pos)) {
          traps[pos] = {trapType, false, false};
          trapMap[trapType].push_back(pos);
          updateTraps();
        }
      }
      break;
    case BuildInfo::DOOR: {
        BuildInfo::DoorInfo info = buildInfo[currentButton].doorInfo;
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
          BuildInfo::SquareInfo info = buildInfo[currentButton].squareInfo;
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
    doors.at(pos).marked = false;
  }
}

void Collective::onPickedUp(Vec2 pos, vector<Item*> items) {
  CHECK(!items.empty());
  for (Item* it : items)
    unmarkItem(it);
}
  
void Collective::onCantPickItem(vector<Item*> items) {
  for (Item* it : items)
    unmarkItem(it);
}

void Collective::onBrought(Vec2 pos, vector<Item*> items) {
}

void Collective::onAppliedItem(Vec2 pos, Item* item) {
  CHECK(item->getTrapType());
  if (traps.count(pos)) {
    traps[pos].marked = false;
    traps[pos].armed = true;
  }
}

void Collective::onAppliedSquare(Vec2 pos) {
  if (mySquares.at(SquareType::LIBRARY).count(pos)) {
    mana += 1 + max(0., 1 - double(getDangerLevel()) / 1000);
  }
  if (mySquares.at(SquareType::LABORATORY).count(pos))
    if (Random.roll(30)) {
      level->getSquare(pos)->dropItems(ItemFactory::potions().random());
      Statistics::add(StatId::POTION_PRODUCED);
    }
  if (mySquares.at(SquareType::WORKSHOP).count(pos))
    if (Random.roll(40)) {
      vector<PItem> items  = ItemFactory::workshop().random();
      if (items[0]->getType() == ItemType::WEAPON)
        Statistics::add(StatId::WEAPON_PRODUCED);
      if (items[0]->getType() == ItemType::ARMOR)
        Statistics::add(StatId::ARMOR_PRODUCED);
      level->getSquare(pos)->dropItems(std::move(items));
    }
}

void Collective::onAppliedItemCancel(Vec2 pos) {
  traps.at(pos).marked = false;
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

void Collective::updateTraps() {
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems {
    {TrapType::BOULDER, getTrapItems(TrapType::BOULDER, myTiles)},
    {TrapType::POISON_GAS, getTrapItems(TrapType::POISON_GAS, myTiles)}};
  for (auto elem : traps) {
    vector<pair<Item*, Vec2>>& items = trapItems.at(elem.second.type);
    if (!items.empty()) {
      if (!elem.second.armed && !elem.second.marked) {
        taskMap.addTask(Task::applyItem(this, items.back().second, items.back().first, elem.first));
        markItem(items.back().first);
        items.pop_back();
        traps[elem.first].marked = true;
      }
    }
  }
  for (auto& elem : doors) {
    if (!elem.second.marked && !elem.second.built && numGold(elem.second.cost.id) >= elem.second.cost.value) {
      taskMap.addTask(Task::construction(this, elem.first, SquareType::TRIBE_DOOR));
      elem.second.marked = true;
      takeGold(elem.second.cost);
    }
  }
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
  map<Vec2, Task*> taskPos;
  for (Task* task : taskMap.getTasks())
    if (task->canTransfer()) {
      taskPos[task->getPosition()] = task;
    } else
 //     Debug() << "Delay: can't tranfer " << task->getInfo();
  while (!q.empty()) {
    Vec2 pos = q.front();
    q.pop();
    if (taskPos.count(pos))
      taskMap.delayTask(taskPos.at(pos), delayTime);
    if (dist[pos] >= radius || !level->getSquare(pos)->canEnterEmpty(Creature::getDefault()))
      continue;
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(dist.getBounds()) && dist[v] == infinity) {
        dist[v] = dist[pos] + 1;
        q.push(v);
      }
  }
}

void Collective::tick() {
  warning[int(Warning::MANA)] = mana < 100;
  warning[int(Warning::WOOD)] = numGold(ResourceId::WOOD) == 0;
  warning[int(Warning::DIGGING)] = mySquares.at(SquareType::FLOOR).empty();
  warning[int(Warning::MINIONS)] = minions.size() <= 1;
  for (auto elem : taskInfo)
    if (!mySquares.at(elem.second.square).empty())
      warning[int(elem.second.warning)] = false;
  updateTraps();
  vector<Vec2> enemyPos;
  for (Vec2 pos : myTiles) {
    if (Creature* c = level->getSquare(pos)->getCreature()) {
 /*     if (!contains(creatures, c) && c->getTribe() == Tribe::player
          && !contains({"boulder"}, c->getName()))
        // We just found a friendly creature (and not a boulder nor a chicken)
        addCreature(c);*/
      if (c->getTribe() != Tribe::player)
        enemyPos.push_back(c->getPosition());
    }
    vector<Item*> gold = level->getSquare(pos)->getItems(unMarkedItems(ItemType::GOLD));
    if (gold.size() > 0 && !mySquares[SquareType::TREASURE_CHEST].count(pos)) {
      if (!mySquares[SquareType::TREASURE_CHEST].empty()) {
        warning[int(Warning::CHESTS)] = false;
        Optional<Vec2> target;
        for (Vec2 chest : mySquares[SquareType::TREASURE_CHEST])
          if ((!target || (chest - pos).length8() < (*target - pos).length8()) && 
              level->getSquare(chest)->getItems(Item::typePredicate(ItemType::GOLD)).size() <= 30)
            target = chest;
        if (!target)
          warning[int(Warning::MORE_CHESTS)] = true;
        else {
          warning[int(Warning::MORE_CHESTS)] = false;
          taskMap.addTask(Task::bringItem(this, pos, gold, *target));
          for (Item* it : gold)
            markItem(it);
        }
      } else {
        warning[int(Warning::CHESTS)] = true;
      }
    }
 /*   if (taskMap.isMarked(pos) && marked.at(pos)->isImpossible(level) && !taken.count(marked.at(pos)))
      removeTask(marked.at(pos));*/
  }
  for (ItemFetchInfo elem : getFetchInfo()) {
    for (Vec2 pos : myTiles)
      fetchItems(pos, elem);
    for (SquareType type : elem.additionalPos)
      for (Vec2 pos : mySquares.at(type))
        fetchItems(pos, elem);
  }
  if (!enemyPos.empty())
    delayDangerousTasks(enemyPos, keeper->getTime() + 50);
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

void Collective::onChangeLevelEvent(const Creature* c, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {
  if (c == possessed) { 
    teamLevelChanges[from] = pos;
    if (!levelChangeHistory.count(to))
      levelChangeHistory[to] = toPos;
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

MoveInfo Collective::getMinionMove(Creature* c) {
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
  if (c->getLevel() != level) {
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
  if (contains(minionByType.at(MinionType::BEAST), c))
    return getBeastMove(c);
  for (auto& elem : guardPosts) {
    bool isTraining = contains({MinionTask::TRAIN, MinionTask::LABORATORY, MinionTask::WORKSHOP},
        minionTasks.at(c).getState());
    if (elem.second.attender == c) {
      if (isTraining) {
        minionTasks.at(c).update();
        minionTaskStrings[c] = "guarding";
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
    bool isTraining = contains({MinionTask::TRAIN}, minionTasks.at(c).getState());
    if (elem.second.attender == nullptr && isTraining) {
      elem.second.attender = c;
      if (Task* t = taskMap.getTask(c))
        taskMap.removeTask(t);
    }
  }
 
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  if (c != keeper || !underAttack())
    for (Vec2 v : mySquares[SquareType::STOCKPILE])
      for (Item* it : level->getSquare(v)->getItems([this, c] (const Item* it) {
            return minionEquipment.needsItem(c, it); })) {
        if (c->canEquip(it)) {
          taskMap.addTask(Task::equipItem(this, v, it), c);
        }
        else
          taskMap.addTask(Task::pickItem(this, v, {it}), c);
        return taskMap.getTask(c)->getMove(c);
      }
  minionTasks.at(c).update();
  if (c->getHealth() < 1 && c->canSleep())
    minionTasks.at(c).setState(MinionTask::SLEEP);
  if (c == keeper && !myTiles.empty() && !myTiles.count(c->getPosition())) {
    if (auto move = c->getMoveTowards(chooseRandom(myTiles)))
      return {1.0, [=] {
        c->move(*move);
      }};
  }
  MinionTaskInfo info = taskInfo.at(minionTasks.at(c).getState());
  if (mySquares[info.square].empty()) {
    minionTasks.at(c).updateToNext();
    warning[int(info.warning)] = true;
    return NoMove;
  }
  warning[int(info.warning)] = false;
  taskMap.addTask(Task::applySquare(this, mySquares[info.square]), c);
  minionTaskStrings[c] = info.desc;
  return taskMap.getTask(c)->getMove(c);
}

bool Collective::underAttack() const {
  for (Vec2 v : myTiles)
    if (const Creature* c = level->getSquare(v)->getCreature())
      if (c->getTribe() != Tribe::player)
        return true;
  return false;
}

Task* Collective::TaskMap::getTaskForImp(Creature* c) {
  Task* closest = nullptr;
  for (PTask& task : tasks) {
    if (isDelayed(task.get(), c->getTime()))
      continue;
    double dist = (task->getPosition() - c->getPosition()).length8();
    if ((!taken.count(task.get()) || (task->canTransfer() 
                                && (task->getPosition() - taken.at(task.get())->getPosition()).length8() > dist))
           && (!closest ||
           dist < (closest->getPosition() - c->getPosition()).length8()) && !lockedTasks.count(make_pair(c, task->getUniqueId()))) {
      bool valid = task->getMove(c).isValid();
      if (valid)
        closest = task.get();
      else
        lockedTasks.insert(make_pair(c, task->getUniqueId()));
    }
  }
  return closest;
}

void Collective::TaskMap::takeTask(const Creature* c, Task* task) {
  free(task);
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

void Collective::addCreature(Creature* c, MinionType type) {
  if (keeper == nullptr) {
    keeper = c;
    type = MinionType::KEEPER;
    for (auto elem : spellLearning)
      if (elem.techLevel == 0)
        keeper->addSpell(elem.id);
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
    minionTasks.insert(make_pair(c, getTasksForMinion(c)));
  } else {
    imps.push_back(c);
 //   c->addEnemyVision([this](const Creature* c) { return canSee(c); });
  }
}

void Collective::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == level) {
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
      }
    if (doors.count(pos)) {
      DoorInfo& info = doors.at(pos);
      info.marked = info.built = false;
    }
  }
}

void Collective::onTriggerEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == level)
    traps.at(pos).armed = false;
}

void Collective::onConqueredLand(const string& name) {
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
  if (victim == keeper) {
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
      if (!task->canTransfer())
        taskMap.removeTask(task);
      else
        taskMap.freeTask(task);
    }
    if (contains(imps, c))
      removeElement(imps, c);
    if (contains(minions, c))
      removeElement(minions, c);
    for (MinionType type : minionTypes)
      if (contains(minionByType.at(type), c))
        removeElement(minionByType.at(type), c);
  } else if (victim->getTribe() != Tribe::player) {
    double incMana = victim->getDifficultyPoints();
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
