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

#include "player.h"
#include "level.h"
#include "ranged_weapon.h"
#include "name_generator.h"
#include "game.h"
#include "options.h"
#include "creature.h"
#include "item_factory.h"
#include "effect.h"
#include "view_id.h"
#include "map_memory.h"
#include "player_message.h"
#include "item_action.h"
#include "game_info.h"
#include "equipment.h"
#include "spell.h"
#include "creature_name.h"
#include "view.h"
#include "view_index.h"
#include "music.h"
#include "model.h"
#include "collective.h"
#include "territory.h"
#include "creature_attributes.h"
#include "attack_level.h"
#include "villain_type.h"
#include "visibility_map.h"
#include "collective_name.h"
#include "view_object.h"
#include "body.h"
#include "furniture_usage.h"
#include "furniture.h"
#include "creature_debt.h"
#include "tutorial.h"
#include "message_generator.h"
#include "message_buffer.h"

template <class Archive>
void Player::serialize(Archive& ar, const unsigned int) {
  ar& SUBCLASS(Controller) & SUBCLASS(EventListener);
  ar(travelling, travelDir, target, displayGreeting, levelMemory, messageBuffer);
  ar(adventurer, visibilityMap, tutorial);
}

SERIALIZABLE(Player)

SERIALIZATION_CONSTRUCTOR_IMPL(Player)

Player::Player(WCreature c, bool adv, SMapMemory memory, SMessageBuffer buf, SVisibilityMap v, STutorial t) :
    Controller(c), levelMemory(memory), adventurer(adv), displayGreeting(adventurer), messageBuffer(buf),
    visibilityMap(v), tutorial(t) {
  visibilityMap->update(c, c->getVisibleTiles());
}

Player::~Player() {
}

void Player::onEvent(const GameEvent& event) {
  switch (event.getId()) {
    case EventId::MOVED:
      if (event.get<WCreature>() == getCreature())
        visibilityMap->update(getCreature(), getCreature()->getVisibleTiles());
      break;
    case EventId::PROJECTILE: {
        auto info = event.get<EventInfo::Projectile>();
        if (getCreature()->canSee(info.begin) || getCreature()->canSee(info.end))
          getView()->animateObject(info.begin.getCoord(), info.end.getCoord(), info.viewId);
      }
      break;
    case EventId::EXPLOSION: {
        Position pos = event.get<Position>();
        if (getCreature()->getPosition().isSameLevel(pos)) {
          if (getCreature()->canSee(pos))
            getView()->animation(pos.getCoord(), AnimationId::EXPLOSION);
          else
            privateMessage("BOOM!");
        }
      }
      break;
    case EventId::ALARM: {
        Position pos = event.get<Position>();
        Position myPos = getCreature()->getPosition();
        if (pos == myPos)
          privateMessage("An alarm sounds near you.");
        else if (pos.isSameLevel(myPos))
          privateMessage("An alarm sounds in the " +
              getCardinalName(myPos.getDir(pos).getBearing().getCardinalDir()));
      }
      break;
    case EventId::CONQUERED_ENEMY:
      if (adventurer) {
        WCollective col = event.get<WCollective>();
        if (auto& name = col->getName())
          privateMessage(PlayerMessage("The tribe of " + name->full + " is destroyed.",
                MessagePriority::CRITICAL));
        else
          privateMessage(PlayerMessage("An unnamed tribe is destroyed.", MessagePriority::CRITICAL));
      }
      break;
    case EventId::WON_GAME:
        if (adventurer)
          getGame()->conquered(*getCreature()->getName().first(), getCreature()->getKills().getSize(),
              getCreature()->getPoints());
        break;
    default:
      break;
  }
}

static string getSlotSuffix(EquipmentSlot slot) {
  return "(equipped)";
}

string Player::getInventoryItemName(WConstItem item, bool plural) const {
  if (getCreature()->getEquipment().isEquipped(item))
    return item->getNameAndModifiers(plural, getCreature()) + " "
      + getSlotSuffix(item->getEquipmentSlot());
  else
    return item->getNameAndModifiers(plural, getCreature());
}

void Player::getItemNames(vector<WItem> items, vector<ListElem>& names, vector<vector<WItem> >& groups,
    ItemPredicate predicate) {
  map<string, vector<WItem> > ret = groupBy<WItem, string>(items,
      [this] (WItem const& item) { return getInventoryItemName(item, false); });
  for (auto elem : ret) {
    if (elem.second.size() == 1)
      names.push_back(ListElem(getInventoryItemName(elem.second[0], false),
          predicate(elem.second[0]) ? ListElem::NORMAL : ListElem::INACTIVE).setTip(elem.second[0]->getDescription()));
    else
      names.push_back(ListElem(toString<int>(elem.second.size()) + " "
            + getInventoryItemName(elem.second[0], true),
          predicate(elem.second[0]) ? ListElem::NORMAL : ListElem::INACTIVE).setTip(elem.second[0]->getDescription()));
    groups.push_back(elem.second);
  }
}

void Player::pickUpItemAction(int numStack, bool multi) {
  CHECK(numStack >= 0);
  auto stacks = getCreature()->stackItems(getCreature()->getPickUpOptions());
  if (getUsableUsageType()) {
    --numStack;
    if (numStack == -1) {
      getCreature()->applySquare(getCreature()->getPosition()).perform(getCreature());
      return;
    }
  }
  if (numStack < stacks.size()) {
    vector<WItem> items = stacks[numStack];
    if (multi && items.size() > 1) {
      auto num = getView()->getNumber("Pick up how many " + items[0]->getName(true) + "?", 1, items.size());
      if (!num)
        return;
      items = getPrefix(items, *num);
    }
    tryToPerform(getCreature()->pickUp(items));
  }
}

bool Player::tryToPerform(CreatureAction action) {
  if (action)
    action.perform(getCreature());
  else
    privateMessage(action.getFailedReason());
  return !!action;
}

ItemClass typeDisplayOrder[] {
  ItemClass::WEAPON,
    ItemClass::RANGED_WEAPON,
    ItemClass::ARMOR,
    ItemClass::POTION,
    ItemClass::SCROLL,
    ItemClass::FOOD,
    ItemClass::BOOK,
    ItemClass::AMULET,
    ItemClass::RING,
    ItemClass::TOOL,
    ItemClass::CORPSE,
    ItemClass::OTHER,
    ItemClass::GOLD
};

static string getText(ItemClass type) {
  switch (type) {
    case ItemClass::WEAPON: return "Weapons";
    case ItemClass::RANGED_WEAPON: return "Ranged weapons";
    case ItemClass::AMULET: return "Amulets";
    case ItemClass::RING: return "Rings";
    case ItemClass::ARMOR: return "Armor";
    case ItemClass::SCROLL: return "Scrolls";
    case ItemClass::POTION: return "Potions";
    case ItemClass::FOOD: return "Comestibles";
    case ItemClass::BOOK: return "Books";
    case ItemClass::TOOL: return "Tools";
    case ItemClass::CORPSE: return "Corpses";
    case ItemClass::OTHER: return "Other";
    case ItemClass::GOLD: return "Gold";
  }
  FATAL << int(type);
  return "";
}


vector<WItem> Player::chooseItem(const string& text, ItemPredicate predicate, optional<UserInputId> exitAction) {
  map<ItemClass, vector<WItem> > typeGroups = groupBy<WItem, ItemClass>(
      getCreature()->getEquipment().getItems(), [](WItem const& item) { return item->getClass();});
  vector<ListElem> names;
  vector<vector<WItem> > groups;
  for (auto elem : typeDisplayOrder)
    if (typeGroups[elem].size() > 0) {
      names.push_back(ListElem(getText(elem), ListElem::TITLE));
      getItemNames(typeGroups[elem], names, groups, predicate);
    }
  optional<int> index = getView()->chooseFromList(text, names, 0, MenuType::NORMAL, nullptr, exitAction);
  if (index)
    return groups[*index];
  return vector<WItem>();
}

void Player::applyItem(vector<WItem> items) {
  if (getCreature()->isAffected(LastingEffect::BLIND) &&
      contains({ItemClass::SCROLL, ItemClass::BOOK}, items[0]->getClass())) {
    privateMessage("You can't read while blind!");
    return;
  }
  if (items[0]->getApplyTime() > 1) {
    for (WConstCreature c : getCreature()->getVisibleEnemies())
      if (getCreature()->getPosition().dist8(c->getPosition()) < 3) {
        if (!getView()->yesOrNoPrompt("Applying " + items[0]->getAName() + " takes " +
            toString(items[0]->getApplyTime()) + " turns. Are you sure you want to continue?"))
          return;
        else
          break;
      }
  }
  tryToPerform(getCreature()->applyItem(items[0]));
}

optional<Vec2> Player::chooseDirection(const string& s) {
  return getView()->chooseDirection(getCreature()->getPosition().getCoord(), s);
}

void Player::throwItem(vector<WItem> items, optional<Vec2> dir) {
  if (!dir) {
    auto cDir = chooseDirection("Which direction do you want to throw?");
    if (!cDir)
      return;
    dir = *cDir;
  }
  tryToPerform(getCreature()->throwItem(items[0], *dir));
}

vector<ItemAction> Player::getItemActions(const vector<WItem>& item) const {
  vector<ItemAction> actions;
  if (getCreature()->equip(item[0]))
    actions.push_back(ItemAction::EQUIP);
  if (getCreature()->applyItem(item[0]))
    actions.push_back(ItemAction::APPLY);
  if (getCreature()->unequip(item[0]))
    actions.push_back(ItemAction::UNEQUIP);
  else {
    actions.push_back(ItemAction::THROW);
    actions.push_back(ItemAction::DROP);
    if (item.size() > 1)
      actions.push_back(ItemAction::DROP_MULTI);
    if (item[0]->getShopkeeper(getCreature()))
      actions.push_back(ItemAction::PAY);
    for (Position v : getCreature()->getPosition().neighbors8())
      if (WCreature c = v.getCreature())
        if (getCreature()->isFriend(c)/* && c->canTakeItems(item)*/) {
          actions.push_back(ItemAction::GIVE);
          break;
        }
  }
  return actions;
}

void Player::handleItems(const EntitySet<Item>& itemIds, ItemAction action) {
  vector<WItem> items = getCreature()->getEquipment().getItems(
      [&](WConstItem it) { return itemIds.contains(it);});
  //CHECK(items.size() == itemIds.size()) << int(items.size()) << " " << int(itemIds.size());
  // the above assertion fails for unknown reason, so just fail this softly.
  if (items.empty() || (items.size() == 1 && action == ItemAction::DROP_MULTI))
    return;
  switch (action) {
    case ItemAction::DROP: tryToPerform(getCreature()->drop(items)); break;
    case ItemAction::DROP_MULTI:
      if (auto num = getView()->getNumber("Drop how many " + items[0]->getName(true) + "?", 1, items.size()))
        tryToPerform(getCreature()->drop(getPrefix(items, *num)));
      break;
    case ItemAction::THROW: throwItem(items); break;
    case ItemAction::APPLY: applyItem(items); break;
    case ItemAction::UNEQUIP: tryToPerform(getCreature()->unequip(items[0])); break;
    case ItemAction::GIVE: giveAction(items); break;
    case ItemAction::PAY: payForItemAction(items); break;
    case ItemAction::EQUIP: tryToPerform(getCreature()->equip(items[0])); break;
    default: FATAL << "Unhandled item action " << int(action);
  }
}

void Player::hideAction() {
  tryToPerform(getCreature()->hide());
}

bool Player::interruptedByEnemy() {
  vector<WCreature> enemies = getCreature()->getVisibleEnemies();
  vector<string> ignoreCreatures { "a boar" ,"a deer", "a fox", "a vulture", "a rat", "a jackal", "a boulder" };
  if (enemies.size() > 0) {
    for (WCreature c : enemies)
      if (!ignoreCreatures.contains(c->getName().a())) {
        privateMessage("You notice " + c->getName().a());
        return true;
      }
  }
  return false;
}

void Player::travelAction() {
/*  updateView = true;
  if (!getCreature()->move(travelDir) || getView()->travelInterrupt() || interruptedByEnemy()) {
    travelling = false;
    return;
  }
  tryToPerform(getCreature()->move(travelDir));
  const Location* currentLocation = getCreature()->getPosition().getLocation();
  if (lastLocation != currentLocation && currentLocation != nullptr && currentLocation->getName()) {
    privateMessage("You arrive at " + addAParticle(*currentLocation->getName()));
    travelling = false;
    return;
  }
  vector<Vec2> squareDirs = getCreature()->getPosition().getTravelDir();
  if (squareDirs.size() != 2) {
    travelling = false;
    INFO << "Stopped by multiple routes";
    return;
  }
  optional<int> myIndex = findElement(squareDirs, -travelDir);
  if (!myIndex) { // This was an assertion but was failing
    travelling = false;
    INFO << "Stopped by bad travel data";
    return;
  }
  travelDir = squareDirs[(*myIndex + 1) % 2];*/
}

void Player::targetAction() {
  updateView = true;
  CHECK(target);
  if (getCreature()->getPosition() == *target || getView()->travelInterrupt()) {
    target = none;
    return;
  }
  if (auto action = getCreature()->moveTowards(*target))
    action.perform(getCreature());
  else
    target = none;
  if (interruptedByEnemy() || !isTravelEnabled())
    target = none;
}

void Player::payForItemAction(const vector<WItem>& items) {
  int totalPrice = (int) items.size() * items[0]->getPrice();
  for (auto item : items) {
    CHECK(item->getShopkeeper(getCreature()));
    CHECK(item->getPrice() == items[0]->getPrice());
  }
  vector<WItem> gold = getCreature()->getGold(totalPrice);
  int canPayFor = (int) gold.size() / items[0]->getPrice();
  if (canPayFor == 0)
    privateMessage("You don't have enough gold to pay.");
  else if (canPayFor == items.size() || getView()->yesOrNoPrompt("You only have enough gold for " +
      toString(canPayFor) + " " + items[0]->getName(canPayFor > 1, getCreature()) + ". Still pay?"))
    tryToPerform(getCreature()->payFor(getPrefix(items, canPayFor)));
}

void Player::payForAllItemsAction() {
  if (int totalDebt = getCreature()->getDebt().getTotal()) {
    auto gold = getCreature()->getGold(totalDebt);
    if (gold.size() < totalDebt)
      privateMessage("You don't have enough gold to pay for everything.");
    else {
      for (auto creatureId : getCreature()->getDebt().getCreditors())
        for (WCreature c : getCreature()->getVisibleCreatures())
          if (c->getUniqueId() == creatureId &&
              getView()->yesOrNoPrompt("Give " + c->getName().the() + " " + toString(gold.size()) + " gold?")) {
              if (tryToPerform(getCreature()->give(c, gold)))
                for (auto item : getCreature()->getEquipment().getItems())
                  item->setShopkeeper(nullptr);
      }
    }
  }
}

void Player::giveAction(vector<WItem> items) {
  if (items.size() > 1) {
    if (auto num = getView()->getNumber("Give how many " + items[0]->getName(true) + "?", 1, items.size()))
      items = getPrefix(items, *num);
    else
      return;
  }
  vector<WCreature> creatures;
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && getView()->yesOrNoPrompt("Give " + items[0]->getTheName(items.size() > 1) +
        " to " + creatures[0]->getName().the() + "?"))
    tryToPerform(getCreature()->give(creatures[0], items));
  else if (auto dir = chooseDirection("Give whom?"))
    if (WCreature whom = getCreature()->getPosition().plus(*dir).getCreature())
      tryToPerform(getCreature()->give(whom, items));
}

void Player::chatAction(optional<Vec2> dir) {
  vector<WCreature> creatures;
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && !dir) {
    tryToPerform(getCreature()->chatTo(creatures[0]));
  } else
  if (creatures.size() > 1 || dir) {
    if (!dir)
      dir = chooseDirection("Which direction?");
    if (!dir)
      return;
    if (WCreature c = getCreature()->getPosition().plus(*dir).getCreature())
      tryToPerform(getCreature()->chatTo(c));
  }
}

void Player::fireAction() {
  if (auto testAction = getCreature()->fire(Vec2(1, 0))) {
    if (auto dir = chooseDirection("Fire which direction?"))
      fireAction(*dir);
  } else
    privateMessage(testAction.getFailedReason());
}

void Player::fireAction(Vec2 dir) {
  tryToPerform(getCreature()->fire(dir));
}

void Player::spellAction(SpellId id) {
  Spell* spell = Spell::get(id);
  if (!spell->isDirected())
    tryToPerform(getCreature()->castSpell(spell));
  else if (auto dir = chooseDirection("Which direction?"))
    tryToPerform(getCreature()->castSpell(spell, *dir));
}

const MapMemory& Player::getMemory() const {
  return *levelMemory;
}

void Player::sleeping() {
  if (getCreature()->isAffected(LastingEffect::HALLU))
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  MEASURE(
      getView()->updateView(this, false),
      "level render time");
}

static bool displayTravelInfo = true;

void Player::creatureAction(Creature::Id id) {
  if (getCreature()->getUniqueId() == id)
    tryToPerform(getCreature()->wait());
  else
    for (WCreature c : getCreature()->getVisibleCreatures())
      if (c->getUniqueId() == id) {
        if (!getCreature()->isEnemy(c) || !tryToPerform(getCreature()->attack(c)))
          tryToPerform(getCreature()->moveTowards(c->getPosition()));
      }
}

void Player::extendedAttackAction(Creature::Id id) {
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      if (c->getUniqueId() == id) {
        extendedAttackAction(c);
        return;
      }
}

void Player::extendedAttackAction(WCreature other) {
  vector<ListElem> elems;
  vector<AttackLevel> levels = getCreature()->getBody().getAttackLevels();
  for (auto level : levels)
    switch (level) {
      case AttackLevel::LOW: elems.push_back(ListElem("Low").setTip("Aim at lower parts of the body.")); break;
      case AttackLevel::MIDDLE: elems.push_back(ListElem("Middle").setTip("Aim at middle parts of the body.")); break;
      case AttackLevel::HIGH: elems.push_back(ListElem("High").setTip("Aim at higher parts of the body.")); break;
    }
  elems.push_back(ListElem("Wild").setTip("+20\% damage, -20\% accuracy, +50\% time spent."));
  elems.push_back(ListElem("Swift").setTip("-20\% damage, +20\% accuracy, -30\% time spent."));
  if (auto ind = getView()->chooseFromList("Choose attack parameters:", elems)) {
    if (*ind < levels.size())
      getCreature()->attack(other, CONSTRUCT(Creature::AttackParams, c.level = levels[*ind];)).perform(getCreature());
    else
      getCreature()->attack(other, CONSTRUCT(Creature::AttackParams, c.mod = Creature::AttackParams::Mod(*ind - levels.size());)).perform(getCreature());
  }
}

void Player::retireMessages() {
  auto& messages = messageBuffer->current;
  if (!messages.empty()) {
    messages = {messages.back()};
    messages[0].setFreshness(0);
  }
}

vector<Player::CommandInfo> Player::getCommands() const {
  bool canChat = false;
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      canChat = true;
  return {
    {PlayerInfo::CommandInfo{"Fire ranged weapon", 'f', "", true},
      [] (Player* player) { player->fireAction(); }, false},
    {PlayerInfo::CommandInfo{"Wait", ' ', "Skip this turn.", true},
      [] (Player* player) { player->tryToPerform(player->getCreature()->wait()); }, false},
    {PlayerInfo::CommandInfo{"Travel", 't', "Travel to another site.", !getGame()->isSingleModel()},
      [] (Player* player) { player->getGame()->transferAction(player->getTeam()); }, false},
    {PlayerInfo::CommandInfo{"Chat", 'c', "Chat with someone.", canChat},
      [] (Player* player) { player->chatAction(); }, false},
    {PlayerInfo::CommandInfo{"Hide", 'h', "Hide behind or under a terrain feature or piece of furniture.",
        !!getCreature()->hide()},
      [] (Player* player) { player->hideAction(); }, false},
    /*{PlayerInfo::CommandInfo{"Pay for all items", 'p', "Pay debt to a shopkeeper.", true},
      [] (Player* player) { player->payForAllItemsAction();}, false},*/
    {PlayerInfo::CommandInfo{"Drop everything", none, "Drop all items in possession.", !getCreature()->getEquipment().isEmpty()},
      [] (Player* player) { auto c = player->getCreature(); player->tryToPerform(c->drop(c->getEquipment().getItems())); }, false},
    {PlayerInfo::CommandInfo{"Message history", 'm', "Show message history.", true},
      [] (Player* player) { player->showHistory(); }, false},
  };
}

void Player::makeMove() {
  if (!isSubscribed())
    subscribeTo(getCreature()->getPosition().getModel());
  if (adventurer)
    considerAdventurerMusic();
  if (getCreature()->isAffected(LastingEffect::HALLU))
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  //if (updateView) { Check disabled so that we update in every frame to avoid some square refreshing issues.
    updateView = false;
    for (Position pos : getCreature()->getVisibleTiles()) {
      ViewIndex index;
      pos.getViewIndex(index, getCreature());
      levelMemory->update(pos, index);
    }
    MEASURE(
        getView()->updateView(this, false),
        "level render time");
  //}
  getView()->refreshView();
  /*if (displayTravelInfo && getCreature()->getPosition().getName() == "road"
      && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    getView()->presentText("", "Use ctrl + arrows to travel quickly on roads and corridors.");
    displayTravelInfo = false;
  }*/
  if (displayGreeting && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    CHECK(getCreature()->getName().first());
    getView()->updateView(this, true);
    getView()->presentText("", "Dear " + *getCreature()->getName().first() +
        ",\n \n \tIf you are reading this letter, then you have arrived in the valley of " +
        getGame()->getWorldName() + ". This land is swarming with evil, and you need to destroy all of it, "
        "like in a bad RPG game. Humans, dwarves and elves are your allies. "
        "Find them, talk to them, they will help you. Let your sword guide you.\n \n \nYours, " +
        NameGenerator::get(Random.choose(NameGeneratorId::FIRST_MALE, NameGeneratorId::FIRST_FEMALE))->getNext() +
        "\n \nPS.: Beware the orcs!");
    displayGreeting = false;
    getView()->updateView(this, false);
  }
  UserInput action = getView()->getAction();
  if (travelling && action.getId() == UserInputId::IDLE)
    travelAction();
  else if (target && action.getId() == UserInputId::IDLE)
    targetAction();
  else {
    INFO << "Action " << int(action.getId());
  vector<Vec2> direction;
  bool travel = false;
  bool wasJustTravelling = travelling || !!target;
  if (action.getId() != UserInputId::IDLE) {
    if (action.getId() != UserInputId::REFRESH)
      retireMessages();
    if (action.getId() == UserInputId::TILE_CLICK) {
      travelling = false;
      if (target)
        target = none;
      getView()->resetCenter();
    }
    updateView = true;
  }
  switch (action.getId()) {
    case UserInputId::FIRE: fireAction(action.get<Vec2>()); break;
    case UserInputId::TRAVEL: travel = true;
      FALLTHROUGH;
    case UserInputId::MOVE: direction.push_back(action.get<Vec2>()); break;
    case UserInputId::TILE_CLICK: {
      Position newPos = getCreature()->getPosition().withCoord(action.get<Vec2>());
      if (newPos.dist8(getCreature()->getPosition()) == 1) {
        Vec2 dir = getCreature()->getPosition().getDir(newPos);
        if (WCreature c = newPos.getCreature()) {
          creatureAction(c->getUniqueId());
          break;
        }
        direction.push_back(dir);
      } else
      if (newPos != getCreature()->getPosition() && !wasJustTravelling)
        target = newPos;
      break;
    }
    case UserInputId::INVENTORY_ITEM:
      handleItems(action.get<InventoryItemInfo>().items, action.get<InventoryItemInfo>().action); break;
    case UserInputId::PICK_UP_ITEM: pickUpItemAction(action.get<int>()); break;
    case UserInputId::PICK_UP_ITEM_MULTI: pickUpItemAction(action.get<int>(), true); break;
    case UserInputId::CAST_SPELL: spellAction(action.get<SpellId>()); break;
    case UserInputId::DRAW_LEVEL_MAP: getView()->drawLevelMap(this); break;
    case UserInputId::CREATURE_BUTTON: creatureAction(action.get<Creature::Id>()); break;
    case UserInputId::CREATURE_BUTTON2: extendedAttackAction(action.get<Creature::Id>()); break;
    case UserInputId::EXIT: getGame()->exitAction(); return;
    case UserInputId::PLAYER_COMMAND: {
        int index = action.get<int>();
        auto commands = getCommands();
        if (index >= 0 && index < commands.size()) {
          commands[index].perform(this);
          if (commands[index].actionKillsController)
            return;
        }
      }
      break;
    case UserInputId::PAY_DEBT:
        payForAllItemsAction();
      break;
    case UserInputId::TUTORIAL_CONTINUE:
        if (tutorial)
          tutorial->continueTutorial(getGame());
        break;
    case UserInputId::TUTORIAL_GO_BACK:
        if (tutorial)
          tutorial->goBack();
        break;
#ifndef RELEASE
    case UserInputId::CHEAT_ATTRIBUTES:
      getCreature()->getAttributes().setBaseAttr(AttrType::DAMAGE, 80);
      getCreature()->getAttributes().setBaseAttr(AttrType::DEFENSE, 80);
      getCreature()->getAttributes().setBaseAttr(AttrType::SPELL_DAMAGE, 80);
      getCreature()->getAttributes().setBaseAttr(AttrType::SPEED, 200);
      getCreature()->addPermanentEffect(LastingEffect::FLYING, true);
      break;
#endif
    default: break;
  }
  if (getCreature()->isAffected(LastingEffect::SLEEP)) {
    onFellAsleep();
    return;
  }
  for (Vec2 dir : direction)
    if (travel) {
      if (WCreature other = getCreature()->getPosition().plus(dir).getCreature())
        extendedAttackAction(other);
      else {
/*        vector<Vec2> squareDirs = getCreature()->getPosition().getTravelDir();
        if (findElement(squareDirs, dir)) {
          travelDir = dir;
          lastLocation = getCreature()->getPosition().getLocation();
          travelling = true;
          travelAction();
        }*/
      }
    } else {
      moveAction(dir);
      break;
    }
  }
}

void Player::showHistory() {
  PlayerMessage::presentMessages(getView(), messageBuffer->history);
}

static string getForceMovementQuestion(Position pos, WConstCreature creature) {
  if (pos.canEnterEmpty(creature))
    return "";
  else if (pos.isBurning())
    return "Walk into the fire?";
  else if (pos.canEnterEmpty(MovementTrait::SWIM))
    return "The water is very deep, are you sure?";
  else if (pos.sunlightBurns() && creature->getMovementType().isSunlightVulnerable())
    return "Walk into the sunlight?";
  else if (pos.isTribeForbidden(creature->getTribeId()))
    return "Walk into the forbidden zone?";
  else
    return "Walk into the " + pos.getName() + "?";
}

void Player::moveAction(Vec2 dir) {
  if (tryToPerform(getCreature()->move(dir)))
    return;
  if (auto action = getCreature()->forceMove(dir)) {
    string nextQuestion = getForceMovementQuestion(getCreature()->getPosition().plus(dir), getCreature());
    string hereQuestion = getForceMovementQuestion(getCreature()->getPosition(), getCreature());
    if (hereQuestion == nextQuestion || getView()->yesOrNoPrompt(nextQuestion, true))
      action.perform(getCreature());
  } else if (auto action = getCreature()->bumpInto(dir))
    action.perform(getCreature());
  else if (!getCreature()->getPosition().plus(dir).canEnterEmpty(getCreature()))
    tryToPerform(getCreature()->destroy(dir, DestroyAction::Type::BASH));
}

bool Player::isPlayer() const {
  return true;
}

void Player::privateMessage(const PlayerMessage& message) {
  if (View* view = getView()) {
    if (message.getText().size() < 2)
      return;
    if (auto title = message.getAnnouncementTitle())
      view->presentText(*title, message.getText());
    else {
      messageBuffer->history.push_back(message);
      auto& messages = messageBuffer->current;
      if (!messages.empty() && messages.back().getFreshness() < 1)
        messages.clear();
      messages.emplace_back(message);
      if (message.getPriority() == MessagePriority::CRITICAL)
        view->presentText("Important!", message.getText());
    }
  }
}

WLevel Player::getLevel() const {
  return getCreature()->getLevel();
}

WGame Player::getGame() const {
  if (auto creature = getCreature())
    return creature->getGame();
  else
    return nullptr;
}

View* Player::getView() const {
  if (WGame game = getGame())
    return game->getView();
  else
    return nullptr;
}

Vec2 Player::getPosition() const {
  return getCreature()->getPosition().getCoord();
}

static MessageGenerator messageGenerator(MessageGenerator::SECOND_PERSON);

MessageGenerator& Player::getMessageGenerator() const {
  return messageGenerator;
}

void Player::onStartedControl() {
  getGame()->addPlayer(getCreature());
}

void Player::onEndedControl() {
  if (auto game = getGame()) // if the whole Game is being destructed then we get null here
    game->removePlayer(getCreature());
}

void Player::getViewIndex(Vec2 pos, ViewIndex& index) const {
  bool canSee = visibilityMap->isVisible(Position(pos, getLevel())) ||
      getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP);
  Position position = getCreature()->getPosition().withCoord(pos);
  if (canSee)
    position.getViewIndex(index, getCreature());
  else
    index.setHiddenId(position.getViewObject().id());
  if (!canSee)
    if (auto memIndex = getMemory().getViewIndex(position))
      index.mergeFromMemory(*memIndex);
  if (position.isTribeForbidden(getCreature()->getTribeId()))
    index.setHighlight(HighlightType::FORBIDDEN_ZONE);
  if (WConstCreature c = position.getCreature()) {
    if ((canSee && getCreature()->canSeeDisregardingPosition(c)) || c == getCreature()) {
      index.insert(c->getViewObjectFor(getCreature()->getTribe()));
      if (c == getCreature())
        index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::PLAYER);
      if (getTeam().contains(c))
        index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_HIGHLIGHT);
      if (getCreature()->isEnemy(c))
        index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::HOSTILE);
    } else if (getCreature()->isUnknownAttacker(c))
      index.insert(copyOf(ViewObject::unknownMonster()));
  }
 /* if (pos == getCreature()->getPosition() && index.hasObject(ViewLayer::CREATURE))
      index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_LEADER_HIGHLIGHT);*/

}

void Player::onKilled(WConstCreature attacker) {
  unsubscribe();
  getView()->updateView(this, false);
  if (getGame()->getPlayerCreatures().size() == 1 && getView()->yesOrNoPrompt("Display message history?"))
    showHistory();
  if (adventurer)
    getGame()->gameOver(getCreature(), getCreature()->getKills().getSize(), "monsters", getCreature()->getPoints());
}

void Player::onFellAsleep() {
}

vector<WCreature> Player::getTeam() const {
  return {getCreature()};
}

bool Player::isTravelEnabled() const {
  return true;
}

optional<FurnitureUsageType> Player::getUsableUsageType() const {
  if (auto furniture = getCreature()->getPosition().getFurniture(FurnitureLayer::MIDDLE))
    if (furniture->canUse(getCreature()))
      if (auto usageType = furniture->getUsageType())
        if (!FurnitureUsage::getUsageQuestion(*usageType, getCreature()->getPosition().getName()).empty())
          return usageType;
  return none;
}

void Player::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.messageBuffer = messageBuffer->current;
  gameInfo.singleModel = getGame()->isSingleModel();
  gameInfo.infoType = GameInfo::InfoType::PLAYER;
  SunlightInfo sunlightInfo = getGame()->getSunlightInfo();
  gameInfo.sunlightInfo.description = sunlightInfo.getText();
  gameInfo.sunlightInfo.timeRemaining = sunlightInfo.getTimeRemaining();
  gameInfo.time = getCreature()->getGame()->getGlobalTime();
  gameInfo.playerInfo = PlayerInfo(getCreature());
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
  info.team.clear();
  for (WConstCreature c : getTeam())
    info.team.push_back(c);
  info.levelName = getLevel()->getName();
  info.lyingItems.clear();
  if (auto usageType = getUsableUsageType()) {
    string question = FurnitureUsage::getUsageQuestion(*usageType, getCreature()->getPosition().getName());
    info.lyingItems.push_back(getFurnitureUsageInfo(question, getCreature()->getPosition().getViewObject().id()));
  }
  for (auto stack : getCreature()->stackItems(getCreature()->getPickUpOptions()))
    info.lyingItems.push_back(getItemInfo(stack));
  info.inventory.clear();
  map<ItemClass, vector<WItem> > typeGroups = groupBy<WItem, ItemClass>(
      getCreature()->getEquipment().getItems(), [](WItem const& item) { return item->getClass();});
  info.debt = getCreature()->getDebt().getTotal();
  for (auto elem : typeDisplayOrder)
    if (typeGroups[elem].size() > 0)
      append(info.inventory, getItemInfos(typeGroups[elem]));
  info.commands = getCommands().transform([](const CommandInfo& info) -> PlayerInfo::CommandInfo { return info.commandInfo;});
  if (tutorial)
    tutorial->refreshInfo(getGame(), gameInfo.tutorial);
}

ItemInfo Player::getFurnitureUsageInfo(const string& question, ViewId viewId) const {
  return CONSTRUCT(ItemInfo,
    c.name = question;
    c.fullName = c.name;
    c.description = "Click to " + c.name;
    c.number = 1;
    c.viewId = viewId;);
}

ItemInfo Player::getItemInfo(const vector<WItem>& stack) const {
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(getCreature());
    c.fullName = stack[0]->getNameAndModifiers(false, getCreature());
    c.description = getCreature()->isAffected(LastingEffect::BLIND) ? "" : stack[0]->getDescription();
    c.number = stack.size();
    c.viewId = stack[0]->getViewObject().id();
    for (auto it : stack)
      c.ids.insert(it->getUniqueId());
    c.actions = getItemActions(stack);
    c.equiped = getCreature()->getEquipment().isEquipped(stack[0]);
    c.weight = stack[0]->getWeight();
    if (stack[0]->getShopkeeper(getCreature()))
      c.price = make_pair(ViewId::GOLD, stack[0]->getPrice());
  );
}

vector<ItemInfo> Player::getItemInfos(const vector<WItem>& items) const {
  map<string, vector<WItem> > stacks = groupBy<WItem, string>(items,
      [this] (WItem const& item) { return getInventoryItemName(item, false); });
  vector<ItemInfo> ret;
  for (auto elem : stacks)
    ret.push_back(getItemInfo(elem.second));
  return ret;
}

vector<Vec2> Player::getVisibleEnemies() const {
  return getCreature()->getVisibleEnemies().transform(
      [](WConstCreature c) { return c->getPosition().getCoord(); });
}

double Player::getLocalTime() const {
  return getCreature()->getLocalTime();
}

Player::CenterType Player::getCenterType() const {
  return CenterType::FOLLOW;
}

vector<Vec2> Player::getUnknownLocations(WConstLevel level) const {
  vector<Vec2> ret;
  for (auto col : getCreature()->getPosition().getModel()->getCollectives())
    if (col->getLevel() == getLevel())
      if (auto& pos = col->getTerritory().getCentralPoint())
        if (!getMemory().getViewIndex(*pos))
          ret.push_back(pos->getCoord());
  return ret;
}

void Player::considerAdventurerMusic() {
  for (WCollective col : getCreature()->getPosition().getModel()->getCollectives())
    if (col->getVillainType() == VillainType::MAIN && !col->isConquered() &&
        col->getTerritory().contains(getCreature()->getPosition())) {
      getGame()->setCurrentMusic(MusicType::ADV_BATTLE, true);
      return;
    }
  getGame()->setCurrentMusic(MusicType::ADV_PEACEFUL, true);
}

REGISTER_TYPE(ListenerTemplate<Player>)
