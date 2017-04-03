#pragma once

#include "util.h"

class GameEvent;

class ListenerBase {
  public:
  virtual void onEvent(const GameEvent&) = 0;
  virtual ~ListenerBase() {}
};

template<typename T>
class ListenerTemplate : public ListenerBase {
  public:
  ListenerTemplate(WeakPointer<T> p) : ptr(p) {}

  virtual void onEvent(const GameEvent& e) override {
    ptr->onEvent(e);
  }

  private:
  WeakPointer<T> ptr;
};

class EventGenerator : public OwnedObject<EventGenerator> {
  public:
  using SubscriberId = long long;

  void addEvent(const GameEvent&);

  template <typename T>
  SubscriberId addListener(WeakPointer<T> t) {
    auto id = Random.getLL();
    listeners.emplace(id, unique_ptr<ListenerBase>(new ListenerTemplate<T>(t)));
    return id;
  }

  void removeListener(SubscriberId id);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  map<SubscriberId, unique_ptr<ListenerBase>> SERIAL(listeners);
};



