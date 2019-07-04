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

GlobalTime SpellMap::getReadyTime(const Spell* spell) const {
  return getInfo(spell->getId())->timeout.value_or(-1000_global);
}

void SpellMap::setReadyTime(const Spell* spell, GlobalTime time) {
  getInfo(spell->getId())->timeout = time;
}

vector<const Spell*> SpellMap::getAvailable(const Creature* c) const {
  unordered_set<SpellId, CustomHash<SpellId>> upgraded;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(elem.expType))
      if (auto upgrade = elem.spell.getUpgrade())
        upgraded.insert(*upgrade);
  vector<const SpellInfo*> ret;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(elem.expType)) {
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
  string spellType = type == ExperienceType::SPELL ? "spell"_s : "ability"_s;
  for (auto& elem : elems) {
    if (level == elem.level && elem.expType == type) {
      string spellName = elem.spell.getName();
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

SERIALIZE_DEF(SpellMap, elems)
