#pragma once

#include "util.h"
#include "event_generator.h"
#include "position.h"
#include "enum_variant.h"
#include "model.h"

class Model;
class Technology;
class Collective;

enum class EventId {
  MOVED,
  KILLED,
  PICKED_UP,
  DROPPED,
  ITEMS_APPEARED,
  PROJECTILE,
  EXPLOSION,
  CONQUERED_ENEMY,
  WON_GAME,
  TECHBOOK_READ,
  ALARM,
  TORTURED,
  SURRENDERED,
  TRAP_TRIGGERED,
  TRAP_DISARMED,
  FURNITURE_DESTROYED,
  EQUIPED,
  CREATURE_EVENT
};

namespace EventInfo {

struct Attacked {
  WCreature victim;
  WCreature attacker;
};

struct ItemsHandled {
  WCreature creature;
  vector<WItem> items;
};

struct ItemsAppeared {
  Position position;
  vector<WItem> items;
};

struct Projectile {
  ViewId viewId;
  Position begin;
  Position end;
};

struct TrapDisarmed {
  Position position;
  WCreature creature;
};

struct CreatureEvent {
  WCreature creature;
  string message;
};

struct FurnitureEvent {
  Position position;
  FurnitureLayer layer;
};

}

class GameEvent : public EnumVariant<EventId, TYPES(WCreature, Position, Technology*, WCollective,
    EventInfo::CreatureEvent, EventInfo::Attacked, EventInfo::ItemsHandled, EventInfo::ItemsAppeared,
    EventInfo::Projectile, EventInfo::TrapDisarmed, EventInfo::FurnitureEvent),
    ASSIGN(WCreature, EventId::MOVED),
    ASSIGN(Position, EventId::EXPLOSION, EventId::ALARM, EventId::TRAP_TRIGGERED),
    ASSIGN(Technology*, EventId::TECHBOOK_READ),
    ASSIGN(WCollective, EventId::CONQUERED_ENEMY),
    ASSIGN(EventInfo::CreatureEvent, EventId::CREATURE_EVENT),
    ASSIGN(EventInfo::Attacked, EventId::KILLED, EventId::TORTURED, EventId::SURRENDERED),
    ASSIGN(EventInfo::ItemsHandled, EventId::PICKED_UP, EventId::DROPPED, EventId::EQUIPED),
    ASSIGN(EventInfo::ItemsAppeared, EventId::ITEMS_APPEARED),
    ASSIGN(EventInfo::Projectile, EventId::PROJECTILE),
    ASSIGN(EventInfo::TrapDisarmed, EventId::TRAP_DISARMED),
    ASSIGN(EventInfo::FurnitureEvent, EventId::FURNITURE_DESTROYED) > {
  using EnumVariant::EnumVariant;
};

template <typename T>
class EventListener {
  public:
  EventListener() {}

  EventListener(const EventListener&) = delete;
  EventListener(EventListener&&) = delete;

  void subscribeTo(WModel m) {
    CHECK(!generator && !id);
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

  SERIALIZE_ALL(generator, id);

  ~EventListener() {
    unsubscribe();
  }

  private:
  WeakPointer<EventGenerator> SERIAL(generator);
  optional<EventGenerator::SubscriberId> SERIAL(id);
};

