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
#include "location.h"
#include "level.h"
#include "ranged_weapon.h"
#include "name_generator.h"
#include "model.h"
#include "options.h"
#include "creature.h"
#include "square.h"
#include "pantheon.h"
#include "item_factory.h"
#include "effect.h"
#include "view_id.h"
#include "map_memory.h"

template <class Archive> 
void Player::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Controller)
    & SVAR(travelling)
    & SVAR(travelDir)
    & SVAR(target)
    & SVAR(lastLocation)
    & SVAR(specialCreatures)
    & SVAR(displayGreeting)
    & SVAR(levelMemory)
    & SVAR(usedEpithets)
    & SVAR(model)
    & SVAR(messages)
    & SVAR(messageHistory);
}

SERIALIZABLE(Player);

SERIALIZATION_CONSTRUCTOR_IMPL(Player);

Player::Player(Creature* c, Model* m, bool greeting, map<UniqueEntity<Level>::Id, MapMemory>* memory) :
    Controller(c), levelMemory(memory), model(m), displayGreeting(greeting) {
}

Player::~Player() {
}

void Player::onThrowEvent(const Level* l, const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) {
  if (l == getCreature()->getLevel())
    for (Vec2 v : trajectory)
      if (getCreature()->canSee(v)) {
        model->getView()->animateObject(trajectory, item->getViewObject());
        return;
      }
}

void Player::learnLocation(const Location* loc) {
  for (Vec2 v : loc->getBounds())
    (*levelMemory)[getCreature()->getLevel()->getUniqueId()]
        .addObject(v, getCreature()->getLevel()->getSafeSquare(v)->getViewObject());
}

void Player::onExplosionEvent(const Level* level, Vec2 pos) {
  if (level == getCreature()->getLevel()) {
    if (getCreature()->canSee(pos))
      model->getView()->animation(pos, AnimationId::EXPLOSION);
    else
      privateMessage("BOOM!");
  }
}

ControllerFactory Player::getFactory(Model *m, map<UniqueEntity<Level>::Id, MapMemory>* levelMemory) {
  return ControllerFactory([=](Creature* c) { return new Player(c, m, true, levelMemory);});
}

static string getSlotSuffix(EquipmentSlot slot) {
  return "(equiped)";
/*  switch (slot) {
    case EquipmentSlot::WEAPON: return "(weapon ready)";
    case EquipmentSlot::RANGED_WEAPON: return "(ranged weapon ready)";
    default: return "(being worn)";
  }*/
}

void Player::onBump(Creature*) {
  FAIL << "Shouldn't call onBump on a player";
}

string Player::getInventoryItemName(const Item* item, bool plural) const {
  if (getCreature()->getEquipment().isEquiped(item))
    return item->getNameAndModifiers(plural, getCreature()->isBlind()) + " " 
      + getSlotSuffix(item->getEquipmentSlot());
  else
    return item->getNameAndModifiers(plural, getCreature()->isBlind());
}

void Player::getItemNames(vector<Item*> items, vector<View::ListElem>& names, vector<vector<Item*> >& groups,
    ItemPredicate predicate) {
  map<string, vector<Item*> > ret = groupBy<Item*, string>(items, 
      [this] (Item* const& item) { return getInventoryItemName(item, false); });
  for (auto elem : ret) {
    if (elem.second.size() == 1)
      names.push_back(View::ListElem(getInventoryItemName(elem.second[0], false),
          predicate(elem.second[0]) ? View::NORMAL : View::INACTIVE).setTip(elem.second[0]->getDescription()));
    else
      names.push_back(View::ListElem(toString<int>(elem.second.size()) + " " 
            + getInventoryItemName(elem.second[0], true),
          predicate(elem.second[0]) ? View::NORMAL : View::INACTIVE).setTip(elem.second[0]->getDescription()));
    groups.push_back(elem.second);
  }
}

static string getSquareQuestion(SquareApplyType type, string name) {
  switch (type) {
    case SquareApplyType::DESCEND: return "Descend the " + name;
    case SquareApplyType::ASCEND: return "Ascend the " + name;
    case SquareApplyType::USE_CHEST: return "Open the " + name;
    case SquareApplyType::DRINK: return "Drink from the " + name;
    case SquareApplyType::PRAY: return "Pray at the " + name;
    case SquareApplyType::SLEEP: return "Go to sleep on the " + name;
    default: break;
  }
  return "";
}

void Player::pickUpAction(bool extended) {
  auto items = getCreature()->getPickUpOptions();
  const Square* square = getCreature()->getSquare();
  if (square->getApplyType(getCreature())) {
    string question = getSquareQuestion(*square->getApplyType(getCreature()), square->getName());
    if (!question.empty() && (items.empty() || model->getView()->yesOrNoPrompt(question))) {
      getCreature()->applySquare().perform(getCreature());
      return;
    }
  }
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(getCreature()->getPickUpOptions(), names, groups);
  CHECK(groups.size() == names.size()) << int(groups.size()) << " " << int(names.size());
  if (names.empty())
    return;
  int index = 0;
  if (names.size() > 1) {
    optional<int> res = model->getView()->chooseFromList("Choose an item to pick up:", names);
    if (!res)
      return;
    else
      index = *res;
  }
  CHECK(index >= 0 && index < groups.size()) << index << " " << int(groups.size());
  int num = groups[index].size();
  if (num < 1)
    return;
  if (extended && num > 1) {
    optional<int> res = model->getView()->getNumber("Pick up how many " + groups[index][0]->getName(true) + "?",
        1, num);
    if (!res)
      return;
    num = *res;
  }
  vector<Item*> pickUpItems = getPrefix(groups[index], num);
  tryToPerform(getCreature()->pickUp(pickUpItems));
}

void Player::pickUpItemAction(int numItem) {
  auto items = getCreature()->stackItems(getCreature()->getPickUpOptions());
  if (numItem < items.size())
    tryToPerform(getCreature()->pickUp(items[numItem]));
}

void Player::tryToPerform(CreatureAction action) {
  if (action)
    action.perform(getCreature());
  else
    getCreature()->playerMessage(action.getFailedReason());
}

ItemClass typeDisplayOrder[] {
  ItemClass::WEAPON,
    ItemClass::RANGED_WEAPON,
    ItemClass::AMMO,
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
    case ItemClass::AMMO: return "Projectiles";
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
  FAIL << int(type);
  return "";
}


vector<Item*> Player::chooseItem(const string& text, ItemPredicate predicate, optional<UserInputId> exitAction) {
  map<ItemClass, vector<Item*> > typeGroups = groupBy<Item*, ItemClass>(
      getCreature()->getEquipment().getItems(), [](Item* const& item) { return item->getClass();});
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  for (auto elem : typeDisplayOrder) 
    if (typeGroups[elem].size() > 0) {
      names.push_back(View::ListElem(getText(elem), View::TITLE));
      getItemNames(typeGroups[elem], names, groups, predicate);
    }
  optional<int> index = model->getView()->chooseFromList(text, names, 0, View::NORMAL_MENU, nullptr, exitAction);
  if (index)
    return groups[*index];
  return vector<Item*>();
}

void Player::dropAction(bool extended) {
  vector<Item*> items = chooseItem("Choose an item to drop:", [this](const Item* item) {
      return !getCreature()->getEquipment().isEquiped(item) || item->getClass() == ItemClass::WEAPON;}, UserInputId::DROP);
  int num = items.size();
  if (num < 1)
    return;
  if (extended && num > 1) {
    optional<int> res = model->getView()->getNumber("Drop how many " + items[0]->getName(true, getCreature()->isBlind()) 
        + "?", 1, num);
    if (!res)
      return;
    num = *res;
  }
  tryToPerform(getCreature()->drop(getPrefix(items, num)));
}

void Player::applyAction() {
  vector<Item*> items = chooseItem("Choose an item to apply:", [this](const Item* item) {
      return getCreature()->applyItem(const_cast<Item*>(item));}, UserInputId::APPLY_ITEM);
  if (items.size() == 0)
    return;
  applyItem(items);
}

void Player::applyItem(vector<Item*> items) {
  if (getCreature()->isBlind() && contains({ItemClass::SCROLL, ItemClass::BOOK}, items[0]->getClass())) {
    privateMessage("You can't read while blind!");
    return;
  }
  if (items[0]->getApplyTime() > 1) {
    for (const Creature* c : getCreature()->getVisibleEnemies())
      if ((c->getPosition() - getCreature()->getPosition()).length8() < 3) { 
        if (!model->getView()->yesOrNoPrompt("Applying " + items[0]->getAName() + " takes " + 
            toString(items[0]->getApplyTime()) + " turns. Are you sure you want to continue?"))
          return;
        else
          break;
      }
  }
  tryToPerform(getCreature()->applyItem(items[0]));
}

void Player::throwAction(optional<Vec2> dir) {
  vector<Item*> items = chooseItem("Choose an item to throw:", [this](const Item* item) {
      return !getCreature()->getEquipment().isEquiped(item);}, UserInputId::THROW);
  if (items.size() == 0)
    return;
  throwItem(items, dir);
}

void Player::throwItem(vector<Item*> items, optional<Vec2> dir) {
  if (items[0]->getClass() == ItemClass::AMMO && model->getOptions()->getBoolValue(OptionId::HINTS))
    privateMessage(PlayerMessage("To fire arrows equip a bow and use alt + direction key", PlayerMessage::CRITICAL));
  if (!dir) {
    auto cDir = model->getView()->chooseDirection("Which direction do you want to throw?");
    if (!cDir)
      return;
    dir = *cDir;
  }
  tryToPerform(getCreature()->throwItem(items[0], *dir));
}

void Player::consumeAction() {
  vector<CreatureAction> actions;
  for (Vec2 v : Vec2::directions8())
    if (auto action = getCreature()->consume(v))
      actions.push_back(action);
  if (actions.size() == 1) {
    tryToPerform(actions[0]);
  } else
  if (actions.size() > 1) {
    auto dir = model->getView()->chooseDirection("Which direction?");
    if (!dir)
      return;
    tryToPerform(getCreature()->consume(*dir));
  }
}

typedef GameInfo::PlayerInfo::ItemInfo ItemInfo;

vector<ItemInfo::Action> Player::getItemActions(Item* item) const {
  vector<ItemInfo::Action> actions;
  if (getCreature()->equip(item)) {
    actions.push_back(ItemInfo::EQUIP);
  }
  if (getCreature()->applyItem(item)) {
    actions.push_back(ItemInfo::APPLY);
  }
  if (getCreature()->unequip(item))
    actions.push_back(ItemInfo::UNEQUIP);
  else {
    actions.push_back(ItemInfo::THROW);
    actions.push_back(ItemInfo::DROP);
  }
  return actions;
}

void Player::handleItems(const vector<UniqueEntity<Item>::Id>& itemIds, ItemInfo::Action action) {
  vector<Item*> items = getCreature()->getEquipment().getItems(
      [&](const Item* it) { return contains(itemIds, it->getUniqueId());});
  //CHECK(items.size() == itemIds.size()) << int(items.size()) << " " << int(itemIds.size());
  if (items.empty()) // the above assertion fails for unknown reason, so just fail this softly.
    return;
  switch (action) {
    case ItemInfo::DROP: tryToPerform(getCreature()->drop(items)); break;
    case ItemInfo::THROW: throwItem(items); break;
    case ItemInfo::APPLY: applyItem(items); break;
    case ItemInfo::UNEQUIP: tryToPerform(getCreature()->unequip(items[0])); break;
    case ItemInfo::EQUIP: tryToPerform(getCreature()->equip(items[0])); break;
  }
}

void Player::hideAction() {
  tryToPerform(getCreature()->hide());
}

bool Player::interruptedByEnemy() {
  vector<const Creature*> enemies = getCreature()->getVisibleEnemies();
  vector<string> ignoreCreatures { "a boar" ,"a deer", "a fox", "a vulture", "a rat", "a jackal", "a boulder" };
  if (enemies.size() > 0) {
    for (const Creature* c : enemies)
      if (!contains(ignoreCreatures, c->getName().a())) {
        model->getView()->updateView(this, false);
        privateMessage("You notice " + c->getName().a());
        return true;
      }
  }
  return false;
}

void Player::travelAction() {
  updateView = true;
  if (!getCreature()->move(travelDir) || model->getView()->travelInterrupt() || interruptedByEnemy()) {
    travelling = false;
    return;
  }
  tryToPerform(getCreature()->move(travelDir));
  const Location* currentLocation = getCreature()->getLevel()->getLocation(getCreature()->getPosition());
  if (lastLocation != currentLocation && currentLocation != nullptr && currentLocation->hasName()) {
    privateMessage("You arrive at " + addAParticle(currentLocation->getName()));
    travelling = false;
    return;
  }
  vector<Vec2> squareDirs = getCreature()->getSquare()->getTravelDir();
  if (squareDirs.size() != 2) {
    travelling = false;
    Debug() << "Stopped by multiple routes";
    return;
  }
  optional<int> myIndex = findElement(squareDirs, -travelDir);
  if (!myIndex) { // This was an assertion but was failing
    travelling = false;
    Debug() << "Stopped by bad travel data";
    return;
  }
  travelDir = squareDirs[(*myIndex + 1) % 2];
}

void Player::targetAction() {
  updateView = true;
  CHECK(target);
  if (getCreature()->getPosition() == *target || model->getView()->travelInterrupt()) {
    target = none;
    return;
  }
  if (auto action = getCreature()->moveTowards(*target))
    action.perform(getCreature());
  else
    target = none;
  if (interruptedByEnemy())
    target = none;
}

void Player::payDebtAction() {
  for (Square* square : getCreature()->getSquares(Vec2::directions8()))
    if (const Creature* c = square->getCreature()) {
      if (int debt = c->getDebt(getCreature())) {
        vector<Item*> gold = getCreature()->getGold(debt);
        if (gold.size() < debt) {
          privateMessage("You don't have enough gold to pay.");
        } else if (model->getView()->yesOrNoPrompt("Buy items for " + toString(debt) + " zorkmids?")) {
          privateMessage("You pay " + c->getName().the() + " " + toString(debt) + " zorkmids.");
          getCreature()->give(c, gold);
        }
      } else {
        Debug() << "No debt " << c->getName().bare();
      }
    }
}

void Player::chatAction(optional<Vec2> dir) {
  vector<const Creature*> creatures;
  for (Square* square : getCreature()->getSquares(Vec2::directions8()))
    if (const Creature* c = square->getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && !dir) {
    tryToPerform(getCreature()->chatTo(creatures[0]->getPosition() - getCreature()->getPosition()));
  } else
  if (creatures.size() > 1 || dir) {
    if (!dir)
      dir = model->getView()->chooseDirection("Which direction?");
    if (!dir)
      return;
    tryToPerform(getCreature()->chatTo(*dir));
  }
}

void Player::fireAction(Vec2 dir) {
  tryToPerform(getCreature()->fire(dir));
}

void Player::spellAction() {
  vector<View::ListElem> list;
  vector<Spell*> spells = getCreature()->getSpells();
  for (Spell* spell : spells)
    list.push_back(View::ListElem(spell->getName() + " " + (!getCreature()->isReady(spell) ? "(ready in " +
          toString(int(getCreature()->getSpellDelay(spell) + 0.9999)) + " turns)" : ""),
          getCreature()->isReady(spell) ? View::NORMAL : View::INACTIVE).setTip(spell->getDescription()));
  auto index = model->getView()->chooseFromList("Cast a spell:", list);
  if (!index)
    return;
  Spell* spell = spells[*index];
  if (!spell->isDirected())
    tryToPerform(getCreature()->castSpell(spell));
  else if (auto dir = model->getView()->chooseDirection("Which direction?"))
    tryToPerform(getCreature()->castSpell(spell, *dir));
}

const MapMemory& Player::getMemory() const {
  return (*levelMemory)[getCreature()->getLevel()->getUniqueId()];
}

void Player::sleeping() {
  if (getCreature()->isAffected(LastingEffect::HALLU))
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  MEASURE(
      model->getView()->updateView(this, false),
      "level render time");
}

static bool displayTravelInfo = true;

void Player::attackAction(Creature* other) {
  vector<View::ListElem> elems;
  vector<AttackLevel> levels = getCreature()->getAttackLevels();
  for (auto level : levels)
    switch (level) {
      case AttackLevel::LOW: elems.push_back("Low"); break;
      case AttackLevel::MIDDLE: elems.push_back("Middle"); break;
      case AttackLevel::HIGH: elems.push_back("High"); break;
    }
  if (auto ind = model->getView()->chooseFromList("Choose level of the attack:", elems))
    getCreature()->attack(other, levels[*ind]).perform(getCreature());
}

void Player::retireMessages() {
  if (!messages.empty()) {
    messages = {messages.back()};
    messages[0].setFreshness(0);
  }
}

void Player::makeMove() {
  vector<Vec2> squareDirs = getCreature()->getSquare()->getTravelDir();
  if (getCreature()->isAffected(LastingEffect::HALLU))
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  if (updateView) {
    updateView = false;
    for (Vec2 pos : getCreature()->getLevel()->getVisibleTiles(getCreature())) {
      ViewIndex index;
      getViewIndex(pos, index);
      (*levelMemory)[getCreature()->getLevel()->getUniqueId()].update(pos, index);
    }
    MEASURE(
        model->getView()->updateView(this, false),
        "level render time");
  }
  if (displayTravelInfo && getCreature()->getSquare()->getName() == "road" 
      && model->getOptions()->getBoolValue(OptionId::HINTS)) {
    model->getView()->presentText("", "Use ctrl + arrows to travel quickly on roads and corridors.");
    displayTravelInfo = false;
  }
  if (displayGreeting && model->getOptions()->getBoolValue(OptionId::HINTS)) {
    CHECK(getCreature()->getFirstName());
    model->getView()->presentText("", "Dear " + *getCreature()->getFirstName() + ",\n \n \tIf you are reading this letter, then you have arrived in the valley of " + model->getWorldName() + ". There is a band of dwarves dwelling in caves under a mountain. Find them, talk to them, they will help you. Let your sword guide you.\n \n \nYours, " + NameGenerator::get(NameGeneratorId::FIRST)->getNext() + "\n \nPS.: Beware the orcs!");
    model->getView()->presentText("", "Judging by the corpses lying around here, you suspect that new circumstances may have arisen.");
    displayGreeting = false;
    model->getView()->updateView(this, false);
  }
  for (const Creature* c : getCreature()->getVisibleEnemies()) {
    if (c->isSpecialMonster() && !contains(specialCreatures, c)) {
      privateMessage(PlayerMessage(c->getDescription(), PlayerMessage::CRITICAL));
      specialCreatures.push_back(c);
    }
  }
  UserInput action = model->getView()->getAction();
  if (travelling && action.getId() == UserInputId::IDLE)
    travelAction();
  else if (target&& action.getId() == UserInputId::IDLE)
    targetAction();
  else {
    Debug() << "Action " << int(action.getId());
  vector<Vec2> direction;
  bool travel = false;
  bool wasJustTravelling = travelling || !!target;
  if (action.getId() != UserInputId::IDLE) {
    if (action.getId() != UserInputId::REFRESH && action.getId() != UserInputId::BUTTON_RELEASE) {
      retireMessages();
      travelling = false;
      target = none;
    }
    updateView = true;
  }
  switch (action.getId()) {
    case UserInputId::FIRE: fireAction(action.get<Vec2>()); break;
    case UserInputId::TRAVEL: travel = true;
    case UserInputId::MOVE: direction.push_back(action.get<Vec2>()); break;
    case UserInputId::MOVE_TO: 
      if (action.get<Vec2>().dist8(getCreature()->getPosition()) == 1) {
        Vec2 dir = action.get<Vec2>() - getCreature()->getPosition();
        for (Square* square : getCreature()->getSquare(dir))
          if (const Creature* c = square->getCreature()) {
            if (!getCreature()->isEnemy(c)) {
              chatAction(dir);
              break;
            }
          }
        direction.push_back(dir);
      } else
      if (action.get<Vec2>() != getCreature()->getPosition()) {
        if (!wasJustTravelling) {
          target = action.get<Vec2>();
          target = Vec2(min(getCreature()->getLevel()->getBounds().getKX() - 1, max(0, target->x)),
              min(getCreature()->getLevel()->getBounds().getKY() - 1, max(0, target->y)));
        }
      }
      else
        pickUpAction(false);
      break;
    case UserInputId::INVENTORY_ITEM:
      handleItems(action.get<InventoryItemInfo>().items(), action.get<InventoryItemInfo>().action()); break;
    case UserInputId::PICK_UP: pickUpAction(false); break;
    case UserInputId::EXT_PICK_UP: pickUpAction(true); break;
    case UserInputId::PICK_UP_ITEM: pickUpItemAction(action.get<int>()); break;
    case UserInputId::DROP: dropAction(false); break;
    case UserInputId::EXT_DROP: dropAction(true); break;
    case UserInputId::WAIT: getCreature()->wait().perform(getCreature()); break;
    case UserInputId::APPLY_ITEM: applyAction(); break; 
    case UserInputId::THROW: throwAction(); break;
    case UserInputId::THROW_DIR: throwAction(action.get<Vec2>()); break;
    case UserInputId::HIDE: hideAction(); break;
    case UserInputId::PAY_DEBT: payDebtAction(); break;
    case UserInputId::CHAT: chatAction(); break;
    case UserInputId::CONSUME: consumeAction(); break;
    case UserInputId::SHOW_HISTORY: showHistory(); break;
    case UserInputId::UNPOSSESS:
      if (unpossess())
        return;
      break;
    case UserInputId::CAST_SPELL: spellAction(); break;
    case UserInputId::DRAW_LEVEL_MAP: model->getView()->drawLevelMap(this); break;
    case UserInputId::EXIT: model->exitAction(); return;
    default: break;
  }
  if (getCreature()->isAffected(LastingEffect::SLEEP)) {
    onFellAsleep();
    return;
  }
  for (Vec2 dir : direction)
    if (travel) {
      if (Creature* other = getCreature()->getSafeSquare(dir)->getCreature())
        attackAction(other);
      else {
        vector<Vec2> squareDirs = getCreature()->getSquare()->getTravelDir();
        if (findElement(squareDirs, dir)) {
          travelDir = dir;
          lastLocation = getCreature()->getLevel()->getLocation(getCreature()->getPosition());
          travelling = true;
          travelAction();
        }
      }
    } else {
      moveAction(dir);
      break;
    }
  }
  if (!getCreature()->isDead()) {
    if (getCreature()->getTime() > currentTimePos.time) {
      previousTimePos = currentTimePos;
      currentTimePos = { getCreature()->getPosition(), getCreature()->getTime()};
    }
  }
}

void Player::showHistory() {
  model->getView()->presentList("Message history:", View::getListElem(messageHistory), true);
}

static string getForceMovementQuestion(const Square* square, const Creature* creature) {
  if (square->isBurning())
    return "Walk into the fire?";
  else if (square->canEnterEmpty(MovementTrait::SWIM))
    return "The water is very deep, are you sure?";
  else if (square->canEnterEmpty({MovementTrait::WALK}) &&
      creature->getMovementType().hasTrait(MovementTrait::SUNLIGHT_VULNERABLE))
    return "Walk into the sunlight?";
  else
    return "Walk into the " + square->getName() + "?";
}

void Player::moveAction(Vec2 dir) {
  if (auto action = getCreature()->move(dir)) {
    action.perform(getCreature());
  } else if (auto action = getCreature()->forceMove(dir)) {
    for (Square* square : getCreature()->getSquare(dir))
      if (square->canEnterEmpty(getCreature()) == getCreature()->getSquare()->canEnterEmpty(getCreature()) ||
          model->getView()->yesOrNoPrompt(getForceMovementQuestion(square, getCreature()), true))
        action.perform(getCreature());
  } else if (auto action = getCreature()->bumpInto(dir))
    action.perform(getCreature());
  else if (auto action = getCreature()->destroy(dir, Creature::BASH))
    action.perform(getCreature());
}

bool Player::isPlayer() const {
  return true;
}

void Player::privateMessage(const PlayerMessage& message) {
  if (message.getText().size() < 2)
    return;
  messageHistory.push_back(message.getText());
  if (!messages.empty() && messages.back().getFreshness() < 1)
    messages.clear();
  messages.emplace_back(message);
  if (message.getPriority() == PlayerMessage::CRITICAL)
    model->getView()->presentText("Important!", message.getText());
}

void Player::you(const string& param) {
  privateMessage("You " + param);
}

void Player::you(MsgType type, const string& param) {
  string msg;
  switch (type) {
    case MsgType::ARE: msg = "You are " + param; break;
    case MsgType::YOUR: msg = "Your " + param; break;
    case MsgType::FEEL: msg = "You feel " + param; break;
    case MsgType::FALL_ASLEEP: msg = "You fall asleep" + (param.size() > 0 ? " on the " + param : "."); break;
    case MsgType::WAKE_UP: msg = "You wake up."; break;
    case MsgType::FALL_APART: msg = "You fall apart."; break;
    case MsgType::FALL: msg = "You fall on the " + param; break;
    case MsgType::DIE: privateMessage("You die!!"); break;
    case MsgType::TELE_DISAPPEAR: msg = "You are standing somewhere else!"; break;
    case MsgType::BLEEDING_STOPS: msg = "Your bleeding stops."; break;
    case MsgType::DIE_OF: msg = "You die" + (param.empty() ? string(".") : " of " + param); break;
    case MsgType::MISS_ATTACK: msg = "You miss " + param; break;
    case MsgType::MISS_THROWN_ITEM: msg = param + " misses you"; break;
    case MsgType::MISS_THROWN_ITEM_PLURAL: msg = param + " miss you"; break;
    case MsgType::HIT_THROWN_ITEM: msg = param + " hits you"; break;
    case MsgType::HIT_THROWN_ITEM_PLURAL: msg = param + " hit you"; break;
    case MsgType::ITEM_CRASHES: msg = param + " crashes on you."; break;
    case MsgType::ITEM_CRASHES_PLURAL: msg = param + " crash on you."; break;
    case MsgType::GET_HIT_NODAMAGE: msg = "The " + param + " is harmless."; break;
    case MsgType::COLLAPSE: msg = "You collapse."; break;
    case MsgType::TRIGGER_TRAP: msg = "You trigger something."; break;
    case MsgType::DISARM_TRAP: msg = "You disarm the trap."; break;
    case MsgType::SWING_WEAPON: msg = "You swing your " + param; break;
    case MsgType::THRUST_WEAPON: msg = "You thrust your " + param; break;
    case MsgType::ATTACK_SURPRISE: msg = "You sneak attack " + param; break;
    case MsgType::KICK: msg = "You kick " + param; break;
    case MsgType::BITE: msg = "You bite " + param; break;
    case MsgType::PUNCH: msg = "You punch " + param; break;
    case MsgType::PANIC:
          msg = !getCreature()->isAffected(LastingEffect::HALLU) ? "You are suddenly very afraid" : "You freak out completely"; break;
    case MsgType::RAGE:
          msg = !getCreature()->isAffected(LastingEffect::HALLU) ?"You are suddenly very angry" : "This will be a very long trip."; break;
    case MsgType::CRAWL: msg = "You are crawling"; break;
    case MsgType::STAND_UP: msg = "You are back on your feet"; break;
    case MsgType::CAN_SEE_HIDING: msg = param + " can see you hiding"; break;
    case MsgType::TURN_INVISIBLE: msg = "You can see through yourself!"; break;
    case MsgType::TURN_VISIBLE: msg = "You are no longer invisible"; break;
    case MsgType::DROP_WEAPON: msg = "You drop your " + param; break;
    case MsgType::ENTER_PORTAL: msg = "You enter the portal"; break;
    case MsgType::HAPPENS_TO: msg = param + " you."; break;
    case MsgType::BURN: msg = "You burn in the " + param; break;
    case MsgType::DROWN: msg = "You drown in the " + param; break;
    case MsgType::SET_UP_TRAP: msg = "You set up the trap"; break;
    case MsgType::KILLED_BY: msg = "You are killed by " + param; break;
    case MsgType::TURN: msg = "You turn " + param; break;
    case MsgType::BECOME: msg = "You become " + param; break;
    case MsgType::BREAK_FREE: msg = "You break free from " + param; break;
    case MsgType::PRAY: msg = "You pray to " + param; break;
    case MsgType::COPULATE: msg = "You copulate " + param; break;
    case MsgType::CONSUME: msg = "You absorb " + param; break;
    case MsgType::GROW: msg = "You grow " + param; break;
    case MsgType::SACRIFICE: msg = "You make a sacrifice to " + param; break;
    case MsgType::HIT: msg = "You hit " + param; break;
    default: break;
  }
  privateMessage(msg);
}

const Level* Player::getLevel() const {
  return getCreature()->getLevel();
}

optional<Vec2> Player::getPosition(bool) const {
  return getCreature()->getPosition();
}

optional<CreatureView::MovementInfo> Player::getMovementInfo() const {
  if (previousTimePos.pos.x > -1)
    return MovementInfo({previousTimePos.pos, currentTimePos.pos, previousTimePos.time,
        getCreature()->getUniqueId()});
  else
    return none;
}

void Player::getViewIndex(Vec2 pos, ViewIndex& index) const {
  bool canSee = getCreature()->canSee(pos);
  const Square* square = getLevel()->getSafeSquare(pos);
  if (canSee)
    square->getViewIndex(index, getCreature()->getTribe());
  else
    index.setHiddenId(square->getViewObject().id());
  if (!canSee && getMemory().hasViewIndex(pos))
    index.mergeFromMemory(getMemory().getViewIndex(pos));
  if (const Creature* c = square->getCreature()) {
    if (getCreature()->canSee(c) || c == getCreature())
      index.insert(c->getViewObject());
    else if (contains(getCreature()->getUnknownAttacker(), c))
      index.insert(copyOf(ViewObject::unknownMonster()));
  }
}

void Player::onKilled(const Creature* attacker) {
  model->getView()->updateView(this, false);
  if (model->getView()->yesOrNoPrompt("Display message history?"))
    showHistory();
  model->gameOver(getCreature(), getCreature()->getKills().size(), "monsters", getCreature()->getPoints());
}

bool Player::unpossess() {
  return false;
}

void Player::onFellAsleep() {
}

const vector<Creature*> Player::getTeam() const {
  return {};
}

void Player::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.messageBuffer = messages;
  gameInfo.infoType = GameInfo::InfoType::PLAYER;
  Model::SunlightInfo sunlightInfo = getLevel()->getModel()->getSunlightInfo();
  gameInfo.sunlightInfo.description = sunlightInfo.getText();
  gameInfo.sunlightInfo.timeRemaining = sunlightInfo.timeRemaining;
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  info.playerName = getCreature()->getFirstName().get_value_or("");
  info.title = getCreature()->getName().bare();
  info.spellcaster = !getCreature()->getSpells().empty();
  info.adjectives = getCreature()->getMainAdjectives();
  Item* weapon = getCreature()->getWeapon();
  info.weaponName = weapon ? weapon->getName() : "";
  info.team.clear();
  for (const Creature* c : getTeam())
    info.team.push_back(c);
  const Location* location = getLevel()->getLocation(getCreature()->getPosition());
  info.levelName = location && location->hasName() 
    ? capitalFirst(location->getName()) : getLevel()->getName();
  typedef GameInfo::PlayerInfo::AttributeInfo::Id AttrId;
  info.attributes = {
    { "Attack",
      AttrId::ATT,
      getCreature()->getModifier(ModifierType::DAMAGE),
      getCreature()->isAffected(LastingEffect::RAGE) ? 1 : getCreature()->isAffected(LastingEffect::PANIC) ? -1 : 0,
      "Affects if and how much damage is dealt in combat."},
    { "Defense",
      AttrId::DEF,
      getCreature()->getModifier(ModifierType::DEFENSE),
      getCreature()->isAffected(LastingEffect::RAGE) ? -1 : (getCreature()->isAffected(LastingEffect::PANIC) 
          || getCreature()->isAffected(LastingEffect::MAGIC_SHIELD)) ? 1 : 0,
      "Affects if and how much damage is taken in combat."},
    { "Strength",
      AttrId::STR,
      getCreature()->getAttr(AttrType::STRENGTH),
      getCreature()->isAffected(LastingEffect::STR_BONUS),
      "Affects the values of attack, defense and carrying capacity."},
    { "Dexterity",
      AttrId::DEX,
      getCreature()->getAttr(AttrType::DEXTERITY),
      getCreature()->isAffected(LastingEffect::DEX_BONUS),
      "Affects the values of melee and ranged accuracy, and ranged damage."},
    { "Accuracy",
      AttrId::ACC,
      getCreature()->getModifier(ModifierType::ACCURACY),
      getCreature()->accuracyBonus(),
      "Defines the chance of a successful melee attack and dodging."},
    { "Speed",
      AttrId::SPD,
      getCreature()->getAttr(AttrType::SPEED),
      getCreature()->isAffected(LastingEffect::SPEED) ? 1 : getCreature()->isAffected(LastingEffect::SLOWED) ? -1 : 0,
      "Affects how much game time every action uses."},
/*    { "Level",
      ViewId::STAT_LVL,
      getCreature()->getExpLevel(), 0,
      "Describes general combat value of the creature."}*/
  };
  info.level = getCreature()->getExpLevel();
  info.skills = getCreature()->getSkillNames();
  gameInfo.time = getCreature()->getTime();
  info.effects.clear();
  for (string s : getCreature()->getBadAdjectives())
    info.effects.push_back({s, true});
  for (string s : getCreature()->getGoodAdjectives())
    info.effects.push_back({s, false});
  info.squareName = getCreature()->getSquare()->getName();
  info.lyingItems.clear();
  for (auto stack : getCreature()->stackItems(getCreature()->getPickUpOptions()))
    info.lyingItems.push_back(getItemInfo(stack));
  info.inventory.clear();
  map<ItemClass, vector<Item*> > typeGroups = groupBy<Item*, ItemClass>(
      getCreature()->getEquipment().getItems(), [](Item* const& item) { return item->getClass();});
  for (auto elem : typeDisplayOrder) 
    if (typeGroups[elem].size() > 0)
      info.inventory.push_back({getText(elem), getItemInfos(typeGroups[elem])});
}

ItemInfo Player::getItemInfo(const vector<Item*>& stack) const {
  return {
    stack[0]->getShortName(true, getCreature()->isBlind()),
    stack[0]->getNameAndModifiers(false, getCreature()->isBlind()),
    getCreature()->isBlind() ? "" : stack[0]->getDescription(),
    int(stack.size()),
    stack[0]->getViewObject(),
    transform2<UniqueEntity<Item>::Id>(stack, [](const Item* it) { return it->getUniqueId();}),
    getItemActions(stack[0]),
    getCreature()->getEquipment().isEquiped(stack[0])};
}

vector<ItemInfo> Player::getItemInfos(const vector<Item*>& items) const {
  map<string, vector<Item*> > stacks = groupBy<Item*, string>(items, 
      [this] (Item* const& item) { return getInventoryItemName(item, false); });
  vector<ItemInfo> ret;
  for (auto elem : stacks)
    ret.push_back(getItemInfo(elem.second));
  return ret;
}

vector<Vec2> Player::getVisibleEnemies() const {
  return transform2<Vec2>(getCreature()->getVisibleEnemies(), [](const Creature* c) { return c->getPosition(); });
}

double Player::getTime() const {
  return getCreature()->getTime();
}

