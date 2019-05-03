#include "stdafx.h"
#include "spell_map.h"
#include "spell.h"
#include "creature.h"
#include "creature_name.h"
#include "effect.h"
#include "experience_type.h"
#include "creature_attributes.h"
#include "gender.h"

void SpellMap::add(Spell spell, int level) {
  for (auto& elem : elems)
    if (elem.spell.getId() == spell.getId()) {
      elem.level = min(elem.level, level);
      return;
    }
  elems.push_back(SpellInfo{std::move(spell), none, level});
  auto origLevel = [&](const SpellInfo* info1) {
    auto info = info1;
    while (auto upgrade = info->spell.getUpgrade())
      if (auto res = getInfo(*upgrade))
        info = res;
      else
        break;
    return info->level;
  };
  sort(elems.begin(), elems.end(), [&](const auto& e1, const auto& e2) {
      return origLevel(&e1) < origLevel(&e2) || (origLevel(&e1) == origLevel(&e2) && e1.spell.getId() < e2.spell.getId()); });
}

void SpellMap::setAllReady() {
  for (auto& elem : elems)
    elem.timeout = none;
}

const string& SpellMap::getName(const Spell* s) const {
  while (auto& upgrade = s->getUpgrade())
    s = &getInfo(*upgrade)->spell;
  return s->getId();
}

ExperienceType SpellMap::getExperienceType() const {
  return ExperienceType::SPELL;
}

const SpellMap::SpellInfo* SpellMap::getInfo(const string& id) const {
  for (auto& elem : elems)
    if (elem.spell.getId() == id)
      return &elem;
  return nullptr;
}

SpellMap::SpellInfo* SpellMap::getInfo(const string& id) {
  for (auto& elem : elems)
    if (elem.spell.getId() == id)
      return &elem;
  return nullptr;
}

GlobalTime SpellMap::getReadyTime(const Spell* spell) const {
  return getInfo(spell->getId())->timeout.value_or(-1000_global);
}

void SpellMap::setReadyTime(const Spell* spell, GlobalTime time) {
  getInfo(spell->getId())->timeout = time;
}

vector<const Spell*> SpellMap::getAvailable(const Creature* c) const {
  unordered_set<string> upgraded;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(getExperienceType()))
      if (auto& upgrade = elem.spell.getUpgrade())
        upgraded.insert(*upgrade);
  vector<const SpellInfo*> ret;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(getExperienceType())) {
      if (!upgraded.count(elem.spell.getId()))
        ret.push_back(&elem);
    }
  return ret.transform([](const auto& elem) { return &elem->spell; } );
}

int SpellMap::getLevel(const Spell* s) const {
  return getInfo(s->getId())->level;
}

bool SpellMap::contains(const Spell* s) const {
  for (auto& elem : elems)
    if (&elem.spell == s)
      return true;
  return false;
}

void SpellMap::onExpLevelReached(Creature* c, ExperienceType type, int level) {
  if (type == getExperienceType())
    for (auto& elem : elems) {
      if (level == elem.level) {
        if (auto& upgrade = elem.spell.getUpgrade()) {
          string his = ::his(c->getAttributes().getGender());
          c->addPersonalEvent(c->getName().a() + " improves " + his + " spell of " + getName(&elem.spell));
          c->verb("improve", "improves", his + " spell of " + getName(&elem.spell));
        } else {
          c->addPersonalEvent(c->getName().a() + " learns the spell of " + getName(&elem.spell));
          c->verb("learn", "learns", "the spell of " + getName(&elem.spell));
        }
      }
    }
}

SERIALIZE_DEF(SpellMap, elems)
