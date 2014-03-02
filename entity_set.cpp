#include "stdafx.h"
#include "entity_set.h"


EntitySet::EntitySet() {
}

template <class Container>
EntitySet::EntitySet(const Container& v) {
  for (UniqueEntity* e : v)
    insert(e);
}

template EntitySet::EntitySet(const vector<Item*>&);

void EntitySet::insert(const UniqueEntity* e) {
  insert(e->getUniqueId());
}

void EntitySet::erase(const UniqueEntity* e) {
  erase(e->getUniqueId());
}

bool EntitySet::contains(const UniqueEntity* e) const {
  return contains(e->getUniqueId());
}

void EntitySet::insert(UniqueId id) {
  elems.insert(id);
}

void EntitySet::erase(UniqueId id) {
  elems.erase(id);
}

bool EntitySet::contains(UniqueId id) const {
  return elems.count(id);
}

template <class Archive> 
void EntitySet::serialize(Archive& ar, const unsigned int version) {
  ar & BOOST_SERIALIZATION_NVP(elems);
}

SERIALIZABLE(EntitySet);

EntitySet::Iter EntitySet::begin() const {
  return elems.begin();
}

EntitySet::Iter EntitySet::end() const {
  return elems.end();
}

ItemPredicate EntitySet::containsPredicate() const {
  return [this](const Item* it) { return contains(it); };
}
