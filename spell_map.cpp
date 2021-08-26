#include "stdafx.h"
#include "spell_map.h"
#include "spell.h"
#include "creature.h"
#include "creature_name.h"
#include "effect.h"
#include "experience_type.h"
#include "creature_attributes.h"
#include "gender.h"
#include "creature_factory.h"
#include "game.h"
#include "content_factory.h"
#include "equipment.h"
#include "item.h"

void SpellMap::add(Spell spell, ExperienceType expType, int level) {
  for (auto& elem : elems)
    if (elem.spell.getId() == spell.getId()) {
      elem.level = min(elem.level, level);
      return;
    }
  elems.push_back(SpellInfo{std::move(spell), none, level, expType});
  // it's key to not reference elems in the comparison fun while they are being sorted, so making a copy here
  // otherwise nasty memory corruption
  auto origLevel = [self = *this](const SpellInfo* info1) {
    auto info = info1;
    while (auto upgrade = info->spell.getUpgrade())
      if (auto res = self.getInfo(*upgrade))
        info = res;
      else
        break;
    return info->level;
  };
  sort(elems.begin(), elems.end(), [&origLevel](const auto& e1, const auto& e2) {
    return origLevel(&e1) < origLevel(&e2) || (origLevel(&e1) == origLevel(&e2) && e1.spell.getId() < e2.spell.getId()); });
}

void SpellMap::remove(SpellId id) {
  for (int i = 0; i < elems.size(); ++i)
    if (elems[i].spell.getId() == id) {
      elems.removeIndexPreserveOrder(i);
      elems.shrink_to_fit();
    }
}

void SpellMap::setAllReady() {
  for (auto& elem : elems)
    elem.timeout = none;
}

const SpellMap::SpellInfo* SpellMap::getInfo(SpellId id) const {
  for (auto& elem : elems)
    if (elem.spell.getId() == id)
      return &elem;
  return nullptr;
}

SpellMap::SpellInfo* SpellMap::getInfo(SpellId id) {
  for (auto& elem : elems)
    if (elem.spell.getId() == id)
      return &elem;
  return nullptr;
}

static ItemAbility* getItemAbility(const Creature* c, const Spell* spell) {
  for (auto it : c->getEquipment().getAllEquipped())
    for (auto& a : it->getAbility())
      if (&a.spell == spell)
        return &a;
  return nullptr;
}

GlobalTime SpellMap::getReadyTime(const Creature* c, const Spell* spell) const {
  if (auto info = getInfo(spell->getId()))
    return info->timeout.value_or(-1000_global);
  if (auto a = getItemAbility(c, spell))
    return a->timeout.value_or(-1000_global);
  FATAL << "spell not found";
  fail();
}

void SpellMap::setReadyTime(const Creature* c, const Spell* spell, GlobalTime time) {
  if (auto info = getInfo(spell->getId()))
    info->timeout = time;
  else if (auto a = getItemAbility(c, spell))
    a->timeout = time;
  /*else  this causes a crash if it's a suicide spell granted by equipment
    FATAL << "spell not found";*/
}

vector<const Spell*> SpellMap::getAvailable(const Creature* c) const {
  PROFILE;
  unordered_set<SpellId, CustomHash<SpellId>> upgraded;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(elem.expType))
      if (auto upgrade = elem.spell.getUpgrade())
        upgraded.insert(*upgrade);
  vector<const Spell*> ret;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(elem.expType)) {
      if (!upgraded.count(elem.spell.getId()))
        ret.push_back(&elem.spell);
    }
  for (auto it : c->getEquipment().getAllEquipped())
    for (auto& a : it->getAbility())
      ret.push_back(&a.spell);
  return ret;
}

bool SpellMap::contains(const SpellId id) const {
  return !!getInfo(id);
}

void SpellMap::onExpLevelReached(Creature* c, ExperienceType type, int level) {
  string spellType = type == ExperienceType::SPELL ? "spell"_s : "ability"_s;
  if (auto game = c->getGame()) {
    auto factory = game->getContentFactory();
    for (auto& elem : elems) {
      if (level == elem.level && elem.expType == type) {
        string spellName = elem.spell.getName(factory);
        auto upgrade = elem.spell.getUpgrade();
        if (upgrade && !!getInfo(*upgrade)) {
          string his = ::his(c->getAttributes().getGender());
          c->addPersonalEvent(c->getName().a() + " improves " + his + " " + spellType + " of " + upgrade->data());
          c->verb("improve", "improves", his + " " + spellType + " of " + upgrade->data());
        } else {
          c->addPersonalEvent(c->getName().a() + " learns the " + spellType + " of " + spellName);
          c->verb("learn", "learns", "the " + spellType + " of " + spellName);
        }
      }
    }
  }
}

SERIALIZE_DEF(SpellMap, elems)
