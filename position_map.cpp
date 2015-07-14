#include "stdafx.h"
#include "position_map.h"
#include "level.h"
#include "task.h"

template <class T>
PositionMap<T>::PositionMap(const vector<Level*>& levels) {
  for (Level* l : levels)
    tables.emplace_back(l->getBounds(), T());
}

template <class T>
const T& PositionMap<T>::operator [] (Position pos) const {
  return tables[pos.getLevel()->getUniqueId()][pos.getCoord()];
}

template <class T>
T& PositionMap<T>::operator [] (Position pos) {
  return tables[pos.getLevel()->getUniqueId()][pos.getCoord()];
}

template <class T>
template <class Archive> 
void PositionMap<T>::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(tables);
}

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(PositionMap<T>, PositionMap);

SERIALIZABLE_TMPL(PositionMap, int);
SERIALIZABLE_TMPL(PositionMap, bool);

class Task;

SERIALIZABLE_TMPL(PositionMap, Task*);
SERIALIZABLE_TMPL(PositionMap, HighlightType);
SERIALIZABLE_TMPL(PositionMap, vector<Task*>);


