#ifndef _MARKOV_CHAIN
#define _MARKOV_CHAIN


template<class T>
class MarkovChain {
  public:
  MarkovChain(T initialState, map<T, vector<pair<T, double>>>);

  T getState() const;
  void setState(T);
  void update();

  private:
  T state;
  map<T, vector<pair<T, double>>> transitions;
};

#endif
