#include "stdafx.h"
#include "spell_map.h"
#include "spell.h"
#include "creature.h"
#include "creature_name.h"

void SpellMap::add(Spell* spell) {
  add(spell->getId());
}

void SpellMap::add(SpellId id) {
  if (!elems[id])
    elems[id] = -1.0;
}

double SpellMap::getReadyTime(Spell* spell) const {
  CHECK(contains(spell));
  return *elems[spell->getId()];
}

void SpellMap::setReadyTime(Spell* spell, double time) {
  elems[spell->getId()] = time;
}

vector<Spell*> SpellMap::getAll() const {
  vector<Spell*> ret;
  for (Spell* spell : Spell::getAll())
    if (contains(spell))
      ret.push_back(spell);
  return ret;
}

bool SpellMap::contains(Spell* spell) const {
  return !!elems[spell->getId()];
}
 
void SpellMap::clear() {
  elems.clear();
}

void SpellMap::onExpLevelReached(WCreature c, double level) {
  for (auto spell : Spell::getAll())
    if (auto minLevel = spell->getLearningExpLevel())
      if (level >= *minLevel && !contains(spell)) {
        add(spell);
        c->addPersonalEvent(c->getName().a() + " learns the spell of " + spell->getName());
      }
}

SERIALIZE_DEF(SpellMap, elems)

