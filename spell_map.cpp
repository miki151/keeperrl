#include "stdafx.h"
#include "spell_map.h"
#include "spell.h"
#include "creature.h"
#include "creature_name.h"
#include "effect.h"
#include "experience_type.h"
#include "creature_attributes.h"

void SpellMap::add(Spell spell, string name, int level) {
  elems.push_back(SpellInfo{std::move(spell), none, level, std::move(name)});
}

void SpellMap::setAllReady() {
  for (auto& elem : elems)
    elem.timeout = none;
}

ExperienceType SpellMap::getExperienceType() const {
  return ExperienceType::SPELL;
}

const SpellMap::SpellInfo* SpellMap::getInfo(const Spell* spell) const {
  for (auto& elem : elems)
    if (&elem.spell == spell)
      return &elem;
  FATAL << "Spell not found";
  fail();
}

SpellMap::SpellInfo* SpellMap::getInfo(const Spell* spell) {
  for (auto& elem : elems)
    if (&elem.spell == spell)
      return &elem;
  FATAL << "Spell not found";
  fail();
}

GlobalTime SpellMap::getReadyTime(const Spell* spell) const {
  return getInfo(spell)->timeout.value_or(-1000_global);
}

void SpellMap::setReadyTime(const Spell* spell, GlobalTime time) {
  getInfo(spell)->timeout = time;
}

vector<const Spell*> SpellMap::getAvailable(const Creature* c) const {
  vector<const Spell*> ret;
  for (auto& elem : elems)
    if (elem.level <= c->getAttributes().getExpLevel(getExperienceType())) {
      bool duplicate = false;
      for (int i : All(ret))
        if (ret[i]->getVariant() == elem.spell.getVariant()) {
          duplicate = true;
          if (ret[i]->getCooldown() > elem.spell.getCooldown())
            ret[i] = &elem.spell;
        }
      if (!duplicate)
        ret.push_back(&elem.spell);
    }
  return ret;
}



const string& SpellMap::getName(const Spell* s) const {
  return getInfo(s)->name;
}

int SpellMap::getLevel(const Spell* s) const {
  return getInfo(s)->level;
}

bool SpellMap::contains(const Spell* s) const {
  for (auto& elem : elems)
    if (&elem.spell == s)
      return true;
  return false;
}

void SpellMap::onExpLevelReached(Creature* c, ExperienceType type, int level) {
  if (type == getExperienceType())
    for (auto& elem : elems)
      if (level == elem.level) {
        bool duplicate = [&] {
          for (auto& elem2 : elems)
            if (elem2.name == elem.name && elem2.level < elem.level)
              return true;
          return false;
        }();
        if (!duplicate) {
          c->addPersonalEvent(c->getName().a() + " learns the spell of " + elem.name);
          c->verb("learn", "learns", "the spell of " + elem.name);
        }
      }
}

SERIALIZE_DEF(SpellMap, elems)
