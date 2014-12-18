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
  CHECK_SERIAL;
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

void Player::onAlarmEvent(const Level* l, Vec2 pos) {
  if (l == getCreature()->getLevel()) {
    if (pos == getCreature()->getPosition())
      privateMessage("An alarm sounds near you.");
    else
      privateMessage("An alarm sounds in the " + 
          getCardinalName((pos - getCreature()->getPosition()).getBearing().getCardinalDir()));
  }
}

ControllerFactory Player::getFactory(Model *m, map<UniqueEntity<Level>::Id, MapMemory>* levelMemory) {
  return ControllerFactory([=](Creature* c) { return new Player(c, m, true, levelMemory);});
}

static string getSlotSuffix(EquipmentSlot slot) {
  switch (slot) {
    case EquipmentSlot::WEAPON: return "(weapon ready)";
    case EquipmentSlot::RANGED_WEAPON: return "(ranged weapon ready)";
    default: return "(being worn)";
  }
}

void Player::onBump(Creature*) {
  FAIL << "Shouldn't call onBump on a player";
}

void Player::getItemNames(vector<Item*> items, vector<View::ListElem>& names, vector<vector<Item*> >& groups,
    ItemPredicate predicate) {
  map<string, vector<Item*> > ret = groupBy<Item*, string>(items, 
      [this] (Item* const& item) { 
        if (getCreature()->getEquipment().isEquiped(item))
          return item->getNameAndModifiers(false, getCreature()->isBlind()) + " " 
              + getSlotSuffix(item->getEquipmentSlot());
        else
          return item->getNameAndModifiers(false, getCreature()->isBlind());});
  for (auto elem : ret) {
    if (elem.second.size() == 1)
      names.emplace_back(elem.first, predicate(elem.second[0]) ? View::NORMAL : View::INACTIVE);
    else
      names.emplace_back(toString<int>(elem.second.size()) + " " 
          + elem.second[0]->getNameAndModifiers(true, getCreature()->isBlind()),
          predicate(elem.second[0]) ? View::NORMAL : View::INACTIVE);
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
      getCreature()->applySquare().perform();
      return;
    }
  }
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(getCreature()->getPickUpOptions(), names, groups);
  if (names.empty())
    return;
  int index = 0;
  if (names.size() > 1) {
    Optional<int> res = model->getView()->chooseFromList("Choose an item to pick up:", names);
    if (!res)
      return;
    else
      index = *res;
  }
  int num = groups[index].size();
  if (num < 1)
    return;
  if (extended && num > 1) {
    Optional<int> res = model->getView()->getNumber("Pick up how many " + groups[index][0]->getName(true) + "?",
        1, num);
    if (!res)
      return;
    num = *res;
  }
  vector<Item*> pickUpItems = getPrefix(groups[index], num);
  tryToPerform(getCreature()->pickUp(pickUpItems));
}

void Player::pickUpItemAction(int numItem) {
  auto items = Item::stackItems(getCreature()->getPickUpOptions());
  CHECK(numItem < items.size());
  tryToPerform(getCreature()->pickUp(items[numItem].second));
}

void Player::tryToPerform(CreatureAction action) {
  if (action)
    action.perform();
  else
    getCreature()->playerMessage(action.getFailedReason());
}

void Player::itemsMessage() {
/*  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(getCreature()->getPickUpOptions(), names, groups);
  if (names.size() > 1)
    privateMessage(getCreature()->isBlind() ? "You feel here some items" : "You see here some items.");
  else if (names.size() == 1)
    privateMessage((getCreature()->isBlind() ? string("You feel here ") : ("You see here ")) + 
        (groups[0].size() == 1 ? "a " + groups[0][0]->getNameAndModifiers(false, getCreature()->isBlind()) :
            names[0].getText()));*/
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


vector<Item*> Player::chooseItem(const string& text, ItemPredicate predicate, Optional<UserInputId> exitAction) {
  map<ItemClass, vector<Item*> > typeGroups = groupBy<Item*, ItemClass>(
      getCreature()->getEquipment().getItems(), [](Item* const& item) { return item->getClass();});
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  for (auto elem : typeDisplayOrder) 
    if (typeGroups[elem].size() > 0) {
      names.push_back(View::ListElem(getText(elem), View::TITLE));
      getItemNames(typeGroups[elem], names, groups, predicate);
    }
  Optional<int> index = model->getView()->chooseFromList(text, names, 0, View::NORMAL_MENU, nullptr, exitAction);
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
    Optional<int> res = model->getView()->getNumber("Drop how many " + items[0]->getName(true, getCreature()->isBlind()) 
        + "?", 1, num);
    if (!res)
      return;
    num = *res;
  }
  tryToPerform(getCreature()->drop(getPrefix(items, num)));
}

void Player::onItemsAppeared(vector<Item*> items, const Creature* from) {
  if (!getCreature()->pickUp(items))
    return;
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(items, names, groups);
  CHECK(!names.empty());
  Optional<int> index = model->getView()->chooseFromList("Do you want to take this item?", names);
  if (!index) {
    return;
  }
  int num = groups[*index].size(); //groups[index].size() == 1 ? 1 : howMany(model->getView(), groups[index].size());
  if (num < 1)
    return;
  privateMessage("You take " + getCreature()->getPluralName(groups[*index][0], num));
  tryToPerform(getCreature()->pickUp(getPrefix(groups[*index], num), false));
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

void Player::throwAction(Optional<Vec2> dir) {
  vector<Item*> items = chooseItem("Choose an item to throw:", [this](const Item* item) {
      return !getCreature()->getEquipment().isEquiped(item);}, UserInputId::THROW);
  if (items.size() == 0)
    return;
  throwItem(items, dir);
}

void Player::throwItem(vector<Item*> items, Optional<Vec2> dir) {
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

void Player::equipmentAction() {
  if (!getCreature()->isHumanoid()) {
    privateMessage("You can't use any equipment.");
    return;
  }
  vector<EquipmentSlot> slots;
  for (auto slot : Equipment::slotTitles)
    slots.push_back(slot.first);
  int index = 0;
  getCreature()->startEquipChain();
  while (1) {
    vector<View::ListElem> list;
    vector<Item*> itemsChoice;
    vector<EquipmentSlot> slotsChoice;
    for (auto slot : slots) {
      list.push_back(View::ListElem(Equipment::slotTitles.at(slot), View::TITLE));
      vector<Item*> items = getCreature()->getEquipment().getItem(slot);
      for (Item* item : items) {
        list.push_back(item->getNameAndModifiers());
        itemsChoice.push_back(item);
        slotsChoice.push_back(EquipmentSlot::WEAPON); // anything
      }
      if (items.size() < getCreature()->getEquipment().getMaxItems(slot)) {
        list.push_back("[Equip]");
        slotsChoice.push_back(slot);
        itemsChoice.push_back(nullptr);
      }
    }
    model->getView()->updateView(this);
    Optional<int> newIndex = model->getView()->chooseFromList("Equipment", list, index, View::NORMAL_MENU, nullptr,
        UserInputId::EQUIPMENT);
    if (!newIndex) {
      getCreature()->finishEquipChain();
      return;
    }
    index = *newIndex;
    EquipmentSlot slot = slotsChoice[index];
    if (Item* item = itemsChoice[index]) {
      tryToPerform(getCreature()->unequip(item));
    } else {
      vector<Item*> items = chooseItem("Choose an item to equip:", [=](const Item* item) {
          return item->canEquip()
          && !getCreature()->getEquipment().isEquiped(item)
          && item->getEquipmentSlot() == slot;});
      if (items.size() == 0) {
        continue;
      }
      if (slot == EquipmentSlot::WEAPON && getCreature()->getAttr(AttrType::STRENGTH) < items[0]->getMinStrength()
          && !model->getView()->yesOrNoPrompt(items[0]->getTheName() + " is too heavy, and will incur an accuracy penaulty.\n Do you want to continue?")) {
        getCreature()->finishEquipChain();
        return;
      }
      tryToPerform(getCreature()->equip(items[0]));
    }
  }
}

void Player::grantIdentify(int numItems) {
  auto unidentFun = [this](const Item* item) { return item->canIdentify() && !item->isIdentified();};
  vector<Item*> unIded = getCreature()->getEquipment().getItems(unidentFun);
  if (unIded.empty()) {
    privateMessage("All your posessions are already identified");
    return;
  }
  if (numItems > unIded.size()) {
    privateMessage("You identify all your posessions");
    for (Item* it : unIded)
      it->identify();
  } else
  for (int i : Range(numItems)) {
    vector<Item*> items = chooseItem("Choose an item to identify:", unidentFun);
    if (items.size() == 0)
      return; 
    items[0]->identify();
    privateMessage("You identify " + items[0]->getTheName());
  }
}

void Player::displayInventory() {
  if (!getCreature()->isHumanoid()) {
    model->getView()->presentText("", "You can't use inventory.");
    return;
  }
  if (getCreature()->getEquipment().isEmpty()) {
    model->getView()->presentText("", "Your inventory is empty.");
    return;
  }
  vector<Item*> item = chooseItem("Inventory:", alwaysTrue<const Item*>(), UserInputId::SHOW_INVENTORY);
  if (item.size() == 0) {
    return;
  }
  vector<View::ListElem> options;
  if (getCreature()->equip(item[0])) {
    options.push_back("equip");
  }
  if (getCreature()->applyItem(item[0])) {
    options.push_back("apply");
  }
  if (getCreature()->unequip(item[0]))
    options.push_back("remove");
  else {
    options.push_back("throw");
    options.push_back("drop");
  }
  auto index = model->getView()->chooseFromList("What to do with "
      + getCreature()->getPluralName(item[0], item.size()) + "?", options);
  if (!index) {
    displayInventory();
    return;
  }
  if (options[*index].getText() == "drop")
    tryToPerform(getCreature()->drop(item));
  if (options[*index].getText() == "throw")
    throwItem(item);
  if (options[*index].getText() == "apply")
    applyItem(item);
  if (options[*index].getText() == "remove")
    tryToPerform(getCreature()->unequip(getOnlyElement(item)));
  if (options[*index].getText() == "equip")
    tryToPerform(getCreature()->equip(item[0]));
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
        model->getView()->updateView(this);
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
  itemsMessage();
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
  Optional<int> myIndex = findElement(squareDirs, -travelDir);
  CHECK(myIndex) << "Bad travel data in square";
  travelDir = squareDirs[(*myIndex + 1) % 2];
}

void Player::targetAction() {
  updateView = true;
  CHECK(target);
  if (getCreature()->getPosition() == *target || model->getView()->travelInterrupt()) {
    target = Nothing();
    return;
  }
  if (auto action = getCreature()->moveTowards(*target))
    action.perform();
  else
    target = Nothing();
  itemsMessage();
  if (interruptedByEnemy())
    target = Nothing();
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

void Player::chatAction(Optional<Vec2> dir) {
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
          getCreature()->isReady(spell) ? View::NORMAL : View::INACTIVE));
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
      model->getView()->updateView(this),
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
    getCreature()->attack(other, levels[*ind]).perform();;
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
    MEASURE(
        model->getView()->updateView(this),
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
/*    model->getView()->presentText("", "Every settlement that you find has a leader, and they may have quests for you."
        "\n \nYou can turn these messages off in the options (press F2).");*/
    displayGreeting = false;
    model->getView()->updateView(this);
  }
  for (const Creature* c : getCreature()->getVisibleEnemies()) {
    if (c->isSpecialMonster() && !contains(specialCreatures, c)) {
      privateMessage(PlayerMessage(c->getDescription(), PlayerMessage::CRITICAL));
      specialCreatures.push_back(c);
    }
  }
  if (travelling)
    travelAction();
  else if (target)
    targetAction();
  else {
    UserInput action = model->getView()->getAction();
    Debug() << "Action " << int(action.getId());
  vector<Vec2> direction;
  bool travel = false;
  if (action.getId() != UserInputId::IDLE) {
    if (action.getId() != UserInputId::REFRESH && action.getId() != UserInputId::BUTTON_RELEASE)
      retireMessages();
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
          target = action.get<Vec2>();
          target = Vec2(min(getCreature()->getLevel()->getBounds().getKX() - 1, max(0, target->x)),
              min(getCreature()->getLevel()->getBounds().getKY() - 1, max(0, target->y)));
          // Just in case
          if (!target->inRectangle(getCreature()->getLevel()->getBounds()))
            target = Nothing();
        }
        else
          pickUpAction(false);
      break;
    case UserInputId::SHOW_INVENTORY: displayInventory(); break;
    case UserInputId::PICK_UP: pickUpAction(false); break;
    case UserInputId::EXT_PICK_UP: pickUpAction(true); break;
    case UserInputId::PICK_UP_ITEM: pickUpItemAction(action.get<int>()); break;
    case UserInputId::DROP: dropAction(false); break;
    case UserInputId::EXT_DROP: dropAction(true); break;
    case UserInputId::WAIT: getCreature()->wait().perform(); break;
    case UserInputId::APPLY_ITEM: applyAction(); break; 
    case UserInputId::THROW: throwAction(); break;
    case UserInputId::THROW_DIR: throwAction(action.get<Vec2>()); break;
    case UserInputId::EQUIPMENT: equipmentAction(); break;
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
    case UserInputId::EXIT: model->exitAction(); break;
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
  if (!getCreature()->isDead())
    for (Vec2 pos : getCreature()->getLevel()->getVisibleTiles(getCreature())) {
      ViewIndex index;
      getViewIndex(pos, index);
      (*levelMemory)[getCreature()->getLevel()->getUniqueId()].update(pos, index);
    }
}

void Player::showHistory() {
  model->getView()->presentList("Message history:", View::getListElem(messageHistory), true);
}

void Player::moveAction(Vec2 dir) {
  if (auto action = getCreature()->move(dir)) {
    action.perform();
    itemsMessage();
  } else if (auto action = getCreature()->bumpInto(dir))
    action.perform();
  else if (auto action = getCreature()->destroy(dir, Creature::BASH))
    action.perform();
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

Optional<Vec2> Player::getPosition(bool) const {
  return getCreature()->getPosition();
}

void Player::getViewIndex(Vec2 pos, ViewIndex& index) const {
  const Square* square = getLevel()->getSafeSquare(pos);
  if (getCreature()->canSee(pos))
    square->getViewIndex(index, getCreature()->getTribe());
  else
    index.setHiddenId(square->getViewObject().id());
  if (!getCreature()->canSee(pos) && getMemory().hasViewIndex(pos))
    index.mergeFromMemory(getMemory().getViewIndex(pos));
  if (const Creature* c = square->getCreature()) {
    if (getCreature()->canSee(c) || c == getCreature())
      index.insert(c->getViewObject());
    else if (contains(getCreature()->getUnknownAttacker(), c))
      index.insert(copyOf(ViewObject::unknownMonster()));
  }
}

void Player::onKilled(const Creature* attacker) {
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

class PossessedController : public Player {
  public:
  PossessedController(Creature* c, Creature* _owner, Model* m, map<UniqueEntity<Level>::Id, MapMemory>* memory,
      bool ghost)
    : Player(c, m, false, memory), owner(_owner), isGhost(ghost) {}

  void onKilled(const Creature* attacker) override {
    if (attacker)
      owner->popController();
  }

  REGISTER_HANDLER(AttackEvent, Creature* victim, Creature* attacker) {
    if (!getCreature()->isDead() && victim == owner)
      unpossess();
  }

  bool unpossess() override {
    owner->popController();
    if (isGhost) {
      getCreature()->die();
      return false;
    } else
      return true;
  }

  void moveAction(Vec2 dir) override {
    if (!isGhost) {
      Player::moveAction(dir);
      return;
    }
    for (Square* square : getCreature()->getSquare(dir))
      if (Creature *c = square->getCreature()) {
        if (c == owner)
          owner->popController();
        else
          c->pushController(PController(new PossessedController(c, owner, model, levelMemory, false)));
        getCreature()->die();
        return;
      }
    Player::moveAction(dir);
  }

  void onFellAsleep() override {
    getCreature()->die();
    owner->popController();
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Player)
      & SVAR(owner)
      & SVAR(isGhost);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(PossessedController);

  private:
  Creature* SERIAL(owner);
  bool SERIAL(isGhost);
};

Controller* Player::getPossessedController(Creature* c) {
  getCreature()->pushController(PController(new DoNothingController(getCreature())));
  return new PossessedController(c, getCreature(), model, levelMemory, true);
}

static void grantGift(Creature* c, ItemType type, string deity, int num = 1) {
  c->playerMessage(deity + " grants you a gift.");
  c->takeItems(ItemFactory::fromId(type, num), nullptr);
}

static void applyEffect(Creature* c, EffectType effect, string msg) {
  c->playerMessage(msg);
  Effect::applyToCreature(c, effect, EffectStrength::STRONG);
}

void Player::onWorshipEvent(Creature* who, const Deity* to, WorshipType type) {
  if (who != getCreature())
    return;
  bool prayerAnswered = false;
  for (EpithetId epithet : randomPermutation(to->getEpithets())) {
    if (contains(usedEpithets, epithet))
      continue;
    bool noEffect = false;
    switch (epithet) {
      case EpithetId::DEATH: {
          PCreature death = CreatureFactory::fromId(CreatureId::DEATH, Tribe::get(TribeId::KILL_EVERYONE));
          for (Square* square : getCreature()->getSquares(Vec2::directions8(true)))
            if (square->canEnter(death.get())) {
              getCreature()->playerMessage("Death appears before you.");
              getCreature()->getLevel()->addCreature(square->getPosition(), std::move(death));
              break;
            }
          if (death)
            noEffect = true;
          break; }
      case EpithetId::WAR:
          grantGift(getCreature(), chooseRandom({
                ItemId::SPECIAL_SWORD,
                ItemId::SPECIAL_BATTLE_AXE,
                ItemId::SPECIAL_WAR_HAMMER}), to->getName()); break;
/*      case EpithetId::WISDOM: grantGift(c, 
          chooseRandom({ItemId::MUSHROOM_BOOK, ItemId::POTION_BOOK, ItemId::AMULET_BOOK}), name); break;*/
      case EpithetId::DESTRUCTION: applyEffect(getCreature(), EffectId::DESTROY_EQUIPMENT, ""); break;
      case EpithetId::SECRETS:
          grantGift(getCreature(), {ItemId::POTION, EffectType(EffectId::LASTING, LastingEffect::INVISIBLE)},
              to->getName());
          break;
      case EpithetId::LIGHTNING:
          getCreature()->bleed(0.9);
          getCreature()->you(MsgType::ARE, "struck by a lightning bolt!");
          break;
      case EpithetId::FEAR:
          applyEffect(getCreature(), EffectType(EffectId::LASTING, LastingEffect::PANIC),
              to->getName() + " puts fear in your heart"); break;
      case EpithetId::MIND: 
          if (Random.roll(2))
            applyEffect(getCreature(), EffectType(EffectId::LASTING, LastingEffect::RAGE),
                to->getName() + " fills your head with anger");
          else
            applyEffect(getCreature(), EffectType(EffectId::LASTING, LastingEffect::HALLU), "");
          break;
      case EpithetId::CHANGE:
          if (Random.roll(2) && getCreature()->getWeapon()) {
            PCreature snake = CreatureFactory::fromId(CreatureId::SNAKE, Tribe::get(TribeId::PEST));
            for (Square* square : getCreature()->getSquares(Vec2::directions8(true)))
              if (square->canEnter(snake.get())) {
                getCreature()->getLevel()->addCreature(square->getPosition(), std::move(snake));
                getCreature()->steal({getCreature()->getEquipment().getItem(EquipmentSlot::WEAPON)});
                getCreature()->playerMessage("Ouch!");
                getCreature()->you(MsgType::YOUR, "weapon turns into a snake!");
                break;
              }
            if (!snake)
              break;
          }
          for (Item* it : randomPermutation(getCreature()->getEquipment().getItems())) {
            if (it->getClass() == ItemClass::POTION) {
              getCreature()->playerMessage("Your " + it->getName() + " changes color!");
              getCreature()->steal({it});
              getCreature()->take(ItemFactory::potions().random());
              break;
            }
            if (it->getClass() == ItemClass::SCROLL) {
              getCreature()->playerMessage("Your " + it->getName() + " changes label!");
              getCreature()->steal({it});
              getCreature()->take(ItemFactory::scrolls().random());
              break;
            }
            if (it->getClass() == ItemClass::AMULET) {
              getCreature()->playerMessage("Your " + it->getName() + " changes shape!");
              getCreature()->steal({it});
              getCreature()->take(ItemFactory::amulets().random());
              break;
            }
          }
          break;
      case EpithetId::HEALTH:
          if (getCreature()->getHealth() < 1 || getCreature()->lostOrInjuredBodyParts())
            applyEffect(getCreature(), EffectId::HEAL, "You feel a healing power overcoming you");
          else {
            if (Random.roll(4))
              grantGift(getCreature(), ItemId::HEALING_AMULET, to->getName());
            else
              grantGift(getCreature(), {ItemId::POTION, EffectId::HEAL}, to->getName(),
                  Random.get(1, 4));
          }
          break;
      case EpithetId::NATURE: grantGift(getCreature(), ItemId::FRIENDLY_ANIMALS_AMULET, to->getName()); break;
//      case EpithetId::LOVE: grantGift(c, ItemId::PANIC_MUSHROOM, name); break;
      case EpithetId::WEALTH:
        grantGift(getCreature(), ItemId::GOLD_PIECE, to->getName(), Random.get(100, 200)); break;
      case EpithetId::DEFENSE: grantGift(getCreature(), ItemId::DEFENSE_AMULET, to->getName()); break;
      case EpithetId::DARKNESS: applyEffect(getCreature(), EffectType(EffectId::LASTING, LastingEffect::BLIND), ""); break;
      case EpithetId::CRAFTS: applyEffect(getCreature(),
          chooseRandom({EffectId::ENHANCE_ARMOR, EffectId::ENHANCE_WEAPON}), ""); break;
      default: noEffect = true;
    }
    usedEpithets.push_back(epithet);
    if (!noEffect) {
      prayerAnswered = true;
      break;
    }
  }
  if (!prayerAnswered)
    getCreature()->playerMessage("Your prayer is not answered.");
}

void Player::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.messageBuffer = messages;
  gameInfo.infoType = GameInfo::InfoType::PLAYER;
  Model::SunlightInfo sunlightInfo = getLevel()->getModel()->getSunlightInfo();
  gameInfo.sunlightInfo.description = sunlightInfo.getText();
  gameInfo.sunlightInfo.timeRemaining = sunlightInfo.timeRemaining;
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  info.playerName = getCreature()->getFirstName().getOr("");
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
  info.attributes = {
    {"level",
      getCreature()->getExpLevel(), 0,
      "Describes general combat value of the getCreature()."},
    {"attack",
      getCreature()->getModifier(ModifierType::DAMAGE),
      getCreature()->isAffected(LastingEffect::RAGE) ? 1 : getCreature()->isAffected(LastingEffect::PANIC) ? -1 : 0,
      "Affects if and how much damage is dealt in combat."},
    {"defense",
      getCreature()->getModifier(ModifierType::DEFENSE),
      getCreature()->isAffected(LastingEffect::RAGE) ? -1 : (getCreature()->isAffected(LastingEffect::PANIC) 
          || getCreature()->isAffected(LastingEffect::MAGIC_SHIELD)) ? 1 : 0,
      "Affects if and how much damage is taken in combat."},
    {"accuracy",
      getCreature()->getModifier(ModifierType::ACCURACY),
      getCreature()->accuracyBonus(),
      "Defines the chance of a successful melee attack and dodging."},
    {"strength",
      getCreature()->getAttr(AttrType::STRENGTH),
      getCreature()->isAffected(LastingEffect::STR_BONUS),
      "Affects the values of attack, defense and carrying capacity."},
    {"dexterity",
      getCreature()->getAttr(AttrType::DEXTERITY),
      getCreature()->isAffected(LastingEffect::DEX_BONUS),
      "Affects the values of melee and ranged accuracy, and ranged damage."},
    {"speed",
      getCreature()->getAttr(AttrType::SPEED),
      getCreature()->isAffected(LastingEffect::SPEED) ? 1 : getCreature()->isAffected(LastingEffect::SLOWED) ? -1 : 0,
      "Affects how much time every action takes."}};
  info.skills = getCreature()->getSkillNames();
  info.attributes.push_back({"gold", int(getCreature()->getGold(100000000).size()), 0, ""});
  gameInfo.time = getCreature()->getTime();
 /* info.elfStanding = Tribe::get(TribeId::ELVEN)->getStanding(this);
  info.dwarfStanding = Tribe::get(TribeId::DWARVEN)->getStanding(this);
  info.orcStanding = Tribe::get(TribeId::ORC)->getStanding(this);*/
  info.effects.clear();
  for (string s : getCreature()->getAdjectives())
    info.effects.push_back({s, true});
  info.squareName = getCreature()->getSquare()->getName();
  info.lyingItems.clear();
  for (auto stack : Item::stackItems(getCreature()->getPickUpOptions()))
    if (stack.second.size() == 1)
      info.lyingItems.push_back({stack.first, stack.second[0]->getViewObject()});
    else info.lyingItems.push_back({toString(stack.second.size()) + " "
        + stack.second[0]->getName(true), stack.second[0]->getViewObject()});

}

vector<const Creature*> Player::getVisibleEnemies() const {
  return getCreature()->getVisibleEnemies();
}

template <class Archive>
void Player::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, PossessedController);
}

REGISTER_TYPES(Player);

