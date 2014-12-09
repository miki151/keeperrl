#include "stdafx.h"
#include "spell_map.h"

void SpellMap::add(Spell* spell) {
  add(spell->getId());
}

void SpellMap::add(SpellId id) {
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
  return elems[spell->getId()];
}
 
void SpellMap::clear() {
  elems.clear();
}

template <class Archive>
void SpellMap::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(elems);
  CHECK_SERIAL;
}

SERIALIZABLE(SpellMap);
