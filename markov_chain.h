#ifndef _MARKOV_CHAIN
#define _MARKOV_CHAIN


template<class T>
class MarkovChain {
  public:
  MarkovChain(T initialState, map<T, vector<pair<T, double>>>);

  T getState() const;
  void setState(T);
  void update();
  bool updateToNext();
  bool containsState(T) const;

  SERIALIZATION_DECL(MarkovChain);

  private:
  T SERIAL(state);
  map<T, vector<pair<T, double>>> SERIAL(transitions);
};

#endif
