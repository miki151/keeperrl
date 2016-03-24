#include "stdafx.h"
#include "entity_map.h"
#include "creature.h"
#include "task.h"
#include "collective.h"
#include "collective_config.h"
#include "item.h"

template <typename Key, typename Value>
EntityMap<Key, Value>::EntityMap() {
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::set(const Key* key, const Value& v) {
  set(key->getUniqueId(), v);
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::erase(const Key* key) {
  erase(key->getUniqueId());
}

template <typename Key, typename Value>
const Value& EntityMap<Key, Value>::getOrFail(const Key* key) const {
  return getOrFail(key->getUniqueId());
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrFail(const Key* key) {
  return getOrFail(key->getUniqueId());
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrInit(const Key* key) {
  return getOrInit(key->getUniqueId());
}

template <typename Key, typename Value>
optional<Value> EntityMap<Key, Value>::getMaybe(const Key* key) const {
  return getMaybe(key->getUniqueId());
}

template <typename Key, typename Value>
bool EntityMap<Key, Value>::empty() const {
  return elems.empty();
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::clear() {
  elems.clear();
}

template <typename Key, typename Value>
int EntityMap<Key, Value>::getSize() const {
  return elems.size();
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::set(typename UniqueEntity<Key>::Id id, const Value& value) {
  elems[id] = value;
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::erase(typename UniqueEntity<Key>::Id id) {
  elems.erase(id);
}

template <typename Key, typename Value>
const Value& EntityMap<Key, Value>::getOrFail(typename UniqueEntity<Key>::Id id) const {
  return elems.at(id);
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrFail(typename UniqueEntity<Key>::Id id) {
  return elems.at(id);
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrInit(typename UniqueEntity<Key>::Id id) {
  return elems[id];
}

template <typename Key, typename Value>
optional<Value> EntityMap<Key, Value>::getMaybe(typename UniqueEntity<Key>::Id id) const {
  try {
    return getOrFail(id);
  } catch (std::out_of_range) {
    return none;
  }
}

template <typename Key, typename Value>
typename EntityMap<Key, Value>::Iter EntityMap<Key, Value>::begin() const {
  return elems.begin();
}

template <typename Key, typename Value>
typename EntityMap<Key, Value>::Iter EntityMap<Key, Value>::end() const {
  return elems.end();
}


template <typename Key, typename Value>
template <class Archive> 
void EntityMap<Key, Value>::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, elems);
}


SERIALIZABLE_TMPL(EntityMap, Creature, double);
SERIALIZABLE_TMPL(EntityMap, Creature, int);
SERIALIZABLE_TMPL(EntityMap, Creature, Collective::CurrentTaskInfo);
SERIALIZABLE_TMPL(EntityMap, Creature, Collective::MinionPaymentInfo);
SERIALIZABLE_TMPL(EntityMap, Creature, vector<AttractionInfo>);
SERIALIZABLE_TMPL(EntityMap, Creature, vector<Position>);
SERIALIZABLE_TMPL(EntityMap, Task, double);
SERIALIZABLE_TMPL(EntityMap, Item, const Creature*);
