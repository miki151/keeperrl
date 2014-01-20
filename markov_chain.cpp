#include "stdafx.h"

#include "markov_chain.h"
#include "debug.h"
#include "enums.h"
#include "util.h"

template<class T>
MarkovChain<T>::MarkovChain(T s, map<T, vector<pair<T, double>>> t) : state(s), transitions(t) {
  for (auto elem : transitions) {
    double sum = 0;
    for (auto trans : elem.second)
      sum += trans.second;
    CHECK(sum <= 1);
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

template class MarkovChain<MinionTask>;
