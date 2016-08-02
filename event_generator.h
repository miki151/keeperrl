#ifndef _EVENT_GENERATOR_H
#define _EVENT_GENERATOR_H

#include "util.h"

template <typename Listener>
class EventGenerator {
  public:
  ~EventGenerator();
  const vector<Listener*> getListeners() const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  friend Listener;
  void addListener(Listener*);
  void removeListener(Listener*);
  vector<Listener*> SERIAL(listeners);
};



#endif
