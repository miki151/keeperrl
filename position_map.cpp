#include "stdafx.h"
#include "position_map.h"
#include "level.h"
#include "task.h"

template <class T>
PositionMap<T>::PositionMap(const vector<Level*>& levels) {
  for (Level* l : levels) {
    tables.emplace_back(l->getBounds().minusMargin(-20), T());
    outliers.emplace_back();
  }
}

template <class T>
PositionMap<T>::PositionMap(const vector<Level*>& levels, const T& def) {
  for (Level* l : levels) {
    tables.emplace_back(l->getBounds().minusMargin(-20), def);
    outliers.emplace_back();
  }
}

template <class T>
const T& PositionMap<T>::operator [] (Position pos) const {
  int index = pos.getLevel()->getUniqueId();
  const Table<T>& table = tables[index];
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
  Table<T>& table = tables[index];
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


