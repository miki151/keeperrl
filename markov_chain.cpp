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

#include "markov_chain.h"
#include "debug.h"
#include "enums.h"
#include "util.h"

template <class T>
template <class Archive>
void MarkovChain<T>::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(state)
    & SVAR(transitions);
  CHECK_SERIAL;
}

SERIALIZABLE(MarkovChain<MinionTask>);

template <class T>
SERIALIZATION_CONSTRUCTOR_IMPL2(MarkovChain<T>, MarkovChain);

template<class T>
MarkovChain<T>::MarkovChain(T s, map<T, vector<pair<T, double>>> t) : state(s), transitions(t) {
  for (auto elem : transitions) {
    double sum = 0;
    for (auto trans : elem.second) {
      sum += trans.second;
      CHECK(trans.first != elem.first);
    }
    CHECK(sum <= 1.0000001);
    if (sum < 1)
      transitions[elem.first].emplace_back(elem.first, 1 - sum);
  }
}

template<class T>
T MarkovChain<T>::getState() const {
  return state;
}

template<class T>
void MarkovChain<T>::setState(T s) {
  state = s;
}

template<class T>
void MarkovChain<T>::update() {
  state = chooseRandom(transitions.at(state));
}

template<class T>
bool MarkovChain<T>::containsState(T state) const {
  return transitions.count(state);
}


template class MarkovChain<MinionTask>;
