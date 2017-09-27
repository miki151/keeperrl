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

template <class T>
PositionMap<T>::PositionMap(const T& def) : defaultVal(def) {
}

template <class T>
const T& PositionMap<T>::get(Position pos) const {
  LevelId levelId = pos.getLevel()->getUniqueId();
  try {
    const Table<T>& table = tables.at(levelId);
    if (pos.getCoord().inRectangle(table.getBounds()))
      return table[pos.getCoord()];
    else
      return outliers.at(levelId).at(pos.getCoord());
  } catch (std::out_of_range) {
    return defaultVal;
  }
}

template <class T>
Table<T>& PositionMap<T>::getTable(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  try {
    return tables.at(levelId);
  } catch (std::out_of_range) {
    auto it = tables.insert(make_pair(levelId, Table<T>(pos.getLevel()->getBounds().minusMargin(-20), defaultVal)));
    return it.first->second;
  }
}

template <class T>
T& PositionMap<T>::getOrInit(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  Table<T>& table = getTable(pos);
  if (pos.getCoord().inRectangle(table.getBounds())) {
    return table[pos.getCoord()];
  }
  else try {
    return outliers.at(levelId).at(pos.getCoord());
  } catch (std::out_of_range) {
    return outliers[levelId][pos.getCoord()]  = defaultVal;
  }
}

template <class T>
T& PositionMap<T>::getOrFail(Position pos) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  try {
    Table<T>& table = tables.at(levelId);
    if (pos.getCoord().inRectangle(table.getBounds()))
      return table[pos.getCoord()];
    else
      return outliers.at(levelId).at(pos.getCoord());
  } catch (std::out_of_range) {
    FATAL << "getOrFail failed " << pos.getCoord();
    return defaultVal;
  }
}

template <class T>
void PositionMap<T>::set(Position pos, const T& elem) {
  LevelId levelId = pos.getLevel()->getUniqueId();
  Table<T>& table = getTable(pos);
  if (pos.getCoord().inRectangle(table.getBounds()))
    table[pos.getCoord()] = elem;
  else
    outliers[levelId][pos.getCoord()] = elem;
}

template <class T>
void PositionMap<T>::limitToModel(const WModel m) {
  std::set<LevelId> goodIds;
  for (WLevel l : m->getLevels())
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
  ar(tables, outliers, defaultVal);
}

SERIALIZABLE_TMPL(PositionMap, int)
SERIALIZABLE_TMPL(PositionMap, bool)
SERIALIZABLE_TMPL(PositionMap, double)

class Task;

SERIALIZABLE_TMPL(PositionMap, WTask)
SERIALIZABLE_TMPL(PositionMap, HighlightType)
SERIALIZABLE_TMPL(PositionMap, vector<WTask>)
SERIALIZABLE_TMPL(PositionMap, optional<ViewIndex>)
SERIALIZABLE_TMPL(PositionMap, optional<vector<Position>>)
SERIALIZABLE_TMPL(PositionMap, optional<ConstructionMap::FurnitureInfo>);
SERIALIZABLE_TMPL(PositionMap, optional<ConstructionMap::TrapInfo>);
SERIALIZABLE_TMPL(PositionMap, EnumMap<FurnitureLayer, optional<FurnitureType>>)
