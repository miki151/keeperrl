#pragma once
#include "util.h"
#include "event_listener.h"


template <typename T>
class EventProxy : public EventListener {
  public:
  EventProxy(T* o, Model* m = nullptr) : owner(o) {
    if (m)
      subscribeTo(m);
  }

  SERIALIZATION_CONSTRUCTOR(EventProxy);
  SERIALIZE_ALL2(EventListener, owner);

  virtual void onEvent(const GameEvent& e) override {
    owner->onEvent(e);
  }

  private:
  T* SERIAL(owner) = nullptr;
};

