#include "stdafx.h"

enum Warning { NO_CONNECTION, NO_CHESTS, MORE_CHESTS, NO_BEDS, MORE_BEDS, NO_HATCHERY, NO_TRAINING };
static const int numWarnings = 7;
static bool warning[numWarnings] = {0};
static string warningText[] {
  "You need to connect to the stairs to attract minions.",
  "You need to build a treasure room.",
  "You need a larger treasure room.",
  "You need a lair for your minions.",
  "You need a larger lair for your minions.",
  "Your minions are hungry!",
  "You need training posts for your minions"};

vector<Collective::BuildInfo> Collective::buildInfo {
    BuildInfo({SquareType::FLOOR, 0, "Dig"}),
    BuildInfo({SquareType::TREASURE_CHEST, 0, "Treasure room"}),
    BuildInfo({SquareType::TRIBE_DOOR, 20, "Door"}),
    BuildInfo({SquareType::BRIDGE, 5, "Bridge"}),
    BuildInfo({SquareType::BED, 50, "Lair"}),
    BuildInfo({SquareType::HATCHERY, 10, "Hatchery"}),
    BuildInfo({SquareType::TRAINING_DUMMY, 50, "Training room"}),
    BuildInfo({SquareType::WORKSHOP, 50, "Workshop"}),
    BuildInfo({SquareType::GRAVE, 50, "Graveyard"}),
    BuildInfo({TrapType::BOULDER, "Boulder trap", ViewId::BOULDER}),
    BuildInfo({TrapType::POISON_GAS, "Gas trap", ViewId::GAS_TRAP}),
    BuildInfo(BuildInfo::IMP),
    BuildInfo(BuildInfo::GUARD_POST),
};

#ifndef DEBUG
const int creditVal = 50;
#else
const int creditVal = 500;
#endif

Collective::Collective(CreatureFactory factory, CreatureFactory undead) 
    : minionFactory(factory), undeadFactory(undead), credit(creditVal) {
  EventListener::addListener(this);
  // init the map so the values can be safely read with .at()
  for (BuildInfo info : buildInfo)
    if (info.buildType == BuildInfo::SQUARE)
      mySquares[info.squareInfo.type].clear();
    else if (info.buildType == BuildInfo::TRAP)
      trapMap[info.trapInfo.type].clear();
}


const int basicImpCost = 20;
int startImpNum = -1;
const int minionLimit = 16;


void Collective::render(View* view) {
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
      view->resetCenter();
    }
  }
  if (!possessed) {
    view->refreshView(this);
  } else
    view->stopClock();
}

bool Collective::isTurnBased() {
  return possessed != nullptr;
}

vector<pair<Item*, Vec2>> Collective::getTrapItems(TrapType type, set<Vec2> squares) const {
  vector<pair<Item*, Vec2>> ret;
  if (squares.empty())
    squares = mySquares.at(SquareType::WORKSHOP);
  for (Vec2 pos : squares) {
    vector<Item*> v = level->getSquare(pos)->getItems([type, this](Item* it) {
        return it->getTrapType() == type && !markedItems.count(it); });
    for (Item* it : v)
      ret.emplace_back(it, pos);
  }
  return ret;
}


void Collective::refreshGameInfo(View::GameInfo& gameInfo) const {
  gameInfo.infoType = View::GameInfo::InfoType::BAND;
  View::GameInfo::BandInfo& info = gameInfo.bandInfo;
  info.number = creatures.size();
  info.name = "KeeperRL";
  info.buttons.clear();
  int gold = numGold();
  for (BuildInfo button : buildInfo)
    switch (button.buildType) {
      case BuildInfo::SQUARE: {
            BuildInfo::SquareInfo& elem = button.squareInfo;
            info.buttons.emplace_back(
                elem.name + (elem.cost > 0 ? ("   $ " + convertToString(elem.cost)) : "") 
                + (elem.cost > 0 ? "  [" + convertToString<int>(mySquares.at(elem.type).size()) + "]" : ""),
                SquareFactory::get(elem.type)->getViewObject(),
                elem.cost <= gold);
           }
           break;
      case BuildInfo::TRAP: {
             BuildInfo::TrapInfo& elem = button.trapInfo;
             int numTraps = getTrapItems(elem.type).size();
             info.buttons.emplace_back(elem.name + "   (" + convertToString(numTraps) + " ready)",
                 ViewObject(elem.viewId, ViewLayer::LARGE_ITEM, ""), numTraps > 0);
           }
           break;
      case BuildInfo::IMP:
           info.buttons.emplace_back("Imp   $ " + convertToString(getImpCost()),
               ViewObject(ViewId::IMP, ViewLayer::CREATURE, ""), getImpCost() <= gold);
           break;
      case BuildInfo::GUARD_POST:
           info.buttons.emplace_back("Guard post",
               ViewObject(ViewId::GUARD_POST, ViewLayer::CREATURE, ""), true);

    }
  info.activeButton = currentButton;
  info.tasks = minionTaskStrings;
  info.monsterHeader = "Monsters: " + convertToString(minions.size()) + " / " + convertToString(minionLimit);
  info.creatures.clear();
  for (Creature* c : minions)
    info.creatures.push_back(c);
  info.enemies.clear();
  for (Vec2 v : myTiles)
    if (Creature* c = level->getSquare(v)->getCreature())
      if (c->getTribe() != Tribe::player)
        info.enemies.push_back(c);
  info.numGold = numGold();
  info.warning = "";
  for (int i : Range(numWarnings))
    if (warning[i]) {
      info.warning = warningText[i];
      break;
    }
  info.time = heart->getTime();
  info.gatheringTeam = gatheringTeam;
  info.team.clear();
  for (Creature* c : team)
    info.team.push_back(c);
}

const MapMemory& Collective::getMemory(const Level* l) const {
  return memory[l];
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
  if (marked.count(pos))
    index.setHighlight(HighlightType::BUILD);
  if (!index.hasObject(ViewLayer::LARGE_ITEM)) {
    if (traps.count(pos))
      index.insert(getTrapObject(traps.at(pos).type));
    if (guardPosts.count(pos))
      index.insert(ViewObject(ViewId::GUARD_POST, ViewLayer::LARGE_ITEM, "Guard post"));
  }
  if (const Location* loc = level->getLocation(pos)) {
    if (loc->isMarkedAsSurprise() && loc->getBounds().middle() == pos && !memory[level].hasViewIndex(pos))
      index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE, "Surprise"));
  }
  return index;
}

bool Collective::staticPosition() const {
  return false;
}

Vec2 Collective::getPosition() const {
  return heart->getPosition();
}

enum Selection { SELECT, DESELECT, NONE } selection = NONE;



void Collective::addTask(PTask task, Creature* c) {
  taken[task.get()] = c;
  taskMap[c] = task.get();
  addTask(std::move(task));
}

void Collective::addTask(PTask task) {
  tasks.push_back(std::move(task));
}

void Collective::removeTask(Task* task) {
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

void Collective::markSquare(Vec2 pos, BuildInfo::SquareInfo info) {
  tasks.push_back(Task::construction(this, pos, info.type));
  marked[pos] = tasks.back().get();
  if (info.cost)
    completionCost[tasks.back().get()] = info.cost;
}

void Collective::unmarkSquare(Vec2 pos) {
  Task* t = marked.at(pos);
  if (completionCost.count(t)) {
    returnGold(completionCost.at(t));
    completionCost.erase(t);
  }
  removeTask(t);
  marked.erase(pos);
}

int Collective::numGold() const {
  int ret = credit;
  for (Vec2 pos : mySquares.at(SquareType::TREASURE_CHEST))
    ret += level->getSquare(pos)->getItems(Item::typePredicate(ItemType::GOLD)).size();
  return ret;
}

void Collective::takeGold(int number) {
  int num = number;
  if (num == 0)
    return;
  CHECK(num > 0);
  if (credit) {
    if (credit >= num) {
      credit -= num;
      return;
    } else {
      num -= credit;
      credit = 0;
    }
  }
  for (Vec2 pos : randomPermutation(mySquares[SquareType::TREASURE_CHEST])) {
    vector<Item*> goldHere = level->getSquare(pos)->getItems(Item::typePredicate(ItemType::GOLD));
    for (Item* it : goldHere) {
      level->getSquare(pos)->removeItem(it);
      if (--num == 0)
        return;
    }
  }
  Debug(FATAL) << "Didn't have enough gold";
}

void Collective::returnGold(int num) {
  if (num == 0)
    return;
  CHECK(num > 0);
  if (mySquares[SquareType::TREASURE_CHEST].empty()) {
    credit += num;
  } else
    level->getSquare(chooseRandom(mySquares[SquareType::TREASURE_CHEST]))->
        dropItems(ItemFactory::fromId(ItemId::GOLD_PIECE, num));
}

int Collective::getImpCost() const {
  if (imps.size() < startImpNum)
    return 0;
  return basicImpCost * pow(2, double(imps.size() - startImpNum) / 5);
}

void Collective::possess(const Creature* cr, View* view) {
  CHECK(contains(creatures, cr));
  CHECK(!cr->isDead());
  Creature* c = const_cast<Creature*>(cr);
  if (c->isSleeping())
    c->wakeUp();
  freeFromGuardPost(c);
  c->pushController(new Player(c, view, false, &memory));
  possessed = c;
  c->getLevel()->setPlayer(c);
}

bool Collective::canBuildDoor(Vec2 pos) const {
  Rectangle innerRect = level->getBounds().minusMargin(1);
  auto wallFun = [=](Vec2 pos) {
      return level->getSquare(pos)->canConstruct(SquareType::FLOOR) ||
          !pos.inRectangle(innerRect); };
  return pos.inRectangle(innerRect) && 
      ((wallFun(pos - Vec2(0, 1)) && wallFun(pos - Vec2(0, -1))) ||
       (wallFun(pos - Vec2(1, 0)) && wallFun(pos - Vec2(-1, 0))));
}

bool Collective::canPlacePost(Vec2 pos) const {
  return !guardPosts.count(pos) && !traps.count(pos) &&
      level->getSquare(pos)->canEnterEmpty(Creature::getDefault()) && memory[level].hasViewIndex(pos);
}
  
void Collective::freeFromGuardPost(const Creature* c) {
  for (auto& elem : guardPosts)
    if (elem.second.attender == c)
      elem.second.attender = nullptr;
}

void Collective::processInput(View* view) {
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
    case CollectiveAction::CANCEL_TEAM: gatheringTeam = false; team.clear(); break;
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
    case CollectiveAction::GO_TO: {
        Vec2 pos = action.getPosition();
        if (!pos.inRectangle(level->getBounds()))
          return;
        switch (buildInfo[currentButton].buildType) {
          case BuildInfo::IMP:
              if (numGold() >= getImpCost() && selection == NONE) {
                selection = SELECT;
                PCreature imp = CreatureFactory::fromId(CreatureId::IMP, Tribe::player,
                    MonsterAIFactory::collective(this));
                for (Vec2 v : pos.neighbors8(true))
                  if (v.inRectangle(level->getBounds()) && level->getSquare(v)->canEnter(imp.get()) 
                      && canSee(v)) {
                    takeGold(getImpCost());
                    addCreature(imp.get());
                    level->addCreature(v, std::move(imp));
                    break;
                  }
              }
              break;
          case BuildInfo::TRAP: {
                TrapType trapType = buildInfo[currentButton].trapInfo.type;
                if (getTrapItems(trapType).size() > 0 && canPlacePost(pos)){
                  traps[pos] = {trapType, false, false};
                  trapMap[trapType].push_back(pos);
                }
              }
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
          case BuildInfo::SQUARE:
              if (selection == NONE) {
                if (Creature* c = level->getSquare(pos)->getCreature())
                  if (contains(minions, c)) {
                    possess(c, view);
                    break;
                  }
              }
              if (marked.count(pos) && selection != SELECT) {
                unmarkSquare(pos);
                selection = DESELECT;
              } else {
                BuildInfo::SquareInfo info = buildInfo[currentButton].squareInfo;
                bool diggingSquare = !memory[level].hasViewIndex(pos) ||
                  (level->getSquare(pos)->canConstruct(info.type));
                if (diggingSquare || selection != NONE) {
                  if (!marked.count(pos) && selection != DESELECT && diggingSquare && 
                      numGold() >= info.cost && 
                      (info.type != SquareType::TRIBE_DOOR || canBuildDoor(pos)) &&
                      (info.type == SquareType::FLOOR || canSee(pos))) {
                    markSquare(pos, info);
                    selection = SELECT;
                    takeGold(info.cost);
                  }
                }
              }
              break;
        }
      }
      break;
    case CollectiveAction::BUTTON_RELEASE: selection = NONE; break;

    default: break;
  }
}

void Collective::onConstructed(Vec2 pos, SquareType type) {
  CHECK(!mySquares[type].count(pos));
  mySquares[type].insert(pos);
  if (contains({SquareType::FLOOR, SquareType::BRIDGE}, type))
    locked.clear();
  if (marked.count(pos))
    marked.erase(pos);
}

void Collective::onPickedUp(Vec2 pos, vector<Item*> items) {
  CHECK(!items.empty());
  for (Item* it : items)
    markedItems.erase(it);
}

void Collective::onBrought(Vec2 pos, vector<Item*> items) {
}

void Collective::onAppliedItem(Vec2 pos, Item* item) {
  CHECK(item->getTrapType());
  traps[pos].marked = false;
  traps[pos].armed = true;
}

void Collective::onAppliedItemCancel(Vec2 pos) {
  traps.at(pos).marked = false;
}

Vec2 Collective::getHeartPos() const {
  return heart->getPosition();
}

ItemPredicate Collective::unMarkedItems(ItemType type) const {
  return [this, type](const Item* it) {
      return it->getType() == type && !markedItems.count(it); };
}

void Collective::update(Creature* c) {
  if (!contains(creatures, c) || c->getLevel() != level)
    return;
  for (Vec2 pos : level->getVisibleTiles(c)) {
    myTiles.insert(pos);
    ViewIndex index = level->getSquare(pos)->getViewIndex(c);
    memory[level].clearSquare(pos);
    for (ViewLayer l : { ViewLayer::ITEM, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM})
      if (index.hasObject(l))
        memory[level].addObject(pos, index.getObject(l));
  }
}

bool Collective::isDownstairsVisible() const {
  Vec2 pos = getOnlyElement(level->getLandingSquares(StairDirection::DOWN, StairKey::DWARF));
  return memory[level].hasViewIndex(pos);
}

void Collective::tick() {
  if (isDownstairsVisible()
      && (minions.size() - vampires.size() < mySquares[SquareType::BED].size() || minions.empty())
      && Random.roll(40) && minions.size() < minionLimit) {
    PCreature c = minionFactory.random(MonsterAIFactory::collective(this));
    addCreature(c.get());
    level->landCreature(StairDirection::DOWN, StairKey::DWARF, std::move(c));
  }
  warning[NO_CONNECTION] = !isDownstairsVisible();
  warning[NO_BEDS] = mySquares[SquareType::BED].size() == 0 && !minions.empty();
  warning[MORE_BEDS] = mySquares[SquareType::BED].size() < minions.size() - vampires.size();
  warning[NO_TRAINING] = mySquares[SquareType::TRAINING_DUMMY].empty() && !minions.empty();
  warning[NO_HATCHERY] = mySquares[SquareType::HATCHERY].empty() && !minions.empty();
  map<TrapType, vector<pair<Item*, Vec2>>> trapItems {
    {TrapType::BOULDER, getTrapItems(TrapType::BOULDER, myTiles)},
    {TrapType::POISON_GAS, getTrapItems(TrapType::POISON_GAS, myTiles)}};
  for (auto elem : traps) {
    vector<pair<Item*, Vec2>>& items = trapItems.at(elem.second.type);
    if (!items.empty()) {
      if (!elem.second.armed && !elem.second.marked) {
        addTask(Task::applyItem(this, items.back().second, items.back().first, elem.first));
        markedItems.insert({items.back().first});
        items.pop_back();
        traps[elem.first].marked = true;
      }
    }
  }
  for (Vec2 pos : myTiles) {
    if (Creature* c = level->getSquare(pos)->getCreature())
      if (!contains(creatures, c) && c->getTribe() == Tribe::player
          && !contains({"chicken", "boulder"}, c->getName()))
        // We just found a friendly creature (and not a boulder nor a chicken)
        addCreature(c);
    vector<Item*> gold = level->getSquare(pos)->getItems(unMarkedItems(ItemType::GOLD));
    if (gold.size() > 0 && !mySquares[SquareType::TREASURE_CHEST].count(pos)) {
      if (!mySquares[SquareType::TREASURE_CHEST].empty()) {
        warning[NO_CHESTS] = false;
        Optional<Vec2> target;
        for (Vec2 chest : mySquares[SquareType::TREASURE_CHEST])
          if ((!target || (chest - pos).length8() < (*target - pos).length8()) && 
              level->getSquare(chest)->getItems(Item::typePredicate(ItemType::GOLD)).size() <= 30)
            target = chest;
        if (!target)
          warning[MORE_CHESTS] = true;
        else {
          warning[MORE_CHESTS] = false;
          addTask(Task::bringItem(this, pos, gold, *target));
          markedItems.insert(gold.begin(), gold.end());
        }
      } else {
        warning[NO_CHESTS] = true;
      }
    }
    vector<Item*> corpses = level->getSquare(pos)->getItems(unMarkedItems(ItemType::CORPSE));
    if (corpses.size() > 0) {
      if (!mySquares[SquareType::GRAVE].count(pos) && 
          !mySquares[SquareType::GRAVE].empty()) {
        Vec2 target = chooseRandom(mySquares[SquareType::GRAVE]);
        addTask(Task::bringItem(this, pos, {corpses[0]}, target));
        markedItems.insert({corpses[0]});
      }
      if (mySquares[SquareType::GRAVE].count(pos) && Random.roll(200) && 
          vampires.size() < mySquares[SquareType::GRAVE].size() && minions.size() < minionLimit) {
        for (Item* it : corpses)
          if (it->getCorpseInfo()->canBeRevived) {
            PCreature vampire = undeadFactory.random(MonsterAIFactory::collective(this));
            for (Vec2 v : pos.neighbors8(true))
              if (level->getSquare(v)->canEnter(vampire.get())) {
                level->getSquare(pos)->removeItems({it});
                vampires.push_back(vampire.get());
                addCreature(vampire.get());
                level->addCreature(v, std::move(vampire));
                break;
              }
            if (!vampire)
              break;
          }
      }
    }
    vector<Item*> equipment = level->getSquare(pos)->getItems([this](Item* it) {
        return minionEquipment.isItemUseful(it) && !markedItems.count(it); });
    if (!equipment.empty() && !mySquares[SquareType::WORKSHOP].empty() &&
        !mySquares[SquareType::WORKSHOP].count(pos)) {
      Vec2 target = chooseRandom(mySquares[SquareType::WORKSHOP]);
      addTask(Task::bringItem(this, pos, equipment, target));
      markedItems.insert(equipment.begin(), equipment.end());
    }
    if (marked.count(pos) && marked.at(pos)->isImpossible(level) && !taken.count(marked.at(pos)))
      removeTask(marked.at(pos));
  }
}

bool Collective::canSee(const Creature* c) const {
  return canSee(c->getPosition());
}

bool Collective::canSee(Vec2 position) const {
  return memory[level].hasViewIndex(position)
      || contains(level->getLandingSquares(StairDirection::DOWN, StairKey::DWARF), position)
      || contains(level->getLandingSquares(StairDirection::UP, StairKey::DWARF), position);
  for (Creature* member : creatures)
    if (member->canSee(position))
      return true;
  return false;
}

void Collective::setLevel(Level* l) {
  for (Vec2 v : l->getBounds())
    if (/*contains({SquareApplyType::ASCEND, SquareApplyType::DESCEND},
            l->getSquare(v)->getApplyType(Creature::getDefault())) ||*/
        l->getSquare(v)->getName() == "gold ore")
      memory[l].addObject(v, l->getSquare(v)->getViewObject());
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
  for (auto& elem : guardPosts) {
    bool isTraining = contains({MinionTask::TRAIN, MinionTask::TRAIN_IDLE}, minionTasks.at(c).getState());
    if (elem.second.attender == c) {
      if (isTraining) {
        minionTasks.at(c).update();
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
    bool isTraining = contains({MinionTask::TRAIN, MinionTask::TRAIN_IDLE}, minionTasks.at(c).getState());
    if (elem.second.attender == nullptr && isTraining) {
      elem.second.attender = c;
      if (taskMap.count(c))
        removeTask(taskMap.at(c));
    }
  }
 
  if (taskMap.count(c)) {
    Task* task = taskMap.at(c);
    if (task->isDone()) {
      removeTask(task);
    } else
      return task->getMove(c);
  }
  for (Vec2 v : mySquares[SquareType::WORKSHOP])
    for (Item* it : level->getSquare(v)->getItems([this, c] (const Item* it) {
          return minionEquipment.needsItem(c, it); })) {
      if (c->canEquip(it)) {
        addTask(Task::equipItem(this, v, it), c);
      }
      else
        addTask(Task::pickItem(this, v, {it}), c);
      return taskMap.at(c)->getMove(c);
    }
  minionTasks.at(c).update();
  if (c->getHealth() < 1)
    minionTasks.at(c).setState(MinionTask::SLEEP);
  switch (minionTasks.at(c).getState()) {
    case MinionTask::SLEEP: {
        set<Vec2>& whatBeds = (contains(vampires, c) ? mySquares[SquareType::GRAVE] : mySquares[SquareType::BED]);
        if (whatBeds.empty())
          return NoMove;
        addTask(Task::applySquare(this, whatBeds), c);
        minionTaskStrings[c] = "sleeping";
        break; }
    case MinionTask::TRAIN:
        if (mySquares[SquareType::TRAINING_DUMMY].empty())
          return NoMove;
        addTask(Task::applySquare(this, mySquares[SquareType::TRAINING_DUMMY]), c);
        minionTaskStrings[c] = "training";
        break;
    case MinionTask::TRAIN_IDLE:
        return NoMove;
    case MinionTask::WORKSHOP:
        if (mySquares[SquareType::WORKSHOP].empty())
          return NoMove;
        addTask(Task::applySquare(this, mySquares[SquareType::WORKSHOP]), c);
        minionTaskStrings[c] = "crafting";
        break;
    case MinionTask::WORKSHOP_IDLE:
        return NoMove;
    case MinionTask::EAT:
        if (mySquares[SquareType::HATCHERY].empty())
          return NoMove;
        minionTaskStrings[c] = "eating";
        addTask(Task::eat(this, mySquares[SquareType::HATCHERY]), c);
        break;
  }
  return taskMap.at(c)->getMove(c);
}

MoveInfo Collective::getMove(Creature* c) {
  if (!contains(creatures, c))
    return NoMove;
  if (!contains(imps, c)) {
    CHECK(contains(minions, c));
    return getMinionMove(c);
  }
  if (c->getLevel() != level)
    return NoMove;
  if (startImpNum == -1)
    startImpNum = imps.size();
  if (taskMap.count(c)) {
    Task* task = taskMap.at(c);
    if (task->isDone()) {
      removeTask(task);
    } else
      return task->getMove(c);
  }
  Task* closest = nullptr;
  for (PTask& task : tasks) {
    double dist = (task->getPosition() - c->getPosition()).length8();
    if ((!taken.count(task.get()) || (task->canTransfer() 
                                && (task->getPosition() - taken.at(task.get())->getPosition()).length8() > dist))
           && (!closest ||
           dist < (closest->getPosition() - c->getPosition()).length8()) && !locked.count(make_pair(c, task.get()))) {
      bool valid = task->getMove(c).isValid();
      if (valid)
        closest = task.get();
      else
        locked.insert(make_pair(c, task.get()));
    }
  }
  if (closest) {
    if (taken.count(closest)) {
      taskMap.erase(taken.at(closest));
      taken.erase(closest);
    }
    taskMap[c] = closest;
    taken[closest] = c;
    return closest->getMove(c);
  } else
    return NoMove;
}

MarkovChain<MinionTask> Collective::getTasksForMinion(Creature* c) {
  MinionTask t1, t2;
  if (c->getName() == "gnome") {
    t1 = MinionTask::WORKSHOP;
    t2 = MinionTask::WORKSHOP_IDLE;
  } else {
    t1 = MinionTask::TRAIN;
    t2 = MinionTask::TRAIN_IDLE;
  }
  return MarkovChain<MinionTask>(MinionTask::SLEEP, {
      {MinionTask::SLEEP, {{ MinionTask::EAT, 0.5}, { t1, 0.5}}},
      {MinionTask::EAT, {{ t1, 0.4}, { MinionTask::SLEEP, 0.2}}},
      {t1, {{ MinionTask::EAT, 0.005}, { MinionTask::SLEEP, 0.005}, {t2, 0.99}}},
      {t2, {{ t1, 1}}}});
}

void Collective::addCreature(Creature* c) {
  if (c->getName() == "dungeon heart") {
    CHECK(heart == nullptr) << "Too many dungeon hearts";
    heart = c;
    creatures.push_back(c);
    return;
  }
  creatures.push_back(c);
  if (!c->canConstruct(SquareType::FLOOR)) {
    minions.push_back(c);
    minionTasks.insert(make_pair(c, getTasksForMinion(c)));
  } else {
    imps.push_back(c);
 //   c->addEnemyVision([this](const Creature* c) { return canSee(c); });
  }
}

void Collective::onSquareReplacedEvent(const Level* l, Vec2 pos) {
  if (l == level) {
    bool found = false;
    for (auto& elem : mySquares)
      if (elem.second.count(pos)) {
        elem.second.erase(pos);
        found = true;
      }
    CHECK(found);
  }
}

void Collective::onTriggerEvent(const Level* l, Vec2 pos) {
  if (traps.count(pos) && l == level)
    traps.at(pos).armed = false;
}

void Collective::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim == heart) {
    messageBuffer.addMessage(MessageBuffer::important("Your dungeon heart was destroyed. "
          "You've been playing KeeperRL alpha."));
    exit(0);
  }
  if (contains(creatures, victim)) {
    Creature* c = const_cast<Creature*>(victim);
    removeElement(creatures, c);
    for (auto& elem : guardPosts)
      if (elem.second.attender == c)
        elem.second.attender = nullptr;
    if (contains(team, c))
      removeElement(team, c);
    if (taskMap.count(c)) {
      if (!taskMap.at(c)->canTransfer()) {
        taskMap.at(c)->cancel();
        removeTask(taskMap.at(c));
      } else {
        taken.erase(taskMap.at(c));
        taskMap.erase(c);
      }
    }
    if (contains(imps, c))
      removeElement(imps, c);
    if (contains(minions, c))
      removeElement(minions, c);
    if (contains(vampires, c))
      removeElement(vampires, c);
  }
}
  
const Level* Collective::getLevel() const {
  return level;
}
