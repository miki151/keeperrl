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

PlayerInfo::PlayerInfo(WConstCreature c) : bestAttack(c) {
  firstName = c->getName().first().value_or("");
  name = c->getName().bare();
  title = c->getName().title();
  description = capitalFirst(c->getAttributes().getDescription());
  WItem weapon = c->getWeapon();
  weaponName = weapon ? weapon->getName() : "";
  viewId = c->getViewObject().id();
  morale = c->getMorale();
  levelName = c->getLevel()->getName();
  positionHash = c->getPosition().getHash();
  creatureId = c->getUniqueId();
  attributes = AttributeInfo::fromCreature(c);
  levelInfo.level = c->getAttributes().getExpLevel();
  levelInfo.limit = c->getAttributes().getMaxExpLevel();
  skills = getSkillNames(c);
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
}

const CreatureInfo* CollectiveInfo::getMinion(UniqueEntity<Creature>::Id id) const {
  for (auto& elem : minions)
    if (elem.uniqueId == id)
      return &elem;
  return nullptr;
}


vector<AttributeInfo> AttributeInfo::fromCreature(WConstCreature c) {
  auto genInfo = [c](AttrType type, int bonus, const char* help) {
    return AttributeInfo {
        getName(type),
        type,
        c->getAttr(type),
        bonus,
        help
    };
  };
  return {
      genInfo(
          AttrType::DAMAGE,
          c->isAffected(LastingEffect::RAGE) ? 1 : c->isAffected(LastingEffect::PANIC) ? -1 : 0,
          "Affects if and how much damage is dealt in combat."
      ),
      genInfo(
          AttrType::DEFENSE,
          c->isAffected(LastingEffect::RAGE) ? -1 : (c->isAffected(LastingEffect::PANIC)) ? 1 : 0,
          "Affects if and how much damage is taken in combat."
      ),
      genInfo(
          AttrType::SPELL_DAMAGE,
          0,
          "Base value of magic attacks."
      ),
      genInfo(
          AttrType::RANGED_DAMAGE,
          0,
          "Affects if and how much damage is dealt when shooting a ranged weapon."
      ),
    };
}
