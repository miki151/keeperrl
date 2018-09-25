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
#include "spell_map.h"
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
#include "pretty_printing.h"
#include "item_type.h"
#include "creature_factory.h"
#include "time_queue.h"
#include "unknown_locations.h"
#include "furniture_click.h"

template <class Archive>
void Player::serialize(Archive& ar, const unsigned int) {
  ar& SUBCLASS(Controller) & SUBCLASS(EventListener);
  ar(travelling, travelDir, target, displayGreeting, levelMemory, messageBuffer);
  ar(adventurer, visibilityMap, tutorial, unknownLocations);
}

SERIALIZABLE(Player)

SERIALIZATION_CONSTRUCTOR_IMPL(Player)

Player::Player(WCreature c, bool adv, SMapMemory memory, SMessageBuffer buf, SVisibilityMap v, SUnknownLocations loc,
      STutorial t) :
    Controller(c), levelMemory(memory), adventurer(adv), displayGreeting(adventurer), messageBuffer(buf),
    visibilityMap(v), tutorial(t), unknownLocations(loc) {
  visibilityMap->update(c, c->getVisibleTiles());
}

Player::~Player() {
}

void Player::onEvent(const GameEvent& event) {
  using namespace EventInfo;
  event.visit(
      [&](const CreatureMoved& info) {
        if (info.creature == creature)
          visibilityMap->update(creature, creature->getVisibleTiles());
      },
      [&](const Projectile& info) {
        if (creature->canSee(info.begin) || creature->canSee(info.end))
          getView()->animateObject(info.begin.getCoord(), info.end.getCoord(), info.viewId);
      },
      [&](const Explosion& info) {
        if (creature->getPosition().isSameLevel(info.pos)) {
          privateMessage("BOOM!");
        }
      },
      [&](const CreatureKilled& info) {
        auto pos = info.victim->getPosition();
        if (creature->canSee(pos))
          if (auto anim = info.victim->getBody().getDeathAnimation())
            getView()->animation(pos.getCoord(), *anim);
      },
      [&](const CreatureAttacked& info) {
        auto pos = info.victim->getPosition();
        if (creature->canSee(pos)) {
          auto dir = info.attacker->getPosition().getDir(pos);
          if (dir.length8() == 1) {
            auto orientation = dir.getCardinalDir();
            getView()->animation(pos.getCoord(), AnimationId::ATTACK, orientation);
          }
        }
      },
      [&](const Alarm& info) {
        if (!info.silent) {
          Position pos = info.pos;
          Position myPos = creature->getPosition();
          if (pos == myPos)
            privateMessage("An alarm sounds near you.");
          else if (pos.isSameLevel(myPos))
            privateMessage("An alarm sounds in the " +
                getCardinalName(myPos.getDir(pos).getBearing().getCardinalDir()));
        }
      },
      [&](const ConqueredEnemy& info) {
        if (adventurer && info.collective->isDiscoverable()) {
          if (auto& name = info.collective->getName())
            privateMessage(PlayerMessage("The tribe of " + name->full + " is destroyed.",
                  MessagePriority::CRITICAL));
          else
            privateMessage(PlayerMessage("An unnamed tribe is destroyed.", MessagePriority::CRITICAL));
        }
      },
      [&](const OtherEffect& info) {
        // TODO: in Keeper mode effects are spawned twice: once from Player, once from PlayerControl
        if (info.alwaysShow || creature->canSee(info.position))
          getView()->animation(info.effect, info.position.getCoord(), info.targetOffset, info.color);
      },
      [&](const WonGame&) {
        if (adventurer)
          getGame()->conquered(*creature->getName().first(), creature->getKills().getSize(), creature->getPoints());
      },
      [&](const auto&) {}
  );
}

static string getSlotSuffix(EquipmentSlot slot) {
  return "(equipped)";
}

string Player::getInventoryItemName(WConstItem item, bool plural) const {
  if (creature->getEquipment().isEquipped(item))
    return item->getNameAndModifiers(plural, creature) + " "
      + getSlotSuffix(item->getEquipmentSlot());
  else
    return item->getNameAndModifiers(plural, creature);
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
  auto stacks = creature->stackItems(creature->getPickUpOptions());
  if (getUsableUsageType()) {
    --numStack;
    if (numStack == -1) {
      creature->applySquare(creature->getPosition()).perform(creature);
      return;
    }
  }
  if (numStack < stacks.size()) {
    vector<WItem> items = stacks[numStack];
    if (multi && items.size() > 1) {
      auto num = getView()->getNumber("Pick up how many " + items[0]->getName(true) + "?",
          Range(1, items.size()), 1);
      if (!num)
        return;
      items = getPrefix(items, *num);
    }
    tryToPerform(creature->pickUp(items));
  }
}

bool Player::tryToPerform(CreatureAction action) {
  if (action)
    action.perform(creature);
  else
    privateMessage(action.getFailedReason());
  return !!action;
}

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
      creature->getEquipment().getItems(), [](WItem const& item) { return item->getClass();});
  vector<ListElem> names;
  vector<vector<WItem> > groups;
  for (auto elem : ENUM_ALL(ItemClass))
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
  PROFILE;
  if (creature->isAffected(LastingEffect::BLIND) &&
      contains({ItemClass::SCROLL, ItemClass::BOOK}, items[0]->getClass())) {
    privateMessage("You can't read while blind!");
    return;
  }
  if (items[0]->getApplyTime() > 1_visible) {
    for (WConstCreature c : creature->getVisibleEnemies())
      if (creature->getPosition().dist8(c->getPosition()) < 3) {
        if (!getView()->yesOrNoPrompt("Applying " + items[0]->getAName() + " takes " +
            toString(items[0]->getApplyTime()) + " turns. Are you sure you want to continue?"))
          return;
        else
          break;
      }
  }
  tryToPerform(creature->applyItem(items[0]));
}

optional<Vec2> Player::chooseDirection(const string& s) {
  return getView()->chooseDirection(creature->getPosition().getCoord(), s);
}

void Player::throwItem(vector<WItem> items, optional<Vec2> dir) {
  if (!dir) {
    auto cDir = chooseDirection("Which direction do you want to throw?");
    if (!cDir)
      return;
    dir = *cDir;
  }
  tryToPerform(creature->throwItem(items[0], *dir));
}

void Player::handleIntrinsicAttacks(const EntitySet<Item>& itemIds, ItemAction action) {
  auto& attacks = creature->getBody().getIntrinsicAttacks();
  for (auto part : ENUM_ALL(BodyPart))
    if (auto& attack = attacks[part])
      if (itemIds.contains(attack->item.get()))
        switch (action) {
          case ItemAction::INTRINSIC_ALWAYS:
            attack->active = IntrinsicAttack::ALWAYS;
            break;
          case ItemAction::INTRINSIC_NO_WEAPON:
            attack->active = IntrinsicAttack::NO_WEAPON;
            break;
          case ItemAction::INTRINSIC_NEVER:
            attack->active = IntrinsicAttack::NEVER;
            break;
          default:
            FATAL << "Unhandled intrinsic item action: " << (int) action;
            break;
        }
}

void Player::handleItems(const EntitySet<Item>& itemIds, ItemAction action) {
  vector<WItem> items = creature->getEquipment().getItems().filter(
      [&](WConstItem it) { return itemIds.contains(it);});
  //CHECK(items.size() == itemIds.size()) << int(items.size()) << " " << int(itemIds.size());
  // the above assertion fails for unknown reason, so just fail this softly.
  if (items.empty() || (items.size() == 1 && action == ItemAction::DROP_MULTI))
    return;
  switch (action) {
    case ItemAction::DROP: tryToPerform(creature->drop(items)); break;
    case ItemAction::DROP_MULTI:
      if (auto num = getView()->getNumber("Drop how many " + items[0]->getName(true) + "?",
          Range(1, items.size()), 1))
        tryToPerform(creature->drop(getPrefix(items, *num)));
      break;
    case ItemAction::THROW: throwItem(items); break;
    case ItemAction::APPLY: applyItem(items); break;
    case ItemAction::UNEQUIP: tryToPerform(creature->unequip(items[0])); break;
    case ItemAction::GIVE: giveAction(items); break;
    case ItemAction::PAY: payForItemAction(items); break;
    case ItemAction::EQUIP: tryToPerform(creature->equip(items[0])); break;
    case ItemAction::NAME:
      if (auto name = getView()->getText("Enter a name for " + items[0]->getTheName(),
          items[0]->getArtifactName().value_or(""), 14))
        items[0]->setArtifactName(*name);
      break;
    default: FATAL << "Unhandled item action " << int(action);
  }
}

void Player::hideAction() {
  tryToPerform(creature->hide());
}

bool Player::interruptedByEnemy() {
  if (auto combatIntent = creature->getLastCombatIntent())
    if (combatIntent->time > lastEnemyInterruption && combatIntent->time > getGame()->getGlobalTime() - 5_visible) {
      lastEnemyInterruption = combatIntent->time;
      privateMessage("You are being attacked by " + combatIntent->attacker->getName().a());
      return true;
    }
  return false;
}

void Player::travelAction() {
/*  updateView = true;
  if (!creature->move(travelDir) || getView()->travelInterrupt() || interruptedByEnemy()) {
    travelling = false;
    return;
  }
  tryToPerform(creature->move(travelDir));
  const Location* currentLocation = creature->getPosition().getLocation();
  if (lastLocation != currentLocation && currentLocation != nullptr && currentLocation->getName()) {
    privateMessage("You arrive at " + addAParticle(*currentLocation->getName()));
    travelling = false;
    return;
  }
  vector<Vec2> squareDirs = creature->getPosition().getTravelDir();
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
  if (creature->getPosition() == *target || getView()->travelInterrupt()) {
    target = none;
    return;
  }
  if (auto action = creature->moveTowards(*target, Creature::NavigationFlags().noDestroying()))
    action.perform(creature);
  else
    target = none;
  if (interruptedByEnemy() || !isTravelEnabled())
    target = none;
}

void Player::payForItemAction(const vector<WItem>& items) {
  int totalPrice = (int) items.size() * items[0]->getPrice();
  for (auto item : items) {
    CHECK(item->getShopkeeper(creature));
    CHECK(item->getPrice() == items[0]->getPrice());
  }
  vector<WItem> gold = creature->getGold(totalPrice);
  int canPayFor = (int) gold.size() / items[0]->getPrice();
  if (canPayFor == 0)
    privateMessage("You don't have enough gold to pay.");
  else if (canPayFor == items.size() || getView()->yesOrNoPrompt("You only have enough gold for " +
      toString(canPayFor) + " " + items[0]->getName(canPayFor > 1, creature) + ". Still pay?"))
    tryToPerform(creature->payFor(getPrefix(items, canPayFor)));
}

void Player::payForAllItemsAction() {
  if (int totalDebt = creature->getDebt().getTotal()) {
    auto gold = creature->getGold(totalDebt);
    if (gold.size() < totalDebt)
      privateMessage("You don't have enough gold to pay for everything.");
    else {
      for (auto creatureId : creature->getDebt().getCreditors())
        for (WCreature c : creature->getVisibleCreatures())
          if (c->getUniqueId() == creatureId &&
              getView()->yesOrNoPrompt("Give " + c->getName().the() + " " + toString(gold.size()) + " gold?")) {
              if (tryToPerform(creature->give(c, gold)))
                for (auto item : creature->getEquipment().getItems())
                  item->setShopkeeper(nullptr);
      }
    }
  }
}

void Player::giveAction(vector<WItem> items) {
  PROFILE;
  if (items.size() > 1) {
    if (auto num = getView()->getNumber("Give how many " + items[0]->getName(true) + "?", Range(1, items.size()), 1))
      items = getPrefix(items, *num);
    else
      return;
  }
  vector<WCreature> creatures;
  for (Position pos : creature->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && getView()->yesOrNoPrompt("Give " + items[0]->getTheName(items.size() > 1) +
        " to " + creatures[0]->getName().the() + "?"))
    tryToPerform(creature->give(creatures[0], items));
  else if (auto dir = chooseDirection("Give whom?"))
    if (WCreature whom = creature->getPosition().plus(*dir).getCreature())
      tryToPerform(creature->give(whom, items));
}

void Player::chatAction(optional<Vec2> dir) {
  PROFILE;
  vector<WCreature> creatures;
  for (Position pos : creature->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      creatures.push_back(c);
  if (creatures.size() == 1 && !dir) {
    tryToPerform(creature->chatTo(creatures[0]));
  } else
  if (creatures.size() > 1 || dir) {
    if (!dir)
      dir = chooseDirection("Which direction?");
    if (!dir)
      return;
    if (WCreature c = creature->getPosition().plus(*dir).getCreature())
      tryToPerform(creature->chatTo(c));
  }
}

void Player::fireAction() {
  if (auto testAction = creature->fire(Vec2(1, 0))) {
    if (auto dir = chooseDirection("Fire which direction?"))
      fireAction(*dir);
  } else
    privateMessage(testAction.getFailedReason());
}

void Player::fireAction(Vec2 dir) {
  tryToPerform(creature->fire(dir));
}

void Player::spellAction(SpellId id) {
  Spell* spell = Spell::get(id);
  if (!spell->isDirected())
    tryToPerform(creature->castSpell(spell));
  else if (auto dir = chooseDirection("Which direction?"))
    tryToPerform(creature->castSpell(spell, *dir));
}

const MapMemory& Player::getMemory() const {
  return *levelMemory;
}

void Player::sleeping() {
  PROFILE;
  MEASURE(
      getView()->updateView(this, false),
      "level render time");
}

vector<Player::OtherCreatureCommand> Player::getOtherCreatureCommands(WCreature c) const {
  vector<OtherCreatureCommand> ret;
  auto genAction = [&](int priority, const string& text, bool allowAuto, CreatureAction action) {
    if (action)
      ret.push_back({priority, text, allowAuto, [action](Player* player) { player->tryToPerform(action); }});
  };
  if (c->getPosition().dist8(creature->getPosition()) == 1) {
    genAction(0, "Swap position", true, creature->move(c->getPosition(), none));
    genAction(3, "Push", true, creature->push(c));
  }
  if (creature->isEnemy(c)) {
    genAction(1, "Attack", true, creature->attack(c));
    auto equipped = creature->getEquipment().getSlotItems(EquipmentSlot::WEAPON);
    if (equipped.size() == 1) {
      auto weapon = equipped[0];
      genAction(4, "Attack using " + weapon->getName(), true, creature->attack(c,
          CONSTRUCT(Creature::AttackParams, c.weapon = weapon;)));
    }
    for (auto part : ENUM_ALL(BodyPart))
      if (auto& attack = creature->getBody().getIntrinsicAttacks()[part])
        genAction(4, "Attack using " + attack->item->getName(), true, creature->attack(c,
            CONSTRUCT(Creature::AttackParams, c.weapon = attack->item.get();)));
  } else {
    genAction(1, "Attack", false, creature->attack(c));
    genAction(1, "Pet", false, creature->pet(c));
  }
  if (creature == c)
    genAction(0, "Skip turn", true, creature->wait());
  if (c->getPosition().dist8(creature->getPosition()) == 1)
    genAction(10, "Chat", false, creature->chatTo(c));
  std::stable_sort(ret.begin(), ret.end(), [](const auto& c1, const auto& c2) {
    if (c1.allowAuto && !c2.allowAuto)
      return true;
    if (c2.allowAuto && !c1.allowAuto)
      return false;
    return c1.priority < c2.priority;
  });
  return ret;
}

void Player::creatureClickAction(Position pos, bool extended) {
  if (auto clicked = pos.getCreature()) {
    auto commands = getOtherCreatureCommands(clicked);
    if (extended) {
      auto commandsText = commands.transform([](auto& command) -> string { return command.name; });
      if (auto index = getView()->chooseAtMouse(commandsText))
        commands[*index].perform(this);
    }
    else if (!commands.empty() && commands[0].allowAuto)
      commands[0].perform(this);
  } else
  if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE))
    if (furniture->getClickType() && furniture->getTribe() == creature->getTribeId()) {
      furniture->click(pos);
      updateSquareMemory(pos);
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
  PROFILE;
  bool canChat = false;
  for (Position pos : creature->getPosition().neighbors8())
    if (WCreature c = pos.getCreature())
      canChat = true;
  return {
    {PlayerInfo::CommandInfo{"Fire ranged weapon", 'f', "", true},
      [] (Player* player) { player->fireAction(); }, false},
    {PlayerInfo::CommandInfo{"Skip turn", ' ', "Skip this turn.", true},
      [] (Player* player) { player->tryToPerform(player->creature->wait()); }, false},
    {PlayerInfo::CommandInfo{"Wait", 'w', "Wait until all other team members make their moves (doesn't skip turn).",
        getGame()->getPlayerCreatures().size() > 1},
      [] (Player* player) { player->getModel()->getTimeQueue().postponeMove(player->creature); }, false},
    /*{PlayerInfo::CommandInfo{"Travel", 't', "Travel to another site.", !getGame()->isSingleModel()},
      [] (Player* player) { player->getGame()->transferAction(player->getTeam()); }, false},*/
    {PlayerInfo::CommandInfo{"Chat", 'c', "Chat with someone.", canChat},
      [] (Player* player) { player->chatAction(); }, false},
    {PlayerInfo::CommandInfo{"Hide", 'h', "Hide behind or under a terrain feature or piece of furniture.",
        !!creature->hide()},
      [] (Player* player) { player->hideAction(); }, false},
    /*{PlayerInfo::CommandInfo{"Pay for all items", 'p', "Pay debt to a shopkeeper.", true},
      [] (Player* player) { player->payForAllItemsAction();}, false},*/
    {PlayerInfo::CommandInfo{"Drop everything", none, "Drop all items in possession.", !creature->getEquipment().isEmpty()},
      [] (Player* player) { auto c = player->creature; player->tryToPerform(c->drop(c->getEquipment().getItems())); }, false},
    {PlayerInfo::CommandInfo{"Message history", 'm', "Show message history.", true},
      [] (Player* player) { player->showHistory(); }, false},
  };
}

void Player::updateUnknownLocations() {
  vector<Position> locations;
  for (auto col : getModel()->getCollectives())
    if (col->getLevel() == getLevel() && !col->isConquered())
      if (auto& pos = col->getTerritory().getCentralPoint())
        if (!getMemory().getViewIndex(*pos))
          locations.push_back(*pos);
  unknownLocations->update(locations);
}

void Player::updateSquareMemory(Position pos) {
  ViewIndex index;
  pos.getViewIndex(index, creature);
  levelMemory->update(pos, index);
}

void Player::makeMove() {
  PROFILE;
  updateUnknownLocations();
  if (!isSubscribed())
    subscribeTo(getModel());
  if (adventurer)
    considerAdventurerMusic();
  //if (updateView) { Check disabled so that we update in every frame to avoid some square refreshing issues.
    updateView = false;
    for (Position pos : creature->getVisibleTiles())
      updateSquareMemory(pos);
    MEASURE(
        getView()->updateView(this, false),
        "level render time");
  //}
  getView()->refreshView();
  /*if (displayTravelInfo && creature->getPosition().getName() == "road"
      && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    getView()->presentText("", "Use ctrl + arrows to travel quickly on roads and corridors.");
    displayTravelInfo = false;
  }*/
  if (displayGreeting && getGame()->getOptions()->getBoolValue(OptionId::HINTS)) {
    CHECK(creature->getName().first());
    getView()->updateView(this, true);
    getView()->presentText("", "Dear " + *creature->getName().first() +
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
  if (!handleUserInput(action))
    switch (action.getId()) {
      case UserInputId::FIRE: fireAction(action.get<Vec2>()); break;
      case UserInputId::TRAVEL: travel = true;
        FALLTHROUGH;
      case UserInputId::MOVE: direction.push_back(action.get<Vec2>()); break;
      case UserInputId::TILE_CLICK: {
        Position newPos = creature->getPosition().withCoord(action.get<Vec2>());
        if (newPos.dist8(creature->getPosition()) == 1) {
          Vec2 dir = creature->getPosition().getDir(newPos);
          direction.push_back(dir);
        } else
        if (newPos != creature->getPosition() && !wasJustTravelling)
          target = newPos;
        break;
      }
      case UserInputId::INVENTORY_ITEM:
        handleItems(action.get<InventoryItemInfo>().items, action.get<InventoryItemInfo>().action);
        break;
      case UserInputId::INTRINSIC_ATTACK:
        handleIntrinsicAttacks(action.get<InventoryItemInfo>().items, action.get<InventoryItemInfo>().action);
        break;
      case UserInputId::PICK_UP_ITEM: pickUpItemAction(action.get<int>()); break;
      case UserInputId::PICK_UP_ITEM_MULTI: pickUpItemAction(action.get<int>(), true); break;
      case UserInputId::CAST_SPELL: spellAction(action.get<SpellId>()); break;
      case UserInputId::DRAW_LEVEL_MAP: getView()->drawLevelMap(this); break;
      case UserInputId::CREATURE_MAP_CLICK:
        creatureClickAction(Position(action.get<Vec2>(), getLevel()), false);
        break;
      case UserInputId::CREATURE_MAP_CLICK_EXTENDED:
        creatureClickAction(Position(action.get<Vec2>(), getLevel()), true);
        break;
      case UserInputId::EXIT: getGame()->exitAction(); return;
      case UserInputId::APPLY_EFFECT:
        if (auto effect = PrettyPrinting::parseObject<Effect>(action.get<string>()))
          effect->applyToCreature(creature, nullptr);
        else
          getView()->presentText("Sorry", "Couldn't parse \"" + action.get<string>() + "\"");
        break;
      case UserInputId::CREATE_ITEM:
        if (auto itemType = PrettyPrinting::parseObject<ItemType>(action.get<string>()))
          creature->take(itemType->setPrefixChance(1).get());
        else
          getView()->presentText("Sorry", "Couldn't parse \"" + action.get<string>() + "\"");
        break;
      case UserInputId::SUMMON_ENEMY:
        if (auto id = PrettyPrinting::parseObject<CreatureId>(action.get<string>())) {
          auto factory = CreatureFactory::singleCreature(TribeId::getMonster(), *id);
          Effect::summon(creature->getPosition(), factory, 1, 1000_visible,
              3_visible);
        } else
          getView()->presentText("Sorry", "Couldn't parse \"" + action.get<string>() + "\"");
        break;
      case UserInputId::PLAYER_COMMAND: {
        int index = action.get<int>();
        auto commands = getCommands();
        if (index >= 0 && index < commands.size()) {
          commands[index].perform(this);
          if (commands[index].actionKillsController)
            return;
        }
        break;
      }
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
      case UserInputId::SCROLL_TO_HOME:
        getView()->setScrollPos(getPosition());
        break;
      case UserInputId::DRAW_WORLD_MAP:
        getGame()->transferAction(getTeam());
        break;
  #ifndef RELEASE
      case UserInputId::CHEAT_ATTRIBUTES:
        creature->getAttributes().increaseBaseAttr(AttrType::DAMAGE, 80);
        creature->getAttributes().increaseBaseAttr(AttrType::DEFENSE, 80);
        creature->getAttributes().increaseBaseAttr(AttrType::SPELL_DAMAGE, 80);
        creature->addPermanentEffect(LastingEffect::SPEED, true);
        creature->addPermanentEffect(LastingEffect::FLYING, true);
        break;
      case UserInputId::CHEAT_SPELLS: {
        auto &spellMap = creature->getAttributes().getSpellMap();
        for (auto spell : EnumAll<SpellId>())
          spellMap.add(spell);
        spellMap.setAllReady();
        break;
      }
      case UserInputId::CHEAT_POTIONS: {
        auto &items = creature->getEquipment().getItems();
        for (auto leType : ENUM_ALL(LastingEffect)) {
          bool found = false;
          for (auto &item : items)
            if (auto &eff = item->getEffect())
              if (auto le = eff->getValueMaybe<Effect::Lasting>())
                if (le->lastingEffect == leType) {
                  found = true;
                  break;
                }
          if (!found) {
            ItemType itemType{ItemType::Potion{Effect::Lasting{leType}}};
            creature->take(itemType.get());
          }
        }
        break;
      }
  #endif
      default: break;
    }
  if (creature->isAffected(LastingEffect::SLEEP)) {
    onFellAsleep();
    return;
  }
  for (Vec2 dir : direction)
    if (travel) {
      /*if (WCreature other = creature->getPosition().plus(dir).getCreature())
        extendedAttackAction(other);
      else {
        vector<Vec2> squareDirs = creature->getPosition().getTravelDir();
        if (findElement(squareDirs, dir)) {
          travelDir = dir;
          lastLocation = creature->getPosition().getLocation();
          travelling = true;
          travelAction();
        }
      }*/
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
  auto dirPos = creature->getPosition().plus(dir);
  if (tryToPerform(creature->move(dir)))
    return;
  if (auto action = creature->forceMove(dir)) {
    string nextQuestion = getForceMovementQuestion(dirPos, creature);
    string hereQuestion = getForceMovementQuestion(creature->getPosition(), creature);
    if (hereQuestion == nextQuestion || getView()->yesOrNoPrompt(nextQuestion, true))
      action.perform(creature);
    return;
  }
  if (auto other = dirPos.getCreature()) {
    auto actions = getOtherCreatureCommands(other);
    if (!actions.empty() && actions[0].allowAuto)
      actions[0].perform(this);
    return;
  }
  if (!dirPos.canEnterEmpty(creature))
    tryToPerform(creature->destroy(dir, DestroyAction::Type::BASH));
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
  return creature->getLevel();
}

Game* Player::getGame() const {
  if (creature)
    return creature->getGame();
  else
    return nullptr;
}

WModel Player::getModel() const {
  return creature->getLevel()->getModel();
}

View* Player::getView() const {
  if (auto game = getGame())
    return game->getView();
  else
    return nullptr;
}

Vec2 Player::getPosition() const {
  return creature->getPosition().getCoord();
}

static MessageGenerator messageGenerator(MessageGenerator::SECOND_PERSON);

MessageGenerator& Player::getMessageGenerator() const {
  return messageGenerator;
}

void Player::onStartedControl() {
  getGame()->addPlayer(creature);
  getModel()->getTimeQueue().postponeMove(creature);
}

void Player::onEndedControl() {
  if (auto game = getGame()) // if the whole Game is being destructed then we get null here
    game->removePlayer(creature);
  if (!creature->isDead())
    getModel()->getTimeQueue().postponeMove(creature);
}

void Player::getViewIndex(Vec2 pos, ViewIndex& index) const {
  bool canSee = visibilityMap->isVisible(Position(pos, getLevel())) ||
      getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP);
  Position position = creature->getPosition().withCoord(pos);
  if (canSee)
    position.getViewIndex(index, creature);
  else
    index.setHiddenId(position.getTopViewId());
  if (!canSee)
    if (auto memIndex = getMemory().getViewIndex(position))
      index.mergeFromMemory(*memIndex);
  if (position.isTribeForbidden(creature->getTribeId()))
    index.setHighlight(HighlightType::FORBIDDEN_ZONE);
  if (getGame()->getOptions()->getBoolValue(OptionId::SHOW_MAP))
    for (auto col : getGame()->getCollectives())
      if (col->getTerritory().contains(position))
        index.setHighlight(HighlightType::RECT_SELECTION);
  if (auto furniture = position.getFurniture(FurnitureLayer::MIDDLE)) {
    if (auto clickType = furniture->getClickType())
      if (furniture->getTribe() == creature->getTribeId())
        if (auto& obj = furniture->getViewObject())
          if (index.hasObject(obj->layer()))
            index.getObject(obj->layer()).setExtendedActions({FurnitureClick::getText(*clickType, position, furniture)});
  }
  if (WCreature c = position.getCreature()) {
    if ((canSee && creature->canSeeInPosition(c)) || c == creature ||
        creature->canSeeOutsidePosition(c)) {
      index.insert(c->getViewObjectFor(creature->getTribe()));
      index.equipmentCounts = c->getEquipment().getCounts();
      auto& object = index.getObject(ViewLayer::CREATURE);
      if (c == creature)
        object.setModifier(ViewObject::Modifier::PLAYER);
      if (getTeam().contains(c))
        object.setModifier(ViewObject::Modifier::TEAM_HIGHLIGHT);
      if (creature->isEnemy(c))
        object.setModifier(ViewObject::Modifier::HOSTILE);
      else
        object.getCreatureStatus().intersectWith(getDisplayedOnMinions());
      auto actions = getOtherCreatureCommands(c);
      if (!actions.empty()) {
        if (actions[0].allowAuto)
          object.setClickAction(actions[0].name);
        vector<string> extended;
        for (auto& action : actions)
          if (action.name != object.getClickAction())
            extended.push_back(action.name);
        object.setExtendedActions(extended);
      }
    } else if (creature->isUnknownAttacker(c))
      index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::CREATURE));
  }
  if (unknownLocations->contains(position))
    index.insert(ViewObject(ViewId::UNKNOWN_MONSTER, ViewLayer::TORCH2, "Surprise"));
  if (position != creature->getPosition() && creature->isAffected(LastingEffect::HALLU))
    for (auto& object : index.getAllObjects())
      object.setId(ViewObject::shuffle(object.id(), Random));
}

void Player::onKilled(WConstCreature attacker) {
  unsubscribe();
  getView()->updateView(this, false);
  if (getGame()->getPlayerCreatures().size() == 1 && getView()->yesOrNoPrompt("Display message history?"))
    showHistory();
  if (adventurer)
    getGame()->gameOver(creature, creature->getKills().getSize(), "monsters", creature->getPoints());
}

void Player::onFellAsleep() {
}

vector<WCreature> Player::getTeam() const {
  return {creature};
}

bool Player::isTravelEnabled() const {
  return true;
}

bool Player::handleUserInput(UserInput) {
  return false;
}

optional<FurnitureUsageType> Player::getUsableUsageType() const {
  if (auto furniture = creature->getPosition().getFurniture(FurnitureLayer::MIDDLE))
    if (furniture->canUse(creature))
      if (auto usageType = furniture->getUsageType())
        if (!FurnitureUsage::getUsageQuestion(*usageType, furniture->getName()).empty())
          return usageType;
  return none;
}

vector<TeamMemberAction> Player::getTeamMemberActions(WConstCreature member) const {
  return {};
}

void Player::refreshGameInfo(GameInfo& gameInfo) const {
  gameInfo.messageBuffer = messageBuffer->current;
  gameInfo.singleModel = getGame()->isSingleModel();
  gameInfo.infoType = GameInfo::InfoType::PLAYER;
  SunlightInfo sunlightInfo = getGame()->getSunlightInfo();
  gameInfo.sunlightInfo.description = sunlightInfo.getText();
  gameInfo.sunlightInfo.timeRemaining = sunlightInfo.getTimeRemaining();
  gameInfo.time = creature->getGame()->getGlobalTime();
  gameInfo.playerInfo = PlayerInfo(creature);
  auto& info = *gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>();
  info.controlMode = getGame()->getPlayerCreatures().size() == 1 ? PlayerInfo::LEADER : PlayerInfo::FULL;
  auto team = getTeam();
  auto leader = team[0];
  if (team.size() > 1) {
    auto& timeQueue = getModel()->getTimeQueue();
    auto timeCmp = [&timeQueue](WConstCreature c1, WConstCreature c2) {
      if (c1->isPlayer() && !c2->isPlayer())
        return true;
      if (!c1->isPlayer() && c2->isPlayer())
        return false;
      return timeQueue.compareOrder(c1, c2);
    };
    sort(team.begin(), team.end(), timeCmp);
  }
  for (WConstCreature c : team) {
    info.teamInfos.emplace_back(c);
    info.teamInfos.back().teamMemberActions = getTeamMemberActions(c);
  }
  info.lyingItems.clear();
  if (auto usageType = getUsableUsageType()) {
    auto furniture = creature->getPosition().getFurniture(FurnitureLayer::MIDDLE);
    string question = FurnitureUsage::getUsageQuestion(*usageType, furniture->getName());
    ViewId questionViewId = ViewId::EMPTY;
    if (auto& obj = furniture->getViewObject())
      questionViewId = obj->id();
    info.lyingItems.push_back(getFurnitureUsageInfo(question, questionViewId));
  }
  for (auto stack : creature->stackItems(creature->getPickUpOptions()))
    info.lyingItems.push_back(ItemInfo::get(creature, stack));
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

vector<Vec2> Player::getVisibleEnemies() const {
  return creature->getVisibleEnemies().transform(
      [](WConstCreature c) { return c->getPosition().getCoord(); });
}

double Player::getAnimationTime() const {
  return getModel()->getMoveCounter();
}

Player::CenterType Player::getCenterType() const {
  return CenterType::FOLLOW;
}

const vector<Vec2>& Player::getUnknownLocations(WConstLevel level) const {
  return unknownLocations->getOnLevel(level);
}

void Player::considerAdventurerMusic() {
  for (WCollective col : getModel()->getCollectives())
    if (col->getVillainType() == VillainType::MAIN && !col->isConquered() &&
        col->getTerritory().contains(creature->getPosition())) {
      getGame()->setCurrentMusic(MusicType::ADV_BATTLE, true);
      return;
    }
  getGame()->setCurrentMusic(MusicType::ADV_PEACEFUL, true);
}

REGISTER_TYPE(ListenerTemplate<Player>)
