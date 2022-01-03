#include "stdafx.h"
#include "position_map.h"
#include "level.h"
#include "task.h"
#include "view_index.h"
#include "model.h"
#include "view_object.h"
#include "furniture_type.h"
#include "furniture_layer.h"
#include "construction_map.h"
#include "zones.h"

template <typename T>
static optional<T&> getReferenceOptional(heap_optional<T>& t) {
  if (t)
    return *t;
  else
    return none;
}

template <typename T>
static optional<const T&> getReferenceOptional(const heap_optional<T>& t) {
  if (t)
    return *t;
  else
    return none;
}

template <class T>
optional<const T&> PositionMap<T>::getReferenceMaybe(Position pos) const {
  LevelId levelId = pos.getLevel()->getUniqueId();
  try {
    const auto& table = tables.at(levelId);
    if (pos.getCoord().inRectangle(table.getBounds()))
      return getReferenceOptional(table[pos.getCoord()]);
    else
      return outliers.at(levelId).at(pos.getCoord());
  } catch (std::out_of_range) {
    return none;
  }
}

template <class T>
optional<T&> PositionMap<T>::getReferenceMaybe(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  try {
    auto& table = tables.at(levelId);
    if (pos.getCoord().inRectangle(table.getBounds()))
      return getReferenceOptional(table[pos.getCoord()]);
    else
      return outliers.at(levelId).at(pos.getCoord());
  } catch (std::out_of_range) {
    return none;
  }
}

template<class T>
optional<T> PositionMap<T>::getValueMaybe(Position pos) const {
  if (auto elem = getReferenceMaybe(pos))
    return *elem;
  else
    return none;
}

template<class T>
bool PositionMap<T>::contains(Position pos) const {
  return !!getReferenceMaybe(pos);
}

template <class T>
Table<heap_optional<T>>& PositionMap<T>::getTable(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  try {
    return tables.at(levelId);
  } catch (std::out_of_range) {
    auto it = tables.insert(make_pair(levelId, Table<heap_optional<T>>(pos.getLevel()->getBounds().minusMargin(-2))));
    return it.first->second;
  }
}

template <class T>
T& PositionMap<T>::getOrInit(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  auto& table = getTable(pos);
  if (pos.getCoord().inRectangle(table.getBounds())) {
    if (!table[pos.getCoord()])
      table[pos.getCoord()] = T();
    return *table[pos.getCoord()];
  }
  else try {
    return outliers.at(levelId).at(pos.getCoord());
  } catch (std::out_of_range) {
    return outliers[levelId][pos.getCoord()] = T();
  }
}

template <class T>
T& PositionMap<T>::getOrFail(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  auto& table = tables.at(levelId);
  if (pos.getCoord().inRectangle(table.getBounds()))
    return *table[pos.getCoord()];
  else
    return outliers.at(levelId).at(pos.getCoord());
}

template <class T>
const T& PositionMap<T>::getOrFail(Position pos) const {
  LevelId levelId = pos.getLevel()->getUniqueId();
  auto& table = tables.at(levelId);
  if (pos.getCoord().inRectangle(table.getBounds()))
    return *table[pos.getCoord()];
  else
    return outliers.at(levelId).at(pos.getCoord());
}

template <class T>
void PositionMap<T>::set(Position pos, const T& elem) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  auto& table = getTable(pos);
  if (pos.getCoord().inRectangle(table.getBounds()))
    table[pos.getCoord()] = elem;
  else
    outliers[levelId][pos.getCoord()] = elem;
}

template<class T>
void PositionMap<T>::erase(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  if (auto table = ::getReferenceMaybe(tables, levelId))
    if (pos.getCoord().inRectangle(table->getBounds()))
      (*table)[pos.getCoord()] = none;
  if (auto out = ::getReferenceMaybe(outliers, levelId))
    if (auto elem = ::getReferenceMaybe(*out, pos.getCoord()))
      elem = none;

}

template <class T>
void PositionMap<T>::limitToModel(const WModel m) {
  std::set<LevelId> goodIds;
  for (Level* l : m->getLevels())
    goodIds.insert(l->getUniqueId());
  for (auto& elem : copyOf(tables))
    if (!goodIds.count(elem.first))
      tables.erase(elem.first);
  for (auto& elem : copyOf(outliers))
    if (!goodIds.count(elem.first))
      outliers.erase(elem.first);
}

template <class T>
template <class Archive> 
void PositionMap<T>::serialize(Archive& ar, const unsigned int version) {
  ar(tables, outliers);
}

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(PositionMap<T>, PositionMap)

SERIALIZABLE_TMPL(PositionMap, int)
SERIALIZABLE_TMPL(PositionMap, bool)
SERIALIZABLE_TMPL(PositionMap, double)

class Task;

//SERIALIZABLE_TMPL(PositionMap, WTask)
SERIALIZABLE_TMPL(PositionMap, EnumSet<ZoneId>)
//SERIALIZABLE_TMPL(PositionMap, HighlightType)
//SERIALIZABLE_TMPL(PositionMap, vector<WTask>)
SERIALIZABLE_TMPL(PositionMap, ViewIndex)
SERIALIZABLE_TMPL(PositionMap, vector<Position>)
SERIALIZABLE_TMPL(PositionMap, ConstructionMap::FurnitureInfo);
SERIALIZABLE_TMPL(PositionMap, EnumMap<FurnitureLayer, optional<FurnitureType>>)
SERIALIZABLE_TMPL(PositionMap, Position)

