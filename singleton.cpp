/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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

template class Singleton<Tribe, TribeId>;
template class Singleton<Technology, TechId>;
template class Singleton<Skill, SkillId>;
template class Singleton<Quest, QuestId>;
