#include "stdafx.h"
#include "singleton.h"
#include "enums.h"
#include "util.h"
#include "quest.h"
#include "tribe.h"
#include "technology.h"

template<class T, class E>
unordered_map<E, unique_ptr<T>> Singleton<T, E>::elems;

template<class T, class E>
void Singleton<T, E>::clearAll() {
  elems.clear();
}

template<class T, class E>
T* Singleton<T, E>::get(E id) {
  TRY(return elems.at(id).get(), "Bad id " << int(id));
}

template<class T, class E>
vector<T*> Singleton<T, E>::getAll() {
  vector<T*> ret;
  for (auto& elem : elems)
    ret.push_back(elem.second.get());
  return ret;
}

template<class T, class E>
bool Singleton<T, E>::exists(E id) {
  return elems.count(id);
}

template<class T, class E>
void Singleton<T, E>::set(E id, T* q) {
  CHECK(!elems.count(id));
  elems[id].reset(q);
}

template class Singleton<Quest, QuestId>;
template class Singleton<Tribe, TribeId>;
template class Singleton<Technology, TechId>;
