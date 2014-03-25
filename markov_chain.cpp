#include "stdafx.h"

#include "markov_chain.h"
#include "debug.h"
#include "enums.h"
#include "util.h"

template <class T>
template <class Archive>
void MarkovChain<T>::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(state)
    & BOOST_SERIALIZATION_NVP(transitions);
}

SERIALIZABLE(MarkovChain<MinionTask>);

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
bool MarkovChain<T>::updateToNext() {
  const vector<pair<T, double>> t = transitions.at(state);
  if (t.size() == 1)
    return false;
  state = chooseRandom(getPrefix(t, 0, t.size() - 1));
  return true;
}

template<class T>
bool MarkovChain<T>::containsState(T state) const {
  return transitions.count(state);
}


template class MarkovChain<MinionTask>;
