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
#include "game.h"
#include "content_factory.h"
#include "special_trait.h"
#include "automaton_part.h"
#include "container_range.h"

CreatureInfo::CreatureInfo(const Creature* c)
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

vector<PlayerInfo::SkillInfo> getSkillNames(const Creature* c) {
  vector<PlayerInfo::SkillInfo> ret;
  for (SkillId id : ENUM_ALL(SkillId))
    if (c->getAttributes().getSkills().getValue(id) > 0)
      ret.push_back(PlayerInfo::SkillInfo{Skill::get(id)->getNameForCreature(c), Skill::get(id)->getHelpText()});
  return ret;
}

vector<ItemAction> getItemActions(const Creature* c, const vector<Item*>& item) {
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
    if (c->getPosition().isValid())
      for (Position v : c->getPosition().neighbors8())
        if (Creature* other = v.getCreature())
          if (c->isFriend(other)/* && c->canTakeItems(item)*/) {
            actions.push_back(ItemAction::GIVE);
            break;
          }
  }
  actions.push_back(ItemAction::NAME);
  return actions;
}

ItemInfo ItemInfo::get(const Creature* creature, const vector<Item*>& stack) {
  PROFILE;
  return CONSTRUCT(ItemInfo,
    c.name = stack[0]->getShortName(creature, stack.size() > 1);
    c.fullName = stack[0]->getNameAndModifiers(false, creature);
    c.description = creature->isAffected(LastingEffect::BLIND)
        ? vector<string>() : stack[0]->getDescription(creature->getGame()->getContentFactory());
    c.number = stack.size();
    c.viewId = stack[0]->getViewObject().id();
    c.viewIdModifiers = stack[0]->getViewObject().getAllModifiers();
    for (auto it : stack)
      c.ids.insert(it->getUniqueId());
    c.actions = getItemActions(creature, stack);
    c.equiped = creature->getEquipment().isEquipped(stack[0]);
    c.weight = stack[0]->getWeight();
    if (stack[0]->getShopkeeper(creature))
      c.price = make_pair(ViewId("gold"), stack[0]->getPrice());
  );
}

static vector<ItemInfo> fillIntrinsicAttacks(const Creature* c) {
  vector<ItemInfo> ret;
  auto& intrinsicAttacks = c->getBody().getIntrinsicAttacks();
  for (auto part : ENUM_ALL(BodyPart))
    for (auto& attack : intrinsicAttacks[part]) {
      ret.push_back(ItemInfo::get(c, {attack.item.get()}));
      auto& item = ret.back();
      item.weight.reset();
      if (!c->getBody().numGood(part)) {
        item.unavailable = true;
        item.unavailableReason = "No functional body part: "_s + getName(part);
        item.actions.clear();
      } else {
        item.intrinsicAttackState = attack.active ? ItemInfo::ACTIVE : ItemInfo::INACTIVE;
        item.intrinsicExtraAttack = attack.isExtraAttack;
        item.actions = {attack.active ? ItemAction::INTRINSIC_DEACTIVATE : ItemAction::INTRINSIC_ACTIVATE};
      }
    }
  return ret;
}

static vector<ItemInfo> getItemInfos(const Creature* c, const vector<Item*>& items) {
  map<string, vector<Item*> > stacks = groupBy<Item*, string>(items,
      [&] (Item* const& item) {
          return item->getNameAndModifiers(false, c) + (c->getEquipment().isEquipped(item) ? "(e)" : ""); });
  vector<ItemInfo> ret;
  for (auto elem : stacks)
    ret.push_back(ItemInfo::get(c, elem.second));
  return ret;
}

static PlayerInfo::SpellSchool fillSpellSchool(const Creature* c, SpellSchoolId id, const ContentFactory* factory) {
  PlayerInfo::SpellSchool ret;
  auto& spellSchool = factory->getCreatures().getSpellSchools().at(id);
  ret.name = id.data();
  ret.experienceType = spellSchool.expType;
  for (auto& id : spellSchool.spells) {
    auto spell = factory->getCreatures().getSpell(id.first);
    ret.spells.push_back(
        PlayerInfo::Spell {
          spell->getName(),
          spell->getSymbol(),
          id.second,
          c->getAttributes().getExpLevel(spellSchool.expType) >= id.second,
          {spell->getDescription(factory)},
          none
        });
  }
  return ret;
}

static string getOrdinal(int index) {
  switch (index) {
    case 1: return "first";
    case 2: return "second";
    case 3: return "third";
    default:
      return toString(index) + "th";
  }
}

void fillInstalledPartDescription(const ContentFactory* factory, ItemInfo& info, const AutomatonPart& part) {
  for (auto e : Iter(part.effect))
    info.description.push_back(getOrdinal(e.index() + 1) + " upgrade: " + e->getDescription(factory));
}

ItemInfo getInstalledPartInfo(const ContentFactory* factory, const AutomatonPart& part, int index) {
  ItemInfo ret {};
  ret.ids.insert(Item::Id(index));
  ret.fullName = ret.name = getName(part.slot);
  ret.viewId = ViewId("trap_item");
  fillInstalledPartDescription(factory, ret, part);
  return ret;
}

PlayerInfo::PlayerInfo(const Creature* c) : bestAttack(c) {
  firstName = c->getName().firstOrBare();
  name = c->getName().bare();
  title = c->getName().title();
  description = capitalFirst(c->getAttributes().getDescription());
  viewId = c->getViewObject().id();
  morale = c->getMorale();
  positionHash = c->getPosition().getHash();
  creatureId = c->getUniqueId();
  attributes = AttributeInfo::fromCreature(c);
  experienceInfo = getCreatureExperienceInfo(c);
  auto contentFactory = c->getGame()->getContentFactory();
  for (auto& id : c->getAttributes().getSpellSchools())
    spellSchools.push_back(fillSpellSchool(c, id, contentFactory));
  intrinsicAttacks = fillIntrinsicAttacks(c);
  skills = getSkillNames(c);
  kills = c->getKills().transform([&](auto& info){ return info.viewId; });
  killTitles = c->getKillTitles();
  effects.clear();
  for (auto& adj : c->getBadAdjectives())
    effects.push_back({adj.name, adj.help, true});
  for (auto& adj : c->getGoodAdjectives())
    effects.push_back({adj.name, adj.help, false});
  spells.clear();
  for (auto spell : c->getSpellMap().getAvailable(c)) {
    vector<string> description = {spell->getDescription(contentFactory), "Cooldown: " + toString(spell->getCooldown())};
    if (spell->getRange() > 0)
      description.push_back("Range: " + toString(spell->getRange()));
    spells.push_back({
        spell->getName(),
        spell->getSymbol(),
        none,
        true,
        std::move(description),
        c->isReady(spell) ? none : optional<TimeInterval>(c->getSpellDelay(spell))
    });
  }
  carryLimit = c->getBody().getCarryLimit();
  map<ItemClass, vector<Item*> > typeGroups = groupBy<Item*, ItemClass>(
      c->getEquipment().getItems(), [](Item* const& item) { return item->getClass();});
  debt = c->getDebt().getTotal();
  for (auto elem : ENUM_ALL(ItemClass))
    if (typeGroups[elem].size() > 0)
      append(inventory, getItemInfos(c, typeGroups[elem]));
  if (c->getPosition().isValid())
    moveCounter = c->getPosition().getModel()->getMoveCounter();
  isPlayerControlled = c->isPlayer();
  canAddBodyPart = c->getAttributes().getAutomatonSlots() > c->automatonParts.size();
  for (int i : All(c->automatonParts))
    bodyParts.push_back(getInstalledPartInfo(contentFactory, c->automatonParts[i], i));
}

const CreatureInfo* CollectiveInfo::getMinion(UniqueEntity<Creature>::Id id) const {
  for (auto& elem : minions)
    if (elem.uniqueId == id)
      return &elem;
  return nullptr;
}


vector<AttributeInfo> AttributeInfo::fromCreature(const Creature* c) {
  PROFILE;
  auto genInfo = [c](AttrType type, const char* help) {
    return AttributeInfo {
        getName(type),
        type,
        c->getAttributes().getRawAttr(type),
        c->getAttrBonus(type, true),
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
      genInfo(
          AttrType::PARRY,
          "Prevents defense penalty from multiple attacks in the same turn."
      ),
    };
}

STRUCT_IMPL(ImmigrantDataInfo)
ImmigrantDataInfo::ImmigrantDataInfo() {}
HASH_DEF(ImmigrantDataInfo, requirements, info, name, viewId, attributes, count, timeLeft, id, autoState, cost, generatedTime, keybinding, tutorialHighlight, specialTraits)
