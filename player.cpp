#include "stdafx.h"

#include "player.h"
#include "location.h"
#include "level.h"
#include "message_buffer.h"
#include "ranged_weapon.h"
#include "name_generator.h"
#include "model.h"
#include "options.h"

template <class Archive> 
void Player::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Controller)
    & SUBCLASS(EventListener) 
    & BOOST_SERIALIZATION_NVP(creature)
    & BOOST_SERIALIZATION_NVP(travelling)
    & BOOST_SERIALIZATION_NVP(travelDir)
    & BOOST_SERIALIZATION_NVP(target)
    & BOOST_SERIALIZATION_NVP(lastLocation)
    & BOOST_SERIALIZATION_NVP(specialCreatures)
    & BOOST_SERIALIZATION_NVP(displayGreeting)
    & BOOST_SERIALIZATION_NVP(levelMemory)
    & BOOST_SERIALIZATION_NVP(model)
    & BOOST_SERIALIZATION_NVP(displayTravelInfo);
}

SERIALIZABLE(Player);

Player::Player(Creature* c, View* v, Model* m, bool greet, map<const Level*, MapMemory>* memory) :
    creature(c), displayGreeting(greet), levelMemory(memory), model(m) {
}

Player::~Player() {
}

const Level* Player::getListenerLevel() const {
  return creature->getLevel();
}

void Player::onThrowEvent(const Creature* thrower, const Item* item, const vector<Vec2>& trajectory) {
  for (Vec2 v : trajectory)
    if (creature->canSee(v)) {
      model->getView()->animateObject(trajectory, item->getViewObject());
      return;
    }
}

void Player::learnLocation(const Location* loc) {
  for (Vec2 v : loc->getBounds())
    remember(v, creature->getLevel()->getSquare(v)->getViewObject());
}

void Player::onExplosionEvent(const Level* level, Vec2 pos) {
  if (creature->canSee(pos))
    model->getView()->animation(pos, AnimationId::EXPLOSION);
  else
    creature->privateMessage("BOOM!");
}

ControllerFactory Player::getFactory(View* f, Model *m, map<const Level*, MapMemory>* levelMemory) {
  return ControllerFactory([=](Creature* c) { return new Player(c, f, m, true, levelMemory);});
}

map<EquipmentSlot, string> slotSuffixes = {
    {EquipmentSlot::WEAPON, "(weapon ready)"},
    {EquipmentSlot::RANGED_WEAPON, "(ranged weapon ready)"},
    {EquipmentSlot::BODY_ARMOR, "(being worn)"},
    {EquipmentSlot::HELMET, "(being worn)"},
    {EquipmentSlot::BOOTS, "(being worn)"},
    {EquipmentSlot::AMULET, "(being worn)"}};

map<EquipmentSlot, string> slotTitles = {
    {EquipmentSlot::WEAPON, "Weapon"},
    {EquipmentSlot::RANGED_WEAPON, "Ranged weapon"},
    {EquipmentSlot::HELMET, "Helmet"},
    {EquipmentSlot::BODY_ARMOR, "Body armor"},
    {EquipmentSlot::BOOTS, "Boots"},
    {EquipmentSlot::AMULET, "Amulet"}};

void Player::onBump(Creature*) {
  FAIL << "Shouldn't call onBump on a player";
}

void Player::getItemNames(vector<Item*> items, vector<View::ListElem>& names, vector<vector<Item*> >& groups,
    ItemPredicate predicate) {
  map<string, vector<Item*> > ret = groupBy<Item*, string>(items, 
      [this] (Item* const& item) { 
        if (creature->getEquipment().isEquiped(item))
          return item->getNameAndModifiers(false, creature->isBlind()) + " " 
              + slotSuffixes[creature->getEquipment().getSlot(item)];
        else
          return item->getNameAndModifiers(false, creature->isBlind());});
  for (auto elem : ret) {
    if (elem.second.size() == 1)
      names.emplace_back(elem.first, predicate(elem.second[0]) ? View::NORMAL : View::INACTIVE);
    else
      names.emplace_back(convertToString<int>(elem.second.size()) + " " 
          + elem.second[0]->getNameAndModifiers(true, creature->isBlind()),
          predicate(elem.second[0]) ? View::NORMAL : View::INACTIVE);
    groups.push_back(elem.second);
  }
}

string Player::getPluralName(Item* item, int num) {
  if (num == 1)
    return item->getTheName(false, creature->isBlind());
  else
    return convertToString(num) + " " + item->getTheName(true, creature->isBlind());
}

static string getSquareQuestion(SquareApplyType type, string name) {
  switch (type) {
    case SquareApplyType::DESCEND: return "Descend the " + name;
    case SquareApplyType::ASCEND: return "Ascend the " + name;
    case SquareApplyType::USE_CHEST: return "Open the " + name;
    case SquareApplyType::DRINK: return "Drink from the " + name;
    case SquareApplyType::PRAY: return "Pray at the " + name;
    case SquareApplyType::SLEEP: return "Go to sleep on the " + name;
    default: FAIL << "Unhandled";
  }
  return "";
}

static bool canUseSquare(SquareApplyType type) {
  return !contains({SquareApplyType::TRAIN, SquareApplyType::WORKSHOP}, type);
}

void Player::pickUpAction(bool extended) {
  auto items = creature->getPickUpOptions();
  const Square* square = creature->getConstSquare();
  if (square->getApplyType(creature) && canUseSquare(*square->getApplyType(creature)) &&
      (items.empty() ||
       model->getView()->yesOrNoPrompt(getSquareQuestion(*square->getApplyType(creature), square->getName())))) {
    creature->applySquare();
    return;
  }
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(creature->getPickUpOptions(), names, groups);
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
    Optional<int> res = model->getView()->getNumber("Pick up how many " + groups[index][0]->getName(true) + "?", num);
    if (!res)
      return;
    num = *res;
  }
  vector<Item*> pickUpItems = getPrefix(groups[index], 0, num);
  if (creature->canPickUp(pickUpItems)) {
    creature->privateMessage("You pick up " + getPluralName(groups[index][0], num));
    creature->pickUp(pickUpItems);
  }
}

void Player::itemsMessage() {
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(creature->getPickUpOptions(), names, groups);
  if (names.size() > 1)
    privateMessage(creature->isBlind() ? "You feel here some items" : "You see here some items.");
  else if (names.size() == 1)
    privateMessage((creature->isBlind() ? string("You feel here ") : ("You see here ")) + 
        (groups[0].size() == 1 ? "a " + groups[0][0]->getNameAndModifiers(false, creature->isBlind()) :
            names[0].getText()));
}

ItemType typeDisplayOrder[] { ItemType::WEAPON, ItemType::RANGED_WEAPON, ItemType::AMMO, ItemType::ARMOR, ItemType::POTION, ItemType::SCROLL, ItemType::FOOD, ItemType::BOOK, ItemType::AMULET, ItemType::TOOL, ItemType::CORPSE, ItemType::OTHER, ItemType::GOLD };

static string getText(ItemType type) {
  switch (type) {
    case ItemType::WEAPON: return "Weapons";
    case ItemType::RANGED_WEAPON: return "Ranged weapons";
    case ItemType::AMMO: return "Projectiles";
    case ItemType::AMULET: return "Amulets";
    case ItemType::ARMOR: return "Armor";
    case ItemType::SCROLL: return "Scrolls";
    case ItemType::POTION: return "Potions";
    case ItemType::FOOD: return "Comestibles";
    case ItemType::BOOK: return "Books";
    case ItemType::TOOL: return "Tools";
    case ItemType::CORPSE: return "Corpses";
    case ItemType::OTHER: return "Other";
    case ItemType::GOLD: return "Gold";
  }
  FAIL << int(type);
  return "";
}


vector<Item*> Player::chooseItem(const string& text, ItemPredicate predicate, Optional<ActionId> exitAction) {
  map<ItemType, vector<Item*> > typeGroups = groupBy<Item*, ItemType>(
      creature->getEquipment().getItems(), [](Item* const& item) { return item->getType();});
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  for (auto elem : typeDisplayOrder) 
    if (typeGroups[elem].size() > 0) {
      names.push_back(View::ListElem(getText(elem), View::TITLE));
      getItemNames(typeGroups[elem], names, groups, predicate);
    }
  Optional<int> index = model->getView()->chooseFromList(text, names, 0, exitAction);
  if (index)
    return groups[*index];
  return vector<Item*>();
}

void Player::dropAction(bool extended) {
  vector<Item*> items = chooseItem("Choose an item to drop:", [this](const Item* item) {
      return !creature->getEquipment().isEquiped(item) || item->getType() == ItemType::WEAPON;}, ActionId::DROP);
  int num = items.size();
  if (num < 1)
    return;
  if (extended && num > 1) {
    Optional<int> res = model->getView()->getNumber("Drop how many " + items[0]->getName(true, creature->isBlind()) + "?", num);
    if (!res)
      return;
    num = *res;
  }
  creature->privateMessage("You drop " + getPluralName(items[0], num));
  creature->drop(getPrefix(items, 0, num));
}

void Player::onItemsAppeared(vector<Item*> items, const Creature* from) {
  if (!creature->canPickUp(items))
    return;
  vector<View::ListElem> names;
  vector<vector<Item*> > groups;
  getItemNames(items, names, groups);
  CHECK(!names.empty());
  Optional<int> index = model->getView()->chooseFromList("Do you want to take it?", names);
  if (!index) {
    return;
  }
  int num = groups[*index].size(); //groups[index].size() == 1 ? 1 : howMany(model->getView(), groups[index].size());
  if (num < 1)
    return;
  creature->privateMessage("You take " + getPluralName(groups[*index][0], num));
  creature->pickUp(getPrefix(groups[*index], 0, num), false);
}

void Player::applyAction() {
  if (!creature->isHumanoid())
    return;
  if (creature->numGoodArms() == 0) {
    privateMessage("You don't have hands!");
    return;
  }
  vector<Item*> items = chooseItem("Choose an item to apply:", [this](const Item* item) {
      return creature->canApplyItem(item);}, ActionId::APPLY_ITEM);
  if (items.size() == 0)
    return;
  applyItem(items);
}

void Player::applyItem(vector<Item*> items) {
  if (creature->isBlind() && contains({ItemType::SCROLL, ItemType::BOOK}, items[0]->getType())) {
    privateMessage("You can't read while blind!");
    return;
  }
  if (items[0]->getApplyTime() > 1) {
    for (const Creature* c : creature->getVisibleEnemies())
      if ((c->getPosition() - creature->getPosition()).length8() < 3) { 
        if (!model->getView()->yesOrNoPrompt("Applying " + items[0]->getAName() + " takes " + 
            convertToString(items[0]->getApplyTime()) + " turns. Are you sure you want to continue?"))
          return;
        else
          break;
      }
  }
  if (creature->canApplyItem(items[0])) {
    privateMessage("You " + items[0]->getApplyMsgFirstPerson());
    creature->applyItem(items[0]);
  }
}

void Player::throwAction(Optional<Vec2> dir) {
  vector<Item*> items = chooseItem("Choose an item to throw:", [this](const Item* item) {
      return !creature->getEquipment().isEquiped(item);}, ActionId::THROW);
  if (items.size() == 0)
    return;
  throwItem(items, dir);
}

void Player::throwItem(vector<Item*> items, Optional<Vec2> dir) {
  if (!dir) {
    auto cDir = model->getView()->chooseDirection("Which direction do you want to throw?");
    if (!cDir)
      return;
    dir = *cDir;
  }
  if (creature->canThrowItem(items[0])) {
    creature->privateMessage("You throw " + items[0]->getAName(false, creature->isBlind()));
    creature->throwItem(items[0], *dir);
  }
}

void Player::equipmentAction() {
  if (!creature->isHumanoid()) {
    creature->privateMessage("You can't use any equipment.");
    return;
  }
  vector<EquipmentSlot> slots;
  for (auto slot : slotTitles)
    slots.push_back(slot.first);
  int index = 0;
  creature->startEquipChain();
  while (1) {
    vector<View::ListElem> list;
    for (auto slot : slots) {
      list.push_back(View::ListElem(slotTitles.at(slot), View::TITLE));
      Item* item = creature->getEquipment().getItem(slot);
      if (item)
        list.push_back(item->getNameAndModifiers());
      else
        list.push_back("[Nothing]");
    }
    model->getView()->updateView(creature);
    Optional<int> newIndex = model->getView()->chooseFromList("Equipment", list, index, ActionId::EQUIPMENT);
    if (!newIndex) {
      creature->finishEquipChain();
      return;
    }
    index = *newIndex;
    EquipmentSlot slot = slots[index];
    if (Item* item = creature->getEquipment().getItem(slot)) {
      if (creature->canUnequip(item))
        creature->unequip(item);
    } else {
      vector<Item*> items = chooseItem("Choose an item to equip:", [=](const Item* item) {
          return item->canEquip()
          && !creature->getEquipment().isEquiped(item)
          && item->getEquipmentSlot() == slot;});
      if (items.size() == 0) {
        continue;
 //       finishEquipChain();
 //       return;
      }
      //  messageBuffer.addMessage("You equip " + items[0]->getTheName());
      if (creature->canEquip(items[0])) {
        creature->equip(items[0]);
      }
    }
  }
}

void Player::grantIdentify(int numItems) {
  auto unidentFun = [this](const Item* item) { return item->canIdentify() && !item->isIdentified();};
  vector<Item*> unIded = creature->getEquipment().getItems(unidentFun);
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
  if (!creature->isHumanoid()) {
    model->getView()->presentText("", "You can't use inventory.");
    return;
  }
  if (creature->getEquipment().isEmpty()) {
    model->getView()->presentText("", "Your inventory is empty.");
    return;
  }
  vector<Item*> item = chooseItem("Inventory:", alwaysTrue<const Item*>(), ActionId::SHOW_INVENTORY);
  if (item.size() == 0) {
    return;
  }
  vector<View::ListElem> options;
  if (creature->canEquip(item[0])) {
    options.push_back("equip");
  }
  if (creature->canApplyItem(item[0])) {
    options.push_back("apply");
  }
  if (creature->canUnequip(item[0]))
    options.push_back("remove");
  else {
    options.push_back("throw");
    options.push_back("drop");
  }
  auto index = model->getView()->chooseFromList("What to do with " + getPluralName(item[0], item.size()) + "?", options);
  if (!index) {
    displayInventory();
    return;
  }
  if (options[*index].getText() == "drop") {
    creature->privateMessage("You drop " + getPluralName(item[0], item.size()));
    creature->drop(item);
  }
  if (options[*index].getText() == "throw") {
    throwItem(item);
  }
  if (options[*index].getText() == "apply") {
    applyItem(item);
  }
  if (options[*index].getText() == "remove") {
    creature->privateMessage("You remove " + getPluralName(item[0], item.size()));
    creature->unequip(getOnlyElement(item));
  }
  if (options[*index].getText() == "equip") {
    creature->privateMessage("You equip " + getPluralName(item[0], item.size()));
    creature->equip(item[0]);
  }
}

void Player::hideAction() {
  if (creature->canHide()) {
    privateMessage("You hide behind the " + creature->getConstSquare()->getName());
    creature->hide();
  } else {
    if (!creature->hasSkill(Skill::ambush))
      privateMessage("You don't have this skill.");
    else
      privateMessage("You can't hide here.");
  }
}

bool Player::interruptedByEnemy() {
  vector<const Creature*> enemies = creature->getVisibleEnemies();
  vector<string> ignoreCreatures { "a boar" ,"a deer", "a fox", "a vulture", "a rat", "a jackal", "a boulder" };
  if (enemies.size() > 0) {
    for (const Creature* c : enemies)
      if (!contains(ignoreCreatures, c->getAName())) {
        model->getView()->refreshView(creature);
        privateMessage("You notice " + c->getAName());
        return true;
      }
  }
  return false;
}

void Player::travelAction() {
  if (!creature->canMove(travelDir) || model->getView()->travelInterrupt() || interruptedByEnemy()) {
    travelling = false;
    return;
  }
  creature->move(travelDir);
  itemsMessage();
  const Location* currentLocation = creature->getLevel()->getLocation(creature->getPosition());
  if (lastLocation != currentLocation && currentLocation != nullptr && currentLocation->hasName()) {
    privateMessage("You arrive at " + addAParticle(currentLocation->getName()));
    travelling = false;
    return;
  }
  vector<Vec2> squareDirs = creature->getConstSquare()->getTravelDir();
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
  CHECK(target);
  if (creature->getPosition() == *target || model->getView()->travelInterrupt()) {
    target = Nothing();
    return;
  }
  Optional<Vec2> move = creature->getMoveTowards(*target);
  if (move)
    creature->move(*move);
  else
    target = Nothing();
  itemsMessage();
  if (interruptedByEnemy())
    target = Nothing();
}

void Player::payDebtAction() {
  for (Vec2 v : Vec2::directions8())
    if (const Creature* c = creature->getConstSquare(v)->getCreature()) {
      if (int debt = c->getDebt(creature)) {
        vector<Item*> gold = creature->getGold(debt);
        if (gold.size() < debt) {
          privateMessage("You don't have enough gold to pay.");
        } else if (model->getView()->yesOrNoPrompt("Buy items for " + convertToString(debt) + " zorkmids?")) {
          privateMessage("You pay " + c->getName() + " " + convertToString(debt) + " zorkmids.");
          creature->give(c, gold);
        }
      } else {
        Debug() << "No debt " << c->getName();
      }
    }
}

void Player::chatAction(Optional<Vec2> dir) {
  vector<const Creature*> creatures;
  for (Vec2 v : Vec2::directions8())
    if (const Creature* c = creature->getConstSquare(v)->getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && !dir) {
    privateMessage("You chat with " + creatures[0]->getTheName());
    creature->chatTo(creatures[0]->getPosition() - creature->getPosition());
  } else
  if (creatures.size() > 1 || dir) {
    if (!dir)
      dir = model->getView()->chooseDirection("Which direction?");
    if (!dir)
      return;
    if (const Creature* c = creature->getConstSquare(*dir)->getCreature()) {
      privateMessage("You chat with " + c->getTheName());
      creature->chatTo(*dir);
    }
  }
}

void Player::fireAction(Vec2 dir) {
  if (creature->canFire(dir))
    creature->fire(dir);
}

void Player::spellAction() {
  vector<View::ListElem> list;
  auto spells = creature->getSpells();
  for (int i : All(spells))
    list.push_back(View::ListElem(spells[i].name + " " + (!creature->canCastSpell(i) ? "(ready in " +
          convertToString(int(spells[i].ready - creature->getTime() + 0.9999)) + " turns)" : ""),
          creature->canCastSpell(i) ? View::NORMAL : View::INACTIVE));
  auto index = model->getView()->chooseFromList("Cast a spell:", list);
  if (!index)
    return;
  creature->privateMessage("You cast " + spells[*index].name);
  creature->castSpell(*index);
}

const MapMemory& Player::getMemory(const Level* l) const {
  if (l == nullptr) 
    l = creature->getLevel();
  return (*levelMemory)[l];
}

void Player::remember(Vec2 pos, const ViewObject& object) {
  (*levelMemory)[creature->getLevel()].addObject(pos, object);
}

void Player::sleeping() {
  if (creature->isHallucinating())
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  MEASURE(
      model->getView()->refreshView(creature),
      "level render time");
}

void Player::makeMove() {
  vector<Vec2> squareDirs = creature->getConstSquare()->getTravelDir();
  const vector<Creature*>& creatures = creature->getLevel()->getAllCreatures();
  if (creature->isHallucinating())
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  MEASURE(
      model->getView()->refreshView(creature),
      "level render time");
  if (Options::getValue(OptionId::HINTS) && displayTravelInfo && creature->getConstSquare()->getName() == "road") {
    model->getView()->presentText("", "Use ctrl + arrows to travel quickly on roads and corridors.");
    displayTravelInfo = false;
  }
  static bool greeting = false;
  if (Options::getValue(OptionId::HINTS) && displayGreeting) {
    CHECK(creature->getFirstName());
    model->getView()->presentText("", "Dear " + *creature->getFirstName() + ",\n \n \tIf you are reading this letter, then you have arrived in the valley of " + NameGenerator::worldNames.getNext() + ". There is a band of dwarves dwelling in caves under a mountain. Find them, talk to them, they will help you. Let your sword guide you.\n \n \nYours, " + NameGenerator::firstNames.getNext() + "\n \nPS.: Beware the goblins!");
    model->getView()->presentText("", "Every settlement that you find has a leader, and they may have quests for you."
        "\n \nYou can turn these messages off in the options (press F2).");
    displayGreeting = false;
  }
  for (const Creature* c : creature->getVisibleEnemies()) {
    if (c->isSpecialMonster() && !contains(specialCreatures, c)) {
      privateMessage(MessageBuffer::important(c->getDescription()));
      specialCreatures.push_back(c);
    }
  }
  if (travelling)
    travelAction();
  else if (target)
    targetAction();
  else {
    Action action = model->getView()->getAction();
  vector<Vec2> direction;
  bool travel = false;
  switch (action.getId()) {
    case ActionId::FIRE: fireAction(action.getDirection()); break;
    case ActionId::TRAVEL: travel = true;
    case ActionId::MOVE: direction.push_back(action.getDirection()); break;
    case ActionId::MOVE_TO: if (action.getDirection().dist8(creature->getPosition()) == 1) {
                              Vec2 dir = action.getDirection() - creature->getPosition();
                              if (const Creature* c = creature->getConstSquare(dir)->getCreature()) {
                                if (!creature->isEnemy(c)) {
                                  chatAction(dir);
                                  break;
                                }
                              }
                              direction.push_back(dir);
                            } else
                            if (action.getDirection() != creature->getPosition()) {
                              target = action.getDirection();
                              target = Vec2(min(creature->getLevel()->getBounds().getKX() - 1, max(0, target->x)),
                                  min(creature->getLevel()->getBounds().getKY() - 1, max(0, target->y)));
                              // Just in case
                              if (!target->inRectangle(creature->getLevel()->getBounds()))
                                target = Nothing();
                            }
                            else
                              pickUpAction(false);
                            break;
    case ActionId::SHOW_INVENTORY: displayInventory(); break;
    case ActionId::PICK_UP: pickUpAction(false); break;
    case ActionId::EXT_PICK_UP: pickUpAction(true); break;
    case ActionId::DROP: dropAction(false); break;
    case ActionId::EXT_DROP: dropAction(true); break;
    case ActionId::WAIT: creature->wait(); break;
    case ActionId::APPLY_ITEM: applyAction(); break; 
    case ActionId::THROW: throwAction(); break;
    case ActionId::THROW_DIR: throwAction(action.getDirection()); break;
    case ActionId::EQUIPMENT: equipmentAction(); break;
    case ActionId::HIDE: hideAction(); break;
    case ActionId::PAY_DEBT: payDebtAction(); break;
    case ActionId::CHAT: chatAction(); break;
    case ActionId::SHOW_HISTORY: messageBuffer.showHistory(); break;
    case ActionId::UNPOSSESS: if (creature->canPopController()) {
                                creature->popController();
                                return;
                              } break;
    case ActionId::CAST_SPELL: spellAction(); break;
    case ActionId::DRAW_LEVEL_MAP: model->getView()->drawLevelMap(creature->getLevel(), creature); break;
    case ActionId::EXIT: model->exitAction(); break;
    case ActionId::IDLE: break;
  }
  if (creature->isSleeping() && creature->canPopController()) {
    if (model->getView()->yesOrNoPrompt("You fell asleep. Do you want to leave your minion?"))
      creature->popController();
    return;
  }
  for (Vec2 dir : direction)
    if (travel) {
      vector<Vec2> squareDirs = creature->getConstSquare()->getTravelDir();
      if (findElement(squareDirs, dir)) {
        travelDir = dir;
        lastLocation = creature->getLevel()->getLocation(creature->getPosition());
        travelling = true;
        travelAction();
      }
    } else
    if (creature->canMove(dir)) {
      creature->move(dir);
      itemsMessage();
      break;
    } else {
      const Creature *c = creature->getConstSquare(dir)->getCreature();
      if (creature->canBumpInto(dir)) {
        creature->bumpInto(dir);
        break;
      } else 
      if (creature->canDestroy(dir)) {
        privateMessage("You bash the " + creature->getSquare(dir)->getName());
        creature->destroy(dir);
        break;
      }
    }
  }
  for (Vec2 pos : creature->getLevel()->getVisibleTiles(creature)) {
    ViewIndex index = creature->getLevel()->getSquare(pos)->getViewIndex(creature);
    (*levelMemory)[creature->getLevel()].clearSquare(pos);
    for (ViewLayer l : { ViewLayer::ITEM, ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM})
      if (index.hasObject(l))
        remember(pos, index.getObject(l));
  }
}

bool Player::isPlayer() const {
  return true;
}

void Player::privateMessage(const string& message) const {
  messageBuffer.addMessage(message);
}

void Player::you(const string& param) const {
  privateMessage("You " + param);
}

void Player::you(MsgType type, const string& param) const {
  string msg;
  switch (type) {
    case MsgType::ARE: msg = "You are " + param; break;
    case MsgType::YOUR: msg = "Your " + param; break;
    case MsgType::FEEL: msg = "You feel " + param; break;
    case MsgType::FALL_ASLEEP: msg = "You fall asleep" + (param.size() > 0 ? " on the " + param : "."); break;
    case MsgType::WAKE_UP: msg = "You wake up."; break;
    case MsgType::FALL_APART: msg = "You fall apart."; break;
    case MsgType::FALL: msg = "You fall on the " + param; break;
    case MsgType::DIE: messageBuffer.addMessage("You die!!"); break;
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
    case MsgType::SWING_WEAPON: msg = "You swing your " + param; break;
    case MsgType::THRUST_WEAPON: msg = "You thrust your " + param; break;
    case MsgType::ATTACK_SURPRISE: msg = "You sneak attack " + param; break;
    case MsgType::KICK: msg = "You kick " + param; break;
    case MsgType::BITE: msg = "You bite " + param; break;
    case MsgType::PUNCH: msg = "You punch " + param; break;
    case MsgType::PANIC:
          msg = !creature->isHallucinating() ? "You are suddenly very afraid" : "You freak out completely"; break;
    case MsgType::RAGE:
          msg = !creature->isHallucinating() ?"You are suddenly very angry" : "This will be a very long trip."; break;
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
    case MsgType::TURN: msg = "You turn into " + param; break;
    case MsgType::HIT: msg = "You hit " + param; break;
    default: break;
  }
  messageBuffer.addMessage(msg);
}


void Player::onKilled(const Creature* attacker) {
  if (!creature->canPopController()) {
    model->gameOver(creature, creature->getKills().size(), "monsters", creature->getPoints());
  } else
    creature->popController();
}
