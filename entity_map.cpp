#include "stdafx.h"
#include "entity_map.h"
#include "creature.h"
#include "task.h"
#include "collective.h"
#include "collective_config.h"
#include "item.h"
#include "immigrant_info.h"
#include "cost_info.h"
#include "time_queue.h"
#include "game_time.h"
#include "equipment_slot.h"

template <typename Key, typename Value>
EntityMap<Key, Value>::EntityMap() {
}

template <typename Key, typename Value>
EntityMap<Key, Value>::~EntityMap() {
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
const Value& EntityMap<Key, Value>::getOrElse(const Key* key, const Value& value) const {
  return getOrElse(key->getUniqueId(), value);
}

template<typename Key, typename Value>
bool EntityMap<Key,Value>::hasKey(const Key* key) const {
  return hasKey(key->getUniqueId());
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::set(WeakPointer<const Key> key, const Value& v) {
  set(key->getUniqueId(), v);
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::erase(WeakPointer<const Key> key) {
  erase(key->getUniqueId());
}

template <typename Key, typename Value>
const Value& EntityMap<Key, Value>::getOrFail(WeakPointer<const Key> key) const {
  return getOrFail(key->getUniqueId());
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrFail(WeakPointer<const Key> key) {
  return getOrFail(key->getUniqueId());
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrInit(WeakPointer<const Key> key) {
  return getOrInit(key->getUniqueId());
}

template <typename Key, typename Value>
optional<Value> EntityMap<Key, Value>::getMaybe(WeakPointer<const Key> key) const {
  return getMaybe(key->getUniqueId());
}

template <typename Key, typename Value>
const Value& EntityMap<Key, Value>::getOrElse(WeakPointer<const Key> key, const Value& value) const {
  return getOrElse(key->getUniqueId(), value);
}

template<typename Key, typename Value>
bool EntityMap<Key,Value>::hasKey(WeakPointer<const Key> key) const {
  return hasKey(key->getUniqueId());
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
vector<typename UniqueEntity<Key>::Id> EntityMap<Key, Value>::getKeys() const {
  return ::getKeys(elems);
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::set(EntityId id, const Value& value) {
  elems[id] = value;
}

template <typename Key, typename Value>
void EntityMap<Key, Value>::erase(EntityId id) {
  elems.erase(id);
}

template <typename Key, typename Value>
const Value& EntityMap<Key, Value>::getOrFail(EntityId id) const {
  return elems.at(id);
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrFail(EntityId id) {
  return elems.at(id);
}

template <typename Key, typename Value>
Value& EntityMap<Key, Value>::getOrInit(EntityId id) {
  return elems[id];
}

template <typename Key, typename Value>
optional<Value> EntityMap<Key, Value>::getMaybe(EntityId id) const {
  return getValueMaybe(elems, id);
}

template <typename Key, typename Value>
const Value& EntityMap<Key, Value>::getOrElse(EntityId id, const Value& value) const {
  auto iter = elems.find(id);
  if (iter != elems.end())
    return iter->second;
  else
    return value;
}

template<typename Key, typename Value>
bool EntityMap<Key,Value>::hasKey(EntityId key) const {
  return elems.count(key);
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
  ar(elems);
}

SERIALIZABLE_TMPL(EntityMap, Creature, double);
SERIALIZABLE_TMPL(EntityMap, Creature, TimeQueue::ExtendedTime);
SERIALIZABLE_TMPL(EntityMap, Creature, int);
SERIALIZABLE_TMPL(EntityMap, Creature, Task*);
SERIALIZABLE_TMPL(EntityMap, Creature, Collective::CurrentActivity);
SERIALIZABLE_TMPL(EntityMap, Creature, HashMap<AttractionType, int>);
SERIALIZABLE_TMPL(EntityMap, Creature, vector<Position>);
SERIALIZABLE_TMPL(EntityMap, Creature, PositionSet);
SERIALIZABLE_TMPL(EntityMap, Creature, vector<WeakPointer<Item>>);
SERIALIZABLE_TMPL(EntityMap, Creature, Creature*);
SERIALIZABLE_TMPL(EntityMap, Creature, pair<GlobalTime, GlobalTime>);
SERIALIZABLE_TMPL(EntityMap, Creature, ExperienceType);
SERIALIZABLE_TMPL(EntityMap, Creature, ZoneId);
SERIALIZABLE_TMPL(EntityMap, Creature, EnumSet<EquipmentSlot>);
SERIALIZABLE_TMPL(EntityMap, Creature, LocalTime);
SERIALIZABLE_TMPL(EntityMap, Creature, EnumSet<MinionTrait>);
SERIALIZABLE_TMPL(EntityMap, Creature, GlobalTime);
SERIALIZABLE_TMPL(EntityMap, Task, LocalTime);
SERIALIZABLE_TMPL(EntityMap, Task, MinionActivity);
SERIALIZABLE_TMPL(EntityMap, Task, Task*);
SERIALIZABLE_TMPL(EntityMap, Task, MinionTrait);
SERIALIZABLE_TMPL(EntityMap, Task, Position);
SERIALIZABLE_TMPL(EntityMap, Task, CostInfo);
SERIALIZABLE_TMPL(EntityMap, Task, Creature*);
SERIALIZABLE_TMPL(EntityMap, Item, Creature::Id);
SERIALIZABLE_TMPL(EntityMap, Item, WeakPointer<const Task>);
template class EntityMap<Creature, milliseconds>;
