#pragma once

#include "util.h"

class GameEvent;

class ListenerBase {
  public:
  virtual void onEvent(const GameEvent&) = 0;
  virtual ~ListenerBase() {}

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
  }
};

template<typename T>
class ListenerTemplate : public ListenerBase {
  public:
  ListenerTemplate(WeakPointer<T> p) : ptr(p) {}
  SERIALIZATION_CONSTRUCTOR(ListenerTemplate)

  virtual void onEvent(const GameEvent& e) override {
    ptr->onEvent(e);
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(ListenerBase);
    ar(ptr);
  }

  private:
  WeakPointer<T> SERIAL(ptr);
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



