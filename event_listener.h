#pragma once

#include "util.h"
#include "event_generator.h"
#include "model.h"

template <typename T>
class EventListener {
  public:
  EventListener() {}

  EventListener(const EventListener&) = delete;
  EventListener(EventListener&&) = delete;

  void subscribeTo(WModel m) {
    CHECK(!generator);
    generator = m->eventGenerator.get();
    id = generator->addListener(WeakPointer<T>(static_cast<T*>(this)));
  }

  void unsubscribe() {
    if (generator) {
      generator->removeListener(*id);
      generator = nullptr;
    }
  }

  bool isSubscribed() const {
    return !!generator;
  }

  SERIALIZE_ALL(generator, id)

  ~EventListener() {
    unsubscribe();
  }

  private:
  WeakPointer<EventGenerator> SERIAL(generator);
  optional<EventGenerator::SubscriberId> SERIAL(id);
};

