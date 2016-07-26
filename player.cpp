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
#include "game.h"
#include "options.h"
#include "creature.h"
#include "item_factory.h"
#include "effect.h"
#include "view_id.h"
#include "map_memory.h"
#include "player_message.h"
#include "square_apply_type.h"
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
#include "event_proxy.h"
#include "visibility_map.h"
#include "collective_name.h"

template <class Archive>
void Player::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Controller);
  serializeAll(ar, travelling, travelDir, target, lastLocation, displayGreeting, levelMemory, messages);
  serializeAll(ar, messageHistory, adventurer, eventProxy, visibilityMap);
}

SERIALIZABLE(Player);

SERIALIZATION_CONSTRUCTOR_IMPL(Player);

Player::Player(Creature* c, Model* m, bool adv, MapMemory* memory) :
    Controller(c), levelMemory(memory), eventProxy(this, m), adventurer(adv), displayGreeting(adventurer) {
  visibilityMap->update(c, c->getVisibleTiles());
}

Player::~Player() {
}

void Player::onEvent(const GameEvent& event) {
  switch (event.getId()) {
    case EventId::MOVED: 
      if (event.get<Creature*>() == getCreature())
        visibilityMap->update(getCreature(), getCreature()->getVisibleTiles());
      break;
    case EventId::ITEMS_THROWN: {
        auto info = event.get<EventInfo::ItemsThrown>();
        if (getCreature()->getPosition().isSameLevel(info.level))
          for (Vec2 v : info.trajectory)
            if (getCreature()->canSee(v)) {
              getView()->animateObject(info.trajectory, info.items[0]->getViewObject());
              return;
            }
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
        Collective* col = event.get<Collective*>();
        if (col->getVillainType() == VillainType::MAIN || col->getVillainType() == VillainType::LESSER)
          privateMessage(PlayerMessage("The tribe of " + col->getName().getFull() + " is destroyed.",
                MessagePriority::CRITICAL));
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

ControllerFactory Player::getFactory(Model* m, MapMemory* levelMemory) {
  return ControllerFactory([=](Creature* c) { return new Player(c, m, true, levelMemory);});
}

static string getSlotSuffix(EquipmentSlot slot) {
  return "(equipped)";
}

void Player::onBump(Creature*) {
  FAIL << "Shouldn't call onBump on a player";
}

string Player::getInventoryItemName(const Item* item, bool plural) const {
  if (getCreature()->getEquipment().isEquiped(item))
    return item->getNameAndModifiers(plural, getCreature()) + " "
      + getSlotSuffix(item->getEquipmentSlot());
  else
    return item->getNameAndModifiers(plural, getCreature());
}

void Player::getItemNames(vector<Item*> items, vector<ListElem>& names, vector<vector<Item*> >& groups,
    ItemPredicate predicate) {
  map<string, vector<Item*> > ret = groupBy<Item*, string>(items,
      [this] (Item* const& item) { return getInventoryItemName(item, false); });
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

static string getSquareQuestion(SquareApplyType type, string name) {
  switch (type) {
    case SquareApplyType::USE_STAIRS: return "use " + name;
    case SquareApplyType::USE_CHEST: return "open " + name;
    case SquareApplyType::DRINK: return "drink from " + name;
    case SquareApplyType::PRAY: return "pray at " + name;
    case SquareApplyType::SLEEP: return "sleep on " + name;
    case SquareApplyType::NOTICE_BOARD: return "view " + name;
    default: break;
  }
  return "";
}

void Player::pickUpItemAction(int numStack, bool multi) {
  auto stacks = getCreature()->stackItems(getCreature()->getPickUpOptions());
  if (getUsableSquareApplyType()) {
    --numStack;
    if (numStack == -1) {
      getCreature()->applySquare().perform(getCreature());
      return;
    }
  }
  if (numStack < stacks.size()) {
    vector<Item*> items = stacks[numStack];
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
  vector<ListElem> names;
  vector<vector<Item*> > groups;
  for (auto elem : typeDisplayOrder)
    if (typeGroups[elem].size() > 0) {
      names.push_back(ListElem(getText(elem), ListElem::TITLE));
      getItemNames(typeGroups[elem], names, groups, predicate);
    }
  optional<int> index = getView()->chooseFromList(text, names, 0, MenuType::NORMAL, nullptr, exitAction);
  if (index)
    return groups[*index];
  return vector<Item*>();
}

void Player::applyItem(vector<Item*> items) {
  if (getCreature()->isBlind() && contains({ItemClass::SCROLL, ItemClass::BOOK}, items[0]->getClass())) {
    privateMessage("You can't read while blind!");
    return;
  }
  if (items[0]->getApplyTime() > 1) {
    for (const Creature* c : getCreature()->getVisibleEnemies())
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

void Player::throwItem(vector<Item*> items, optional<Vec2> dir) {
  if (items[0]->getClass() == ItemClass::AMMO && getGame()->getOptions()->getBoolValue(OptionId::HINTS))
    privateMessage(PlayerMessage("To fire arrows equip a bow and use alt + direction key", MessagePriority::CRITICAL));
  if (!dir) {
    auto cDir = getView()->chooseDirection("Which direction do you want to throw?");
    if (!cDir)
      return;
    dir = *cDir;
  }
  tryToPerform(getCreature()->throwItem(items[0], *dir));
}

void Player::consumeAction() {
  vector<CreatureAction> actions;
  for (Vec2 v : Vec2::directions8())
    if (Creature* c = getCreature()->getPosition().plus(v).getCreature())
      if (auto action = getCreature()->consume(c))
      actions.push_back(action);
  if (actions.size() == 1) {
    tryToPerform(actions[0]);
  } else
  if (actions.size() > 1) {
    auto dir = getView()->chooseDirection("Which direction?");
    if (!dir)
      return;
    if (Creature* c = getCreature()->getPosition().plus(*dir).getCreature())
      tryToPerform(getCreature()->consume(c));
  }
}

vector<ItemAction> Player::getItemActions(const vector<Item*>& item) const {
  vector<ItemAction> actions;
  if (getCreature()->equip(item[0]))
    actions.push_back(ItemAction::EQUIP);
  if (getCreature()->applyItem(item[0]))
    actions.push_back(ItemAction::APPLY);
  if (getCreature()->unequip(item[0]))
    actions.push_back(ItemAction::UNEQUIP);
  else {
    for (Position v : getCreature()->getPosition().neighbors8())
      if (Creature* c = v.getCreature())
        if (getCreature()->isFriend(c)/* && c->canTakeItems(item)*/) {
          actions.push_back(ItemAction::GIVE);
          break;
        }
    actions.push_back(ItemAction::THROW);
    actions.push_back(ItemAction::DROP);
    if (item.size() > 1)
      actions.push_back(ItemAction::DROP_MULTI);
  }
  return actions;
}

void Player::handleItems(const vector<UniqueEntity<Item>::Id>& itemIds, ItemAction action) {
  vector<Item*> items = getCreature()->getEquipment().getItems(
      [&](const Item* it) { return contains(itemIds, it->getUniqueId());});
  //CHECK(items.size() == itemIds.size()) << int(items.size()) << " " << int(itemIds.size());
  // the above assertion fails for unknown reason, so just fail this softly.
  if (items.empty() || (items.size() == 1 && action == ItemAction::DROP_MULTI)) 
    return;
  switch (action) {
    case ItemAction::DROP: tryToPerform(getCreature()->drop(items)); break;
    case ItemAction::DROP_MULTI:
      if (auto num = getView()->getNumber("Drop how many " + items[0]->getName(true) + "?", 1, items.size()))
        tryToPerform(getCreature()->drop(getPrefix(items, *num))); break;
    case ItemAction::THROW: throwItem(items); break;
    case ItemAction::APPLY: applyItem(items); break;
    case ItemAction::UNEQUIP: tryToPerform(getCreature()->unequip(items[0])); break;
    case ItemAction::GIVE: giveAction(items); break;
    case ItemAction::EQUIP: 
      if (getCreature()->isEquipmentAppropriate(items[0]) || getView()->yesOrNoPrompt(
          items[0]->getTheName() + " is too heavy and will incur an accuracy penalty. Do you want to continue?"))
        tryToPerform(getCreature()->equip(items[0])); break;
    default: FAIL << "Unhandled item action " << int(action);
  }
}

void Player::hideAction() {
  tryToPerform(getCreature()->hide());
}

bool Player::interruptedByEnemy() {
  vector<Creature*> enemies = getCreature()->getVisibleEnemies();
  vector<string> ignoreCreatures { "a boar" ,"a deer", "a fox", "a vulture", "a rat", "a jackal", "a boulder" };
  if (enemies.size() > 0) {
    for (Creature* c : enemies)
      if (!contains(ignoreCreatures, c->getName().a())) {
        getView()->updateView(this, false);
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
    Debug() << "Stopped by multiple routes";
    return;
  }
  optional<int> myIndex = findElement(squareDirs, -travelDir);
  if (!myIndex) { // This was an assertion but was failing
    travelling = false;
    Debug() << "Stopped by bad travel data";
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
  if (interruptedByEnemy())
    target = none;
}

void Player::payDebtAction() {
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (Creature* c = pos.getCreature()) {
      if (int debt = c->getDebt(getCreature())) {
        vector<Item*> gold = getCreature()->getGold(debt);
        if (gold.size() < debt) {
          privateMessage("You don't have enough gold to pay.");
        } else if (getView()->yesOrNoPrompt("Buy items for " + toString(debt) + " gold?")) {
          if (tryToPerform(getCreature()->give(c, gold)))
            privateMessage("You pay " + c->getName().the() + " " + toString(debt) + " gold.");
        }
      } else {
        Debug() << "No debt " << c->getName().bare();
      }
    }
}

void Player::giveAction(vector<Item*> items) {
  if (items.size() > 1) {
    if (auto num = getView()->getNumber("Give how many " + items[0]->getName(true) + "?", 1, items.size()))
      items = getPrefix(items, *num);
    else
      return;
  }
  vector<Creature*> creatures;
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (Creature* c = pos.getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && getView()->yesOrNoPrompt("Give " + items[0]->getTheName(items.size() > 1) +
        " to " + creatures[0]->getName().the() + "?"))
    tryToPerform(getCreature()->give(creatures[0], items));
  else if (auto dir = getView()->chooseDirection("Give whom?"))
    if (Creature* whom = getCreature()->getPosition().plus(*dir).getCreature())
      tryToPerform(getCreature()->give(whom, items));
}

void Player::chatAction(optional<Vec2> dir) {
  vector<Creature*> creatures;
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (Creature* c = pos.getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && !dir) {
    tryToPerform(getCreature()->chatTo(creatures[0]));
  } else
  if (creatures.size() > 1 || dir) {
    if (!dir)
      dir = getView()->chooseDirection("Which direction?");
    if (!dir)
      return;
    if (Creature* c = getCreature()->getPosition().plus(*dir).getCreature())
      tryToPerform(getCreature()->chatTo(c));
  }
}

void Player::fireAction(Vec2 dir) {
  tryToPerform(getCreature()->fire(dir));
}

void Player::spellAction(SpellId id) {
  Spell* spell = Spell::get(id);
  if (!spell->isDirected())
    tryToPerform(getCreature()->castSpell(spell));
  else if (auto dir = getView()->chooseDirection("Which direction?"))
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
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (Creature* c = pos.getCreature())
      if (c->getUniqueId() == id) {
        if (getCreature()->isEnemy(c))
          tryToPerform(getCreature()->attack(c));
        else if (auto move = getCreature()->move(c->getPosition()))
          move.perform(getCreature());
        else
          tryToPerform(getCreature()->chatTo(c));
        break;
      }
}

void Player::extendedAttackAction(Creature::Id id) {
  for (Position pos : getCreature()->getPosition().neighbors8())
    if (Creature* c = pos.getCreature())
      if (c->getUniqueId() == id) {
        extendedAttackAction(c);
        return;
      }
}

void Player::extendedAttackAction(Creature* other) {
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
  if (!messages.empty()) {
    messages = {messages.back()};
    messages[0].setFreshness(0);
  }
}

void Player::makeMove() {
  if (adventurer)
    considerAdventurerMusic();
  if (currentTimePos && currentTimePos->pos.getLevel() != getCreature()->getLevel()) {
    previousTimePos = currentTimePos = none;
  }
  if (getCreature()->isAffected(LastingEffect::HALLU))
    ViewObject::setHallu(true);
  else
    ViewObject::setHallu(false);
  if (updateView) {
    updateView = false;
    for (Position pos : getCreature()->getVisibleTiles()) {
      ViewIndex index;
      pos.getViewIndex(index, getCreature());
      levelMemory->update(pos, index);
    }
    MEASURE(
        getView()->updateView(this, false),
        "level render time");
  } 
  getView()->refreshView();
  if (displayTravelInfo && getCreature()->getPosition().getName() == "road" 
      && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    getView()->presentText("", "Use ctrl + arrows to travel quickly on roads and corridors.");
    displayTravelInfo = false;
  }
  if (displayGreeting && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    CHECK(getCreature()->getName().first());
    getView()->updateView(this, true);
    getView()->presentText("", "Dear " + *getCreature()->getName().first() + ",\n \n \tIf you are reading this letter, then you have arrived in the valley of " + getGame()->getWorldName() + ". This land is swarming with evil, and you need to destroy all of it, like in a bad RPG game. Humans, dwarves and elves are your allies. Find them, talk to them, they will help you. Let your sword guide you.\n \n \nYours, " + NameGenerator::get(NameGeneratorId::FIRST)->getNext() + "\n \nPS.: Beware the orcs!");
    displayGreeting = false;
    getView()->updateView(this, false);
  }
  UserInput action = getView()->getAction();
  if (travelling && action.getId() == UserInputId::IDLE)
    travelAction();
  else if (target && action.getId() == UserInputId::IDLE)
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
      getView()->resetCenter();
    }
    updateView = true;
  }
  switch (action.getId()) {
    case UserInputId::FIRE: fireAction(action.get<Vec2>()); break;
    case UserInputId::TRAVEL: travel = true;
    case UserInputId::MOVE: direction.push_back(action.get<Vec2>()); break;
    case UserInputId::MOVE_TO: {
      Position newPos = getCreature()->getPosition().withCoord(action.get<Vec2>());
      if (newPos.dist8(getCreature()->getPosition()) == 1) {
        Vec2 dir = getCreature()->getPosition().getDir(newPos);
        if (Creature* c = newPos.getCreature()) {
          creatureAction(c->getUniqueId());
          break;
        }
        direction.push_back(dir);
      } else
      if (newPos != getCreature()->getPosition()) {
        if (!wasJustTravelling) {
          target = newPos;
/*          Vec2 t = action.get<Vec2>();
          t = Vec2(min(getCreature()->getLevel()->getBounds().right() - 1, max(0, t.x)),
              min(getCreature()->getLevel()->getBounds().bottom() - 1, max(0, t.y)));*/
        }
      }
      break;
      }
    case UserInputId::INVENTORY_ITEM:
      handleItems(action.get<InventoryItemInfo>().items, action.get<InventoryItemInfo>().action); break;
    case UserInputId::PICK_UP_ITEM: pickUpItemAction(action.get<int>()); break;
    case UserInputId::PICK_UP_ITEM_MULTI: pickUpItemAction(action.get<int>(), true); break;
    case UserInputId::WAIT: getCreature()->wait().perform(getCreature()); break;
    case UserInputId::HIDE: hideAction(); break;
    case UserInputId::PAY_DEBT: payDebtAction(); break;
    case UserInputId::CHAT: chatAction(); break;
    case UserInputId::CONSUME: consumeAction(); break;
    case UserInputId::SHOW_HISTORY: showHistory(); break;
    case UserInputId::UNPOSSESS:
      if (unpossess())
        return;
      break;
    case UserInputId::SWAP_TEAM:
      if (swapTeam())
        return;
      break;
    case UserInputId::CAST_SPELL: spellAction(action.get<SpellId>()); break;
    case UserInputId::DRAW_LEVEL_MAP: getView()->drawLevelMap(this); break;
    case UserInputId::CREATURE_BUTTON: creatureAction(action.get<Creature::Id>()); break;
    case UserInputId::CREATURE_BUTTON2: extendedAttackAction(action.get<Creature::Id>()); break;
    case UserInputId::EXIT: getGame()->exitAction(); return;
    case UserInputId::TRANSFER: getGame()->transferAction(getTeam()); return;
#ifndef RELEASE
    case UserInputId::CHEAT_ATTRIBUTES:
      getCreature()->getAttributes().setBaseAttr(AttrType::STRENGTH, 80);
      getCreature()->getAttributes().setBaseAttr(AttrType::DEXTERITY, 80);
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
      if (Creature* other = getCreature()->getPosition().plus(dir).getCreature())
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
  if (!getCreature()->isDead() && (!currentTimePos || getCreature()->getLocalTime() > currentTimePos->time)) {
    if (currentTimePos /*&& (!previousTimePos || currentTimePos->pos.isSameLevel(previousTimePos->pos))*/)
      previousTimePos = currentTimePos;
    else
      previousTimePos = none;
    currentTimePos = { getCreature()->getPosition(), getCreature()->getLocalTime()};
  }
}

void Player::showHistory() {
  PlayerMessage::presentMessages(getView(), messageHistory);
}

static string getForceMovementQuestion(Position pos, const Creature* creature) {
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
    tryToPerform(getCreature()->destroy(dir, Creature::BASH));
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
      messageHistory.push_back(message);
      if (!messages.empty() && messages.back().getFreshness() < 1)
        messages.clear();
      messages.emplace_back(message);
      if (message.getPriority() == MessagePriority::CRITICAL)
        view->presentText("Important!", message.getText());
    }
  }
}

void Player::you(const string& param) {
  privateMessage("You " + param);
}

void Player::you(MsgType type, const vector<string>& param) {
  string msg;
  switch (type) {
    case MsgType::SWING_WEAPON: msg = "You swing your " + param.at(0); break;
    case MsgType::THRUST_WEAPON: msg = "You thrust your " + param.at(0); break;
    case MsgType::KICK: msg = "You kick " + param.at(0); break;
    case MsgType::PUNCH: msg = "You punch " + param.at(0); break;
    default: you(type, param.at(0)); break;
  }
  if (param.size() > 1)
    msg += " " + param[1];
  privateMessage(msg);
}

void Player::you(MsgType type, const string& param) {
  string msg;
  switch (type) {
    case MsgType::ARE: msg = "You are " + param; break;
    case MsgType::YOUR: msg = "Your " + param; break;
    case MsgType::FEEL: msg = "You feel " + param; break;
    case MsgType::WAKE_UP: msg = "You wake up."; break;
    case MsgType::FALL: msg = "You fall " + param; break;
    case MsgType::FALL_ASLEEP: msg = "You fall asleep" + (param.size() > 0 ? " on the " + param : "."); break;
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
    case MsgType::DISARM_TRAP: msg = "You disarm the " + param; break;
    case MsgType::ATTACK_SURPRISE: msg = "You sneak attack " + param; break;
    case MsgType::BITE: msg = "You bite " + param; break;
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
    case MsgType::BREAK_FREE:
        if (param.empty())
          msg = "You break free";
        else
          msg = "You break free from " + param;
        break;
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

Level* Player::getLevel() const {
  return getCreature()->getLevel();
}

Game* Player::getGame() const {
  return getCreature()->getGame();
}

View* Player::getView() const {
  if (Game* game = getGame())
    return game->getView();
  else
    return nullptr;
}

Vec2 Player::getPosition() const {
  return getCreature()->getPosition().getCoord();
}

optional<CreatureView::MovementInfo> Player::getMovementInfo() const {
  if (previousTimePos && currentTimePos)
    return MovementInfo({previousTimePos->pos.getCoord(), currentTimePos->pos.getCoord(), previousTimePos->time,
        getCreature()->getUniqueId()});
  else
    return none;
}

void Player::onDisplaced() {
  currentTimePos->pos = getCreature()->getPosition();
}

void Player::getViewIndex(Vec2 pos, ViewIndex& index) const {
  bool canSee = getCreature()->canSee(pos) || getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP);
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
  if (const Creature* c = position.getCreature()) {
    if (getCreature()->canSee(c) || c == getCreature()) {
      index.insert(c->getViewObjectFor(getCreature()->getTribe()));
      if (contains(getTeam(), c))
        index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_HIGHLIGHT);
    } else if (getCreature()->isUnknownAttacker(c))
      index.insert(copyOf(ViewObject::unknownMonster()));
  }
 /* if (pos == getCreature()->getPosition() && index.hasObject(ViewLayer::CREATURE))
      index.getObject(ViewLayer::CREATURE).setModifier(ViewObject::Modifier::TEAM_LEADER_HIGHLIGHT);*/

}

void Player::onKilled(const Creature* attacker) {
  eventProxy->unsubscribe();
  getView()->updateView(this, false);
  if (getView()->yesOrNoPrompt("Display message history?"))
    showHistory();
  if (adventurer)
    getGame()->gameOver(getCreature(), getCreature()->getKills().getSize(), "monsters", getCreature()->getPoints());
}

bool Player::unpossess() {
  return false;
}

bool Player::swapTeam() {
  return false;
}

void Player::onFellAsleep() {
}

vector<Creature*> Player::getTeam() const {
  return {getCreature()};
}

optional<SquareApplyType> Player::getUsableSquareApplyType() const {
  if (auto applyType = getCreature()->getPosition().getApplyType(getCreature()))
    if (!getSquareQuestion(*applyType, getCreature()->getPosition().getName()).empty())
      return *applyType;
  return none;
}

void Player::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.messageBuffer = messages;
  gameInfo.singleModel = getGame()->isSingleModel();
  gameInfo.infoType = GameInfo::InfoType::PLAYER;
  SunlightInfo sunlightInfo = getGame()->getSunlightInfo();
  gameInfo.sunlightInfo.description = sunlightInfo.getText();
  gameInfo.sunlightInfo.timeRemaining = sunlightInfo.getTimeRemaining();
  gameInfo.time = getCreature()->getGame()->getGlobalTime();
  PlayerInfo& info = gameInfo.playerInfo;
  info.readFrom(getCreature());
  info.team.clear();
  for (const Creature* c : getTeam())
    info.team.push_back(c);
  const Location* location = getCreature()->getPosition().getLocation();
  info.levelName = location && location->getName()
    ? capitalFirst(*location->getName()) : getLevel()->getName();
  info.lyingItems.clear();
  if (auto applyType = getUsableSquareApplyType()) {
    string question = getSquareQuestion(*applyType, getCreature()->getPosition().getName());
    info.lyingItems.push_back(getApplySquareInfo(question, getCreature()->getPosition().getViewObject().id()));
  }
  for (auto stack : getCreature()->stackItems(getCreature()->getPickUpOptions()))
    info.lyingItems.push_back(getItemInfo(stack));
  info.inventory.clear();
  map<ItemClass, vector<Item*> > typeGroups = groupBy<Item*, ItemClass>(
      getCreature()->getEquipment().getItems(), [](Item* const& item) { return item->getClass();});
  for (auto elem : typeDisplayOrder)
    if (typeGroups[elem].size() > 0)
      append(info.inventory, getItemInfos(typeGroups[elem]));
}

ItemInfo Player::getApplySquareInfo(const string& question, ViewId viewId) const {
  return CONSTRUCT(ItemInfo,
    c.name = question;
    c.fullName = c.name;
    c.description = "Click to " + c.name;
    c.number = 1;
    c.viewId = viewId;);
}

ItemInfo Player::getItemInfo(const vector<Item*>& stack) const {
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(getCreature());
    c.fullName = stack[0]->getNameAndModifiers(false, getCreature());
    c.description = getCreature()->isBlind() ? "" : stack[0]->getDescription();
    c.number = stack.size();
    c.viewId = stack[0]->getViewObject().id();
    c.ids = transform2<UniqueEntity<Item>::Id>(stack, [](const Item* it) { return it->getUniqueId();});
    c.actions = getItemActions(stack);
    c.equiped = getCreature()->getEquipment().isEquiped(stack[0]); );
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
  return transform2<Vec2>(getCreature()->getVisibleEnemies(),
      [](const Creature* c) { return c->getPosition().getCoord(); });
}

double Player::getLocalTime() const {
  return getCreature()->getLocalTime();
}

bool Player::isPlayerView() const {
  return true;
}

void Player::considerAdventurerMusic() {
  for (Collective* col : getCreature()->getPosition().getModel()->getCollectives())
    if (col->getVillainType() == VillainType::MAIN && !col->isConquered() &&
        col->getTerritory().contains(getCreature()->getPosition())) {
      getGame()->setCurrentMusic(MusicType::ADV_BATTLE, true);
      return;
    }
  getGame()->setCurrentMusic(MusicType::ADV_PEACEFUL, true);
}

