#include "stdafx.h"
#include "game_info.h"
#include "creature.h"
#include "spell.h"
#include "creature_name.h"
#include "skill.h"
#include "view_id.h"
#include "level.h"
#include "position.h"
#include "creature_attributes.h"
#include "view_object.h"
#include "spell_map.h"
#include "item.h"
#include "body.h"
#include "equipment.h"
#include "creature_debt.h"
#include "item_class.h"
#include "model.h"
#include "time_queue.h"

CreatureInfo::CreatureInfo(WConstCreature c)
    : viewId(c->getViewObject().id()),
      uniqueId(c->getUniqueId()),
      name(c->getName().bare()),
      stackName(c->getName().stack()),
      bestAttack(c->getBestAttack()),
      morale(c->getMorale()) {
}

string PlayerInfo::getFirstName() const {
  if (!firstName.empty())
    return firstName;
  else
    return capitalFirst(name);
}

string PlayerInfo::getTitle() const {
  return title;
}

vector<PlayerInfo::SkillInfo> getSkillNames(WConstCreature c) {
  vector<PlayerInfo::SkillInfo> ret;
  for (auto skill : c->getAttributes().getSkills().getAllDiscrete())
    ret.push_back(PlayerInfo::SkillInfo{Skill::get(skill)->getName(), Skill::get(skill)->getHelpText()});
  for (SkillId id : ENUM_ALL(SkillId))
    if (!Skill::get(id)->isDiscrete() && c->getAttributes().getSkills().getValue(id) > 0)
      ret.push_back(PlayerInfo::SkillInfo{Skill::get(id)->getNameForCreature(c), Skill::get(id)->getHelpText()});
  return ret;
}

vector<ItemAction> getItemActions(WConstCreature c, const vector<WItem>& item) {
  PROFILE;
  vector<ItemAction> actions;
  if (c->equip(item[0]))
    actions.push_back(ItemAction::EQUIP);
  if (c->applyItem(item[0]))
    actions.push_back(ItemAction::APPLY);
  if (c->unequip(item[0]))
    actions.push_back(ItemAction::UNEQUIP);
  else {
    actions.push_back(ItemAction::THROW);
    actions.push_back(ItemAction::DROP);
    if (item.size() > 1)
      actions.push_back(ItemAction::DROP_MULTI);
    if (item[0]->getShopkeeper(c))
      actions.push_back(ItemAction::PAY);
    for (Position v : c->getPosition().neighbors8())
      if (WCreature other = v.getCreature())
        if (c->isFriend(other)/* && c->canTakeItems(item)*/) {
          actions.push_back(ItemAction::GIVE);
          break;
        }
  }
  actions.push_back(ItemAction::NAME);
  return actions;
}

ItemInfo ItemInfo::get(WConstCreature creature, const vector<WItem>& stack) {
  PROFILE;
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(creature, stack.size() > 1);
    c.fullName = stack[0]->getNameAndModifiers(false, creature);
    c.description = creature->isAffected(LastingEffect::BLIND) ? "" : stack[0]->getDescription();
    c.number = stack.size();
    c.viewId = stack[0]->getViewObject().id();
    for (auto it : stack)
      c.ids.insert(it->getUniqueId());
    c.actions = getItemActions(creature, stack);
    c.equiped = creature->getEquipment().isEquipped(stack[0]);
    c.weight = stack[0]->getWeight();
    if (stack[0]->getShopkeeper(creature))
      c.price = make_pair(ViewId::GOLD, stack[0]->getPrice());
  );
}

static vector<ItemInfo> fillIntrinsicAttacks(WConstCreature c) {
  vector<ItemInfo> ret;
  auto& intrinsicAttacks = c->getBody().getIntrinsicAttacks();
  for (auto part : ENUM_ALL(BodyPart))
    if (auto& attack = intrinsicAttacks[part]) {
      ret.push_back(ItemInfo::get(c, {attack->item.get()}));
      auto& item = ret.back();
      item.weight.reset();
      if (!c->getBody().numGood(part)) {
        item.unavailable = true;
        item.unavailableReason = "No functional body part: "_s + getName(part);
        item.actions.clear();
      } else {
        item.intrinsicState = attack->active;
        item.actions = {
            ItemAction::INTRINSIC_ALWAYS, ItemAction::INTRINSIC_NO_WEAPON, ItemAction::INTRINSIC_NEVER};
      }
    }
  return ret;
}

static vector<ItemInfo> getItemInfos(WConstCreature c, const vector<WItem>& items) {
  map<string, vector<WItem> > stacks = groupBy<WItem, string>(items,
      [&] (WItem const& item) {
          return item->getNameAndModifiers(false, c) + (c->getEquipment().isEquipped(item) ? "(e)" : ""); });
  vector<ItemInfo> ret;
  for (auto elem : stacks)
    ret.push_back(ItemInfo::get(c, elem.second));
  return ret;
}

PlayerInfo::PlayerInfo(WConstCreature c) : bestAttack(c) {
  firstName = c->getName().first().value_or("");
  name = c->getName().bare();
  title = c->getName().title();
  description = capitalFirst(c->getAttributes().getDescription());
  viewId = c->getViewObject().id();
  morale = c->getMorale();
  positionHash = c->getPosition().getHash();
  creatureId = c->getUniqueId();
  attributes = AttributeInfo::fromCreature(c);
  levelInfo.level = c->getAttributes().getExpLevel();
  levelInfo.limit = c->getAttributes().getMaxExpLevel();
  levelInfo.combatExperience = c->getAttributes().getCombatExperience();
  intrinsicAttacks = fillIntrinsicAttacks(c);
  skills = getSkillNames(c);
  willMoveThisTurn = c->getPosition().getModel()->getTimeQueue().willMoveThisTurn(c);
  effects.clear();
  for (auto& adj : c->getBadAdjectives())
    effects.push_back({adj.name, adj.help, true});
  for (auto& adj : c->getGoodAdjectives())
    effects.push_back({adj.name, adj.help, false});
  spells.clear();
  for (::Spell* spell : c->getAttributes().getSpellMap().getAll()) {
    bool ready = c->isReady(spell);
    spells.push_back({
        spell->getId(),
        spell->getName() + (ready ? "" : " [" + toString(c->getSpellDelay(spell)) + "]"),
        spell->getDescription(),
        c->isReady(spell) ? none : optional<TimeInterval>(c->getSpellDelay(spell))});
  }
  carryLimit = c->getBody().getCarryLimit();
  map<ItemClass, vector<WItem> > typeGroups = groupBy<WItem, ItemClass>(
      c->getEquipment().getItems(), [](WItem const& item) { return item->getClass();});
  debt = c->getDebt().getTotal();
  for (auto elem : ENUM_ALL(ItemClass))
    if (typeGroups[elem].size() > 0)
      append(inventory, getItemInfos(c, typeGroups[elem]));
  moveCounter = c->getPosition().getModel()->getMoveCounter();
  isPlayerControlled = c->isPlayer();
}

const CreatureInfo* CollectiveInfo::getMinion(UniqueEntity<Creature>::Id id) const {
  for (auto& elem : minions)
    if (elem.uniqueId == id)
      return &elem;
  return nullptr;
}


vector<AttributeInfo> AttributeInfo::fromCreature(WConstCreature c) {
  PROFILE;
  auto genInfo = [c](AttrType type, const char* help) {
    return AttributeInfo {
        getName(type),
        type,
        c->getAttributes().getRawAttr(type),
        c->getAttrBonus(type),
        help
    };
  };
  return {
      genInfo(
          AttrType::DAMAGE,
          "Affects if and how much damage is dealt in combat."
      ),
      genInfo(
          AttrType::DEFENSE,
          "Affects if and how much damage is taken in combat."
      ),
      genInfo(
          AttrType::SPELL_DAMAGE,
          "Base value of magic attacks."
      ),
      genInfo(
          AttrType::RANGED_DAMAGE,
          "Affects if and how much damage is dealt when shooting a ranged weapon."
      ),
    };
}
