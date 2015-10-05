#include "stdafx.h"
#include "position_map.h"
#include "level.h"
#include "task.h"
#include "view_index.h"

template <class T>
PositionMap<T>::PositionMap(const vector<Level*>& levels) : PositionMap(levels, T()) {
}

template <class T>
PositionMap<T>::PositionMap(const vector<Level*>& levels, const T& def) {
  for (Level* l : levels) {
    auto id = l->getUniqueId();
    if (id >= tables.size()) {
      tables.resize(id + 1);
      outliers.resize(id + 1);
    }
    tables[id] = Table<T>(l->getBounds().minusMargin(-20), def);
  }
}

template <class T>
bool PositionMap<T>::isValid(Position pos) const {
  return pos.getLevel()->getUniqueId() < tables.size();
}

template <class T>
const T& PositionMap<T>::operator [] (Position pos) const {
  int index = pos.getLevel()->getUniqueId();
  const Table<T>& table = tables.at(index);
  if (pos.getCoord().inRectangle(table.getBounds()))
    return table[pos.getCoord()];
  else if (outliers[index].count(pos.getCoord()))
    return outliers[index].at(pos.getCoord());
  else {
    static T t;
    return t;
  }
}

template <class T>
T& PositionMap<T>::operator [] (Position pos) {
  int index = pos.getLevel()->getUniqueId();
  Table<T>& table = tables.at(index);
  if (pos.getCoord().inRectangle(table.getBounds()))
    return table[pos.getCoord()];
  else
    return outliers[index][pos.getCoord()];
}

template <class T>
template <class Archive> 
void PositionMap<T>::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(tables) & SVAR(outliers);
}

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(PositionMap<T>, PositionMap);

SERIALIZABLE_TMPL(PositionMap, int);
SERIALIZABLE_TMPL(PositionMap, bool);

class Task;

SERIALIZABLE_TMPL(PositionMap, Task*);
SERIALIZABLE_TMPL(PositionMap, HighlightType);
SERIALIZABLE_TMPL(PositionMap, vector<Task*>);
SERIALIZABLE_TMPL(PositionMap, optional<ViewIndex>);
